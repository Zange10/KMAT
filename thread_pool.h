#ifndef KSP_THREAD_POOL_H
#define KSP_THREAD_POOL_H

#include <pthread.h>


/**
 * @brief Represents a thread pool containing multiple threads and the number of threads in the pool
 */
struct Thread_Pool {
	pthread_t *threads; /**< Array of threads in the pool */
	size_t size;        /**< Size of the thread pool */
};


/**
 * @brief Creates, initializes and runs a thread pool with 64 threads
 *
 * @param thread_method Pointer to the thread method function
 * @param thread_args   Pointer to the arguments for the thread method
 * @return A populated Thread_Pool structure with 64 threads
 */
struct Thread_Pool use_thread_pool64(void *thread_method(void*), void *thread_args);


/**
 * @brief Gets the current thread counter value and increases it by one
 *
 * @return The current value of the thread counter
 */
int get_thread_counter();


/**
 * @brief Waits for all threads in the specified thread pool to finish.
 *
 * @param thread_pool The thread pool structure to be joined.
 */
void join_thread_pool(struct Thread_Pool thread_pool);



#endif //KSP_THREAD_POOL_H
