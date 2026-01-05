#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "worker_pool.h"

static WorkerPool *pool = NULL;

// 内部函数：工作线程的循环体
static void *worker_thread(void *arg) {
    while (true) {
        pthread_mutex_lock(&(pool->lock));

        // 如果队列为空且未关闭，则阻塞等待信号
        while (pool->queue_head == NULL && !pool->shutdown) {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }

        if (pool->shutdown && pool->queue_head == NULL) {
            pthread_mutex_unlock(&(pool->lock));
            pthread_exit(NULL);
        }

        // 取出任务
        Task *task = pool->queue_head;
        if (task != NULL) {
            pool->queue_head = task->next;
            // 如果取完后头变空了，说明队列空了，尾部也要置空
            if (pool->queue_head == NULL) {
                pool->queue_tail = NULL;
            }
            pool->queue_size--;
        }

        pthread_mutex_unlock(&(pool->lock));

        // 在锁外执行函数，防止阻塞其他线程取任务
        if (task != NULL) {
            (*(task->function))(task->arg);
            free(task); // 注意：参数 arg 的释放由具体的 task function 决定
        }
    }
    return NULL;
}

int worker_pool_init(int thread_count) {
    pool = (WorkerPool *)malloc(sizeof(WorkerPool));
    if(!pool){
        perror("malloc workerpool");
        return -1;
    }
    pool->thread_count = thread_count;
    pool->queue_size = 0;
    pool->queue_head = NULL;
    pool->queue_tail = NULL;
    pool->shutdown = false;

    pthread_mutex_init(&(pool->lock), NULL);
    pthread_cond_init(&(pool->notify), NULL);

    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);
    if(!pool->threads){
        perror("malloc threads");
        // --- 重点：开始清理前面已经申请的资源 ---
        pthread_mutex_destroy(&(pool->lock));
        pthread_cond_destroy(&(pool->notify));
        free(pool); 
        pool = NULL; // 防止悬空指针
        return -1;
    }
    for (int i = 0; i < thread_count; i++) {
        pthread_create(&(pool->threads[i]), NULL, worker_thread, NULL);
    }

    printf("[WorkerPool] Initialized with %d threads.\n", thread_count);
    return 0;
}

int worker_pool_add_task(void (*function)(void *), void *arg) {
    pthread_mutex_lock(&(pool->lock));

    Task *new_task = (Task *)malloc(sizeof(Task));
    if(!new_task) {
        pthread_mutex_unlock(&(pool->lock));
        return -1;
    }
    new_task->function = function;
    new_task->arg = arg;
    new_task->next = NULL; // 新任务永远是最后一个

    if (pool->queue_head == NULL) {
        // 如果队列为空，头尾都指向它
        pool->queue_head = new_task;
        pool->queue_tail = new_task;
    } else {
        // 如果不为空，挂在当前尾部的后面，并更新尾部指针
        pool->queue_tail->next = new_task;
        pool->queue_tail = new_task;
    }

    pool->queue_size++;
    pthread_cond_signal(&(pool->notify));
    pthread_mutex_unlock(&(pool->lock));
    return 0;
}

void worker_pool_destroy() {
    pthread_mutex_lock(&(pool->lock));
    pool->shutdown = true;
    pthread_cond_broadcast(&(pool->notify));
    pthread_mutex_unlock(&(pool->lock));

    for (int i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    free(pool->threads);
    pthread_mutex_destroy(&(pool->lock));
    pthread_cond_destroy(&(pool->notify));
    free(pool);
    printf("[WorkerPool] Destroyed.\n");
}