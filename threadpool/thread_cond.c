#include "thread.h"



int
thread_cond_create(pthread_cond_t *cond)
{
    int  err;

    err = pthread_cond_init(cond, NULL);
    if (err == 0) {
        return OK;
    }

    fprintf(stderr, "pthread_cond_init() failed, reason: %s\n",strerror(errno));
    return ERROR;
}


int
thread_cond_destroy(pthread_cond_t *cond)
{
    int  err;

    err = pthread_cond_destroy(cond);
    if (err == 0) {
        return OK;
    }
	
	fprintf(stderr, "pthread_cond_destroy() failed, reason: %s\n",strerror(errno));
    return ERROR;
}


int
thread_cond_signal(pthread_cond_t *cond)
{
    int  err;

    err = pthread_cond_signal(cond);
    if (err == 0) {
        return OK;
    }

	fprintf(stderr, "pthread_cond_signal() failed, reason: %s\n",strerror(errno));
    return ERROR;
}


int
thread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mtx)
{
    int  err;

    err = pthread_cond_wait(cond, mtx);


    if (err == 0) {
        return OK;
    }

	fprintf(stderr, "pthread_cond_wait() failed, reason: %s\n",strerror(errno));
    return ERROR;
}

