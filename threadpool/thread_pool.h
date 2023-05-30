#ifndef _THREAD_POOL_H_INCLUDED_
#define _THREAD_POOL_H_INCLUDED_

#include "thread.h"

#define DEFAULT_THREADS_NUM 4   // 我们默认的线程数是4 ,一般是你是几核的你就搞几个线程,最多不多于核数的2倍
#define DEFAULT_QUEUE_NUM 65535 // 任务对列最大能容纳的数量

typedef unsigned long atomic_uint_t; // 通常用于实现原子操作，即在多线程或多进程环境中对变量进行原子性的读取和写入，以避免竞态条件和数据不一致的问题。
// atomic_uint_t: 实现线程池中的计数器，用于统计线程池中当前空闲的线程数、正在运行的任务数等信息。由于线程池中的计数器会被多个线程同时访问，因此需要使用原子操作保证计数器的正确性，避免出现竞态条件和数据不一致的问题。
typedef struct thread_task_s thread_task_t; // 线程任务
typedef struct thread_pool_s thread_pool_t; // 线程池

struct thread_task_s // 任务的定义,任务结构体
{
    thread_task_t *next;         // 指向下一个任务的指针，用于构成任务队列。
    uint_t id;                   // 任务的唯一标识符，用于在日志中标识不同的任务。
    void *ctx;                   // 上下文.任务的上下文信息，可以是任何类型的数据，用于向任务处理函数传递参数。
    void (*handler)(void *data); // 具体任务怎么执行的函数.handler：任务处理函数的指针，指向一个函数，该函数接受一个 void* 类型的参数，表示任务上下文信息，用于执行具体的任务。
};

typedef struct // 单链表的结构
{
    thread_task_t *first;
    thread_task_t **last; // 二级指针:指向最后一个节点的地址的地址
} thread_pool_queue_t;    // 任务对列的定义

#define thread_pool_queue_init(q) \
    (q)->first = NULL;            \
    (q)->last = &(q)->first // 将thread_pool_queue_init(q);替换为 : (q)->first = NULL; (q)->last = &(q)->first;

struct thread_pool_s
{
    pthread_mutex_t mtx;       // 互斥锁，用于保护线程池中的共享数据。
    thread_pool_queue_t queue; // 任务队列，用于存储待执行的任务。
    int_t waiting;             // 任务队列中等待执行的任务数。
    pthread_cond_t cond;       // 条件变量，用于线程间的同步。

    char *name;      // 线程池的名称。
    uint_t threads;  // 线程池中的线程数。
    int_t max_queue; // 任务队列的最大长度。
};

thread_task_t *thread_task_alloc(size_t size);
int_t thread_task_post(thread_pool_t *tp, thread_task_t *task);
thread_pool_t *thread_pool_init();
void thread_pool_destroy(thread_pool_t *tp);

#endif /* _THREAD_POOL_H_INCLUDED_ */
