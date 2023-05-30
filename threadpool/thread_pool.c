#include "thread_pool.h"

static void thread_pool_exit_handler(void *data);
static void *thread_pool_cycle(void *data);
static int_t thread_pool_init_default(thread_pool_t *tpp, char *name);

static uint_t thread_pool_task_id;

static int debug = 0;

thread_pool_t *thread_pool_init()
{
    int err;
    pthread_t tid;
    uint_t n;
    pthread_attr_t attr;
    thread_pool_t *tp = NULL;

    tp = calloc(1, sizeof(thread_pool_t)); // 在这里，nmemb 的值为 1，表示要分配一个连续的空间；size 的值为 sizeof(thread_pool_t)，表示每个空间的大小为 thread_pool_t 结构体的大小。
    // calloc 函数会在内存中分配 nmemb * size 个字节的连续空间，并将每个字节都初始化为 0。
    if (tp == NULL)
    {
        fprintf(stderr, "thread_pool_init: calloc failed!\n");
        return NULL;
    }

    thread_pool_init_default(tp, NULL); // 线程池默认的初始化

    thread_pool_queue_init(&tp->queue); // 使用宏函数初始化队列

    // 创建互斥量和条件变量
    if (thread_mutex_create(&tp->mtx) != OK)
    {
        free(tp);
        return NULL;
    }

    if (thread_cond_create(&tp->cond) != OK)
    {
        (void)thread_mutex_destroy(&tp->mtx);
        free(tp);
        return NULL;
    }

    // 线程启动时,我们可以指定一些属性
    err = pthread_attr_init(&attr);
    if (err)
    {
        fprintf(stderr, "pthread_attr_init() failed, reason: %s\n", strerror(errno));
        free(tp);
        return NULL;
    }
    // PTHREAD_CREATE_DETACHED:在线程创建时将其属性设为分离状态(detached),主线程是无法使用pthread_join等待到结束的子线程
    err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (err)
    {
        fprintf(stderr, "pthread_attr_setdetachstate() failed, reason: %s\n", strerror(errno));
        free(tp);
        return NULL;
    }

    for (n = 0; n < tp->threads; n++)
    {
        err = pthread_create(&tid, &attr, thread_pool_cycle, tp); // 创建线程
        if (err)
        {
            fprintf(stderr, "pthread_create() failed, reason: %s\n", strerror(errno));
            free(tp);
            return NULL;
        }
    }

    (void)pthread_attr_destroy(&attr); // 用于销毁线程属性对象。具体来说，该语句调用了 POSIX 线程库中的 pthread_attr_destroy 函数

    return tp;
}

// 线程池销毁
void thread_pool_destroy(thread_pool_t *tp)
{
    uint_t n;
    thread_task_t task; // 首先，创建一个特殊的任务结构体 task，并将其上下文信息设置为一个特殊的标志变量 lock。该任务结构体的处理函数为 thread_pool_exit_handler，用于通知工作线程自行退出并销毁自身。
    volatile uint_t lock;

    memset(&task, '\0', sizeof(thread_task_t));

    task.handler = thread_pool_exit_handler; // 给线程分配任务,这个任务就是自杀,销毁线程
    task.ctx = (void *)&lock;

    // 然后，遍历线程池中的每个工作线程，向其发送一个特殊任务 task，并将 lock 设置为 1。如果任务发送失败，直接返回。
    for (n = 0; n < tp->threads; n++) // 看有多少个线程
    {
        lock = 1;

        if (thread_task_post(tp, &task) != OK)
        {
            return;
        }

        // 接下来，等待工作线程将 lock设置为 0，表示工作线程已经成功退出并销毁自身。在等待期间，使用 sched_yield() 函数将 CPU 时间片让给其他线程，以避免 CPU 占用率过高。
        while (lock) // lock没有变成0,就调用sched_yield();
        {
            sched_yield();
        }

        // task.event.active = 0;
    }
    // 最后，销毁线程池中的互斥锁和条件变量，并释放线程池结构体所占用的内存。
    (void)thread_cond_destroy(&tp->cond);
    (void)thread_mutex_destroy(&tp->mtx);

    free(tp);
}

static void
thread_pool_exit_handler(void *data)
{
    uint_t *lock = data;

    *lock = 0;

    pthread_exit(0);
}

thread_task_t *
thread_task_alloc(size_t size)
{
    thread_task_t *task;

    task = calloc(1, sizeof(thread_task_t) + size); // calloc在malloc的基础上多了一步清空
    if (task == NULL)
    {
        return NULL;
    }

    task->ctx = task + 1;

    return task;
}

int_t thread_task_post(thread_pool_t *tp, thread_task_t *task)
{
    // 上锁
    if (thread_mutex_lock(&tp->mtx) != OK) // 注意&tp->mtx 和 &(tp->mtx) 是等价的,是tp先取到互斥锁,然后再取地址
    {
        return ERROR;
    }
    // 队列已经满了
    if (tp->waiting >= tp->max_queue)
    {
        (void)thread_mutex_unlock(&tp->mtx); // 解锁

        fprintf(stderr, "thread pool \"%s\" queue overflow: %ld tasks waiting\n",
                tp->name, tp->waiting);
        return ERROR;
    }

    // task->event.active = 1;
    // 能到这,说明队列还没满了
    task->id = thread_pool_task_id++;
    task->next = NULL;

    if (thread_cond_signal(&tp->cond) != OK) // 发送信号,唤醒一个条件变量
    {
        (void)thread_mutex_unlock(&tp->mtx);
        return ERROR;
    }

    *tp->queue.last = task;       //*tp->queue.last = task 和 *(tp->queue.last) = task 是等价的
    tp->queue.last = &task->next; //&task->next 和 &(task->next) 是等价的

    tp->waiting++;

    (void)thread_mutex_unlock(&tp->mtx);

    if (debug)
        fprintf(stderr, "task #%lu added to thread pool \"%s\"\n",
                task->id, tp->name);

    return OK;
}

// 这个函数是线程池中每个工作线程的主函数，用于执行任务队列中的任务。具体来说，该函数会循环等待任务队列中的任务，一旦有任务可执行，就会从队列中取出该任务，并调用该任务的处理函数来执行任务。
static void *
thread_pool_cycle(void *data)
{
    thread_pool_t *tp = data;

    int err;
    thread_task_t *task;

    if (debug)
        fprintf(stderr, "thread in pool \"%s\" started\n", tp->name);

    for (;;)
    {
        if (thread_mutex_lock(&tp->mtx) != OK) // 首先尝试上锁
        {
            return NULL;
        }

        tp->waiting--; // 一旦拿到了锁,任务队列中等待执行的任务数-1

        while (tp->queue.first == NULL) // 轮到我做了,但是任务对列是空的
        {
            if (thread_cond_wait(&tp->cond, &tp->mtx) != OK) // 在这里挂起
            {
                (void)thread_mutex_unlock(&tp->mtx);
                return NULL;
            }
        }
        // 能到这,说明任务队列有任务了
        task = tp->queue.first;
        tp->queue.first = task->next;

        if (tp->queue.first == NULL)
        {
            tp->queue.last = &tp->queue.first;
        }

        if (thread_mutex_unlock(&tp->mtx) != OK) // 解锁
        {
            return NULL;
        }

        if (debug)
            fprintf(stderr, "run task #%lu in thread pool \"%s\"\n",
                    task->id, tp->name);

        task->handler(task->ctx); // 表示执行当前任务的处理函数，并将任务的上下文信息作为参数传递给该函数

        if (debug)
            fprintf(stderr, "complete task #%lu in thread pool \"%s\"\n", task->id, tp->name);

        task->next = NULL;
        // 释放task
        free(task);
        task = NULL;
        //  notify
    }
}

static int_t
thread_pool_init_default(thread_pool_t *tpp, char *name)
{
    if (tpp)
    {
        tpp->threads = DEFAULT_THREADS_NUM; // 默认的线程数
        tpp->max_queue = DEFAULT_QUEUE_NUM; // 默认的队列数量

        tpp->name = strdup(name ? name : "default"); // 线程池的名字,如果不指定的话,默认是default
        if (debug)
            fprintf(stderr,
                    "thread_pool_init, name: %s ,threads: %lu max_queue: %ld\n",
                    tpp->name, tpp->threads, tpp->max_queue);

        return OK;
    }

    return ERROR;
}
