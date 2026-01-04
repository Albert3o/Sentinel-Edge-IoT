#include <stdio.h>
#include <stdlib.h>
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
    pool->thread_count = thread_count;
    pool->queue_size = 0;
    pool->queue_head = NULL;
    pool->shutdown = false;

    pthread_mutex_init(&(pool->lock), NULL);
    pthread_cond_init(&(pool->notify), NULL);

    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);
    for (int i = 0; i < thread_count; i++) {
        pthread_create(&(pool->threads[i]), NULL, worker_thread, NULL);
    }

    printf("[WorkerPool] Initialized with %d threads.\n", thread_count);
    return 0;
}

int worker_pool_add_task(void (*function)(void *), void *arg) {
    pthread_mutex_lock(&(pool->lock));

    Task *new_task = (Task *)malloc(sizeof(Task));
    new_task->function = function;
    new_task->arg = arg;
    new_task->next = pool->queue_head;
    pool->queue_head = new_task;
    pool->queue_size++;

    pthread_cond_signal(&(pool->notify)); // 唤醒一个正在等待的线程
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