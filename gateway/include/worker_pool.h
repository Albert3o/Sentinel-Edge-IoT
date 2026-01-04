#ifndef WORKER_POOL_H
#define WORKER_POOL_H

#include <pthread.h>
#include <stdbool.h>

// 定义任务结构体
typedef struct Task {
    void (*function)(void *arg); // 函数指针
    void *arg;                   // 函数参数
    struct Task *next;
} Task;

// 线程池结构体
typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t  notify;
    pthread_t      *threads;
    Task           *queue_head;
    int             thread_count;
    int             queue_size;
    bool            shutdown;
} WorkerPool;

// 全职线程池接口
int worker_pool_init(int thread_count);
int worker_pool_add_task(void (*function)(void *), void *arg);
void worker_pool_destroy();

#endif