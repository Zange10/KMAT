#include "thread_pool.h"
#include <stdlib.h>
#include <stdio.h>

pthread_t threads64[64];

struct Thread_Pool use_thread_pool64(void *thread_method(void*), void *thread_args) {
    struct Thread_Pool thread_pool = {threads64, 64};

    for(int i = 0; i < 64; i++) {
        if (pthread_create(&thread_pool.threads[i], NULL, thread_method, &thread_args) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    return thread_pool;
}


void join_thread_pool(struct Thread_Pool thread_pool) {
    for(int i = 0; i < thread_pool.size; i++) {
        pthread_join(thread_pool.threads[i], NULL);
    }
}