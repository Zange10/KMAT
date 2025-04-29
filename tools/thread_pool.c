#include "thread_pool.h"
#include <stdlib.h>
#include <stdio.h>

thread_t threads32[32];
thread_t threads01[1];

#define NUM_COUNTER 4

int counter[NUM_COUNTER];
thread_mutex_t counter_lock[NUM_COUNTER];

struct Thread_Pool use_thread_pool32(void *thread_method(void*), void *thread_args) {
	size_t size = 32;
	struct Thread_Pool thread_pool = {threads32, size};

	// Initialize counters
	for (int i = 0; i < NUM_COUNTER; i++) {
		counter[i] = 0;
	}

	// Initialize mutexes
	for (int i = 0; i < NUM_COUNTER; i++) {
		#ifdef _WIN32
				InitializeCriticalSection(&counter_lock[i]); // Windows mutex init
		#else
				pthread_mutex_init(&counter_lock[i], NULL); // Linux mutex init
		#endif
	}

	// Create threads
	for (int i = 0; i < size; i++) {
		#ifdef _WIN32
			thread_pool.threads[i] = CreateThread(
					NULL,                        // default security attributes
					0,                           // default stack size
					(LPTHREAD_START_ROUTINE) thread_method, // pointer to the thread function
					thread_args,                 // argument to thread function
					0,                           // default creation flags
					NULL);                       // thread ID (not needed in this case)

			if (thread_pool.threads[i] == NULL) {
				fprintf(stderr, "Error creating thread %d\n", i);
				exit(EXIT_FAILURE);
			}
		#else
			if (pthread_create(&thread_pool.threads[i], NULL, thread_method, thread_args) != 0) {
				perror("pthread_create");
				exit(EXIT_FAILURE);
			}
		#endif
	}

	return thread_pool;
}

struct Thread_Pool use_thread_pool01(void *thread_method(void*), void *thread_args) {
	size_t size = 1;
	struct Thread_Pool thread_pool = {threads01, size};

	// Initialize counters
	for (int i = 0; i < NUM_COUNTER; i++) {
		counter[i] = 0;
	}

	// Initialize mutexes
	for (int i = 0; i < NUM_COUNTER; i++) {
#ifdef _WIN32
		InitializeCriticalSection(&counter_lock[i]); // Windows mutex init
#else
		pthread_mutex_init(&counter_lock[i], NULL); // Linux mutex init
#endif
	}

	// Create threads
	for (int i = 0; i < size; i++) {
#ifdef _WIN32
		thread_pool.threads[i] = CreateThread(
				NULL,                        // default security attributes
				0,                           // default stack size
				(LPTHREAD_START_ROUTINE) thread_method, // pointer to the thread function
				thread_args,                 // argument to thread function
				0,                           // default creation flags
				NULL);                       // thread ID (not needed in this case)

		if (thread_pool.threads[i] == NULL) {
			fprintf(stderr, "Error creating thread %d\n", i);
			exit(EXIT_FAILURE);
		}
#else
		if (pthread_create(&thread_pool.threads[i], NULL, thread_method, thread_args) != 0) {
				perror("pthread_create");
				exit(EXIT_FAILURE);
			}
#endif
	}

	return thread_pool;
}

int get_thread_counter(int counter_index) {
	return counter[counter_index];
}

int get_incr_thread_counter(int counter_index) {
	int counter_value;
	#ifdef _WIN32
		EnterCriticalSection(&counter_lock[counter_index]); // Windows lock
	#else
		pthread_mutex_lock(&counter_lock[counter_index]); // Linux lock
	#endif
	counter_value = counter[counter_index];
	counter[counter_index]++;
	#ifdef _WIN32
		LeaveCriticalSection(&counter_lock[counter_index]); // Windows unlock
	#else
		pthread_mutex_unlock(&counter_lock[counter_index]); // Linux unlock
	#endif
	return counter_value;
}

void incr_thread_counter_by_amount(int counter_index, int amount) {
	#ifdef _WIN32
		EnterCriticalSection(&counter_lock[counter_index]); // Windows lock
	#else
		pthread_mutex_lock(&counter_lock[counter_index]); // Linux lock
	#endif
	counter[counter_index] += amount;
	#ifdef _WIN32
		LeaveCriticalSection(&counter_lock[counter_index]); // Windows unlock
	#else
		pthread_mutex_unlock(&counter_lock[counter_index]); // Linux unlock
	#endif
}

void join_thread_pool(struct Thread_Pool thread_pool) {
    for(int i = 0; i < thread_pool.size; i++) {
		#ifdef _WIN32
				WaitForSingleObject(thread_pool.threads[i], INFINITE); // Windows join
		#else
				pthread_join(thread_pool.threads[i], NULL); // Linux join
		#endif
    }
}