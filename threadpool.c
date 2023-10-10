#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include "threadpool.h"

typedef struct task {
    void *(*func) (void *);
    void *args;
} task_t;


struct threadpool {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pthread_t *threads;
    task_t *queue;
    int thread_count;
    int task_count;
    int qsize;
    int head;
    int tail;
    int shutdown;
};


static void *thread_pick_task(void *arg);

static void threadpool_free(threadpool_t *pool);

threadpool_t *threadpool_create(int thread_count, int qsize) {
    threadpool_t *pool;

    pool = malloc(sizeof(threadpool_t));
    if (pool == NULL)
        goto release;

    pool->threads = malloc(thread_count * sizeof(pthread_t));
    if (pool->threads == NULL)
        goto release;

    pool->queue = malloc(qsize * sizeof(task_t));
    if (pool->queue == NULL)
        goto release;
    
    pool->qsize = qsize;
    pool->head = 0;
    pool->tail = 0;
    pool->shutdown = 0;

    if (pthread_mutex_init(&pool->mutex, NULL) || pthread_cond_init(&pool->cond, NULL))
        goto release;
    
    
    for (int i = 0; i < thread_count; i++) {
        if (pthread_create(&pool->threads[i], NULL, thread_pick_task, pool) == 0) {
            pool->thread_count++;
        }
    }
    
    return pool;

release:
    threadpool_free(pool);
    return NULL;
}

int threadpool_add(threadpool_t *pool, void *(*func)(void*), void *args) {
    
    int err = 0;

    if (pool == NULL)
        return THREADPOOL_NULL;
    if (func == NULL)
        return THREADPOOL_PARA_ERROR;

    if (pthread_mutex_lock(&pool->mutex))
        return THREADPOOL_LOCK_FAIL;

    do {
        if (pool->task_count >= pool->qsize) {
            err = THREADPOOL_QFULL;
            break;
        }

        pool->queue[pool->tail].func = func;
        pool->queue[pool->tail].args = args;
        pool->tail = (pool->tail + 1) % pool->qsize;
        pool->task_count++;
        pthread_cond_signal(&pool->cond);

    } while(0);
    
    pthread_mutex_unlock(&pool->mutex);
    return err;
}


static void *thread_pick_task(void *arg) {
    threadpool_t *pool = (threadpool_t *) arg;
    task_t task;

    for (;;) {
        if (pthread_mutex_lock(&pool->mutex))
            return NULL;

        while (pool->task_count == 0 && pool->shutdown == 0) {
            pthread_cond_wait(&pool->cond, &pool->mutex);
        }

        if (pool->shutdown)
            break;

        task = pool->queue[pool->head];
        pool->head = (pool->head + 1) % (pool->qsize);
        pool->task_count--;

        pthread_mutex_unlock(&pool->mutex);

        (*task.func)(task.args);
    }

    pthread_mutex_unlock(&pool->mutex);
    return NULL;
}

static void threadpool_free(threadpool_t *pool) {
    if (pool) {
        if (pool->threads)
            free(pool->threads);
        if (pool->queue)
            free(pool->queue);
        
        free(pool);
    }
}

void threadpool_destroy(threadpool_t *pool) {
    if (pool) {
        pthread_mutex_lock(&pool->mutex);

        pool->shutdown = 1;

        pthread_cond_broadcast(&pool->cond);
        pthread_mutex_unlock(&pool->mutex);


        for (int i = 0; i < pool->thread_count; i++) {
            pthread_join(pool->threads[i], NULL);
        }

        threadpool_free(pool);
    }
}
