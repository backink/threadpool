#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "threadpool.h"

#define THREAD_NUM 32
#define TASK_NUM 256

pthread_mutex_t mutex;
int done = 0;
int tasks = 0;

void *task(void *args) {
    usleep(10000);
    pthread_mutex_lock(&mutex);
    done++;
    pthread_mutex_unlock(&mutex);
}

int main() {

    threadpool_t *pool = threadpool_create(THREAD_NUM, TASK_NUM);
    if (pool == NULL)
        return THREADPOOL_NULL;

    pthread_mutex_init(&mutex, NULL);

    while (threadpool_add(pool, &task, NULL) == 0) {
        tasks++;
    }
    
    while((tasks / 2) > done) {
        usleep(10000);
    }
    printf("Add tasks : %d\nDone tasks : %d\n", tasks, done);
    threadpool_destroy(pool);
    printf("Add tasks : %d\nDone tasks : %d\n", tasks, done);

    return 0;
}
