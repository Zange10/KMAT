#ifndef KSP_THREAD_POOL_H
#define KSP_THREAD_POOL_H

#include <pthread.h>

struct Thread_Pool {
    pthread_t *threads;
    size_t size;
};


struct Thread_Pool use_thread_pool64(void *thread_method(void*), void *thread_args);

int get_thread_counter();

void join_thread_pool(struct Thread_Pool thread_pool);


#endif //KSP_THREAD_POOL_H
