#include "thread.h"

int thread_mutex_create(pthread_mutex_t *mtx)
{
    int err;
    pthread_mutexattr_t attr;

    err = pthread_mutexattr_init(&attr);
    if (err != 0)
    {
        fprintf(stderr, "pthread_mutexattr_init() failed, reason: %s\n", strerror(errno));
        return ERROR;
    }
    // PTHREAD_MUTEX_ERRORCHECK是一个检错锁,如果一个线程请求同一个锁,就会死锁,加了这个属性,不会死锁,会出现一个报错
    err = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    if (err != 0)
    {
        fprintf(stderr, "pthread_mutexattr_settype(PTHREAD_MUTEX_ERRORCHECK) failed, reason: %s\n", strerror(errno));
        return ERROR;
    }

    err = pthread_mutex_init(mtx, &attr); // 初始化锁
    if (err != 0)
    {
        fprintf(stderr, "pthread_mutex_init() failed, reason: %s\n", strerror(errno));
        return ERROR;
    }

    err = pthread_mutexattr_destroy(&attr); // 因为PTHREAD_MUTEX_ERRORCHECK属性被带到mtx中了,所以就不需要attr了
    if (err != 0)
    {
        fprintf(stderr, "pthread_mutexattr_destroy() failed, reason: %s\n", strerror(errno));
    }

    return OK;
}

int thread_mutex_destroy(pthread_mutex_t *mtx)
{
    int err;

    err = pthread_mutex_destroy(mtx);
    if (err != 0)
    {
        fprintf(stderr, "pthread_mutex_destroy() failed, reason: %s\n", strerror(errno));
        return ERROR;
    }

    return OK;
}

int thread_mutex_lock(pthread_mutex_t *mtx)
{
    int err;

    err = pthread_mutex_lock(mtx);
    if (err == 0)
    {
        return OK;
    }
    fprintf(stderr, "pthread_mutex_lock() failed, reason: %s\n", strerror(errno));

    return ERROR;
}

int thread_mutex_unlock(pthread_mutex_t *mtx)
{
    int err;

    err = pthread_mutex_unlock(mtx);

#if 0
    ngx_time_update();
#endif

    if (err == 0)
    {
        return OK;
    }

    fprintf(stderr, "pthread_mutex_unlock() failed, reason: %s\n", strerror(errno));
    return ERROR;
}
