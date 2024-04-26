#include "thread_pool.h"
#include <stdlib.h>
#include <stdio.h>

pthread_t threads64[64];

#define NUM_COUNTER 4

int counter[NUM_COUNTER];
pthread_mutex_t counter_lock[NUM_COUNTER];

struct Thread_Pool use_thread_pool64(void *thread_method(void*), void *thread_args) {
    size_t size = 64;
    struct Thread_Pool thread_pool = {threads64, size};

	for(int i = 0; i < NUM_COUNTER; i++) counter[i] = 0;

    for(int i = 0; i < size; i++) {
        if (pthread_create(&thread_pool.threads[i], NULL, thread_method, thread_args) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    return thread_pool;
}

int get_thread_counter(int counter_index) {
	return counter[counter_index];
}

int get_incr_thread_counter(int counter_index) {
	int counter_value;
	pthread_mutex_lock(&counter_lock[counter_index]);
	counter_value = counter[counter_index];
	counter[counter_index]++;
	pthread_mutex_unlock(&counter_lock[counter_index]);
	return counter_value;
}

void join_thread_pool(struct Thread_Pool thread_pool) {
    for(int i = 0; i < thread_pool.size; i++) {
        pthread_join(thread_pool.threads[i], NULL);
    }
}