#define MAX_THREAD 64
#define MAX_QUEUE 256

typedef struct threadpool threadpool_t;

enum threadpool_error {
    THREADPOOL_NULL = -1,
    THREADPOOL_PARA_ERROR = -2,
    THREADPOOL_QFULL = -3,
    THREADPOOL_LOCK_FAIL = -4,
};

threadpool_t *threadpool_create(int thread_count, int qsize);

int threadpool_add(threadpool_t *threadpool, void *(*func)(void *), void *args);

void threadpool_destroy(threadpool_t *threadpool);

