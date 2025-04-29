#ifndef KSP_THREAD_POOL_H
#define KSP_THREAD_POOL_H

#ifdef _WIN32
#include <windows.h>
typedef HANDLE thread_t;  // Use Windows HANDLE for threads
typedef CRITICAL_SECTION thread_mutex_t;  // Use Windows CRITICAL_SECTION for mutexes
#else
#include <pthread.h>
typedef pthread_t thread_t;  // Use pthread_t for Linux
typedef pthread_mutex_t thread_mutex_t;  // Use pthread_mutex_t for Linux
#endif


/**
 * @brief Represents a thread pool containing multiple threads and the number of threads in the pool
 */
struct Thread_Pool {
	thread_t *threads; /**< Array of threads in the pool */
	size_t size;        /**< Size of the thread pool */
};


/**
 * @brief Creates, initializes and runs a thread pool with 64 threads and sets counters to 0
 *
 * @param thread_method Pointer to the thread method function
 * @param thread_args   Pointer to the arguments for the thread method
 * @return A populated Thread_Pool structure with 64 threads
 */
struct Thread_Pool use_thread_pool32(void *thread_method(void*), void *thread_args);
struct Thread_Pool use_thread_pool01(void *thread_method(void*), void *thread_args);

/**
 * @brief Gets the current specified thread counter value
 *
 * @param counter_index The index of the specified thread counter
 * @return The current value of the thread counter
 */
int get_thread_counter(int counter_index);


/**
 * @brief Gets the current specified thread counter value and increases it by one
 *
 * @param counter_index The index of the specified thread counter
 * @return The current value of the thread counter
 */
int get_incr_thread_counter(int counter_index);


/**
 * @brief Waits for all threads in the specified thread pool to finish.
 *
 * @param thread_pool The thread pool structure to be joined.
 */


/**
 * @brief Increases specified thread counter by given amount
 *
 * @param counter_index The index of the specified thread counter
 * @param amount The amount the counter should be increased by
 */
void incr_thread_counter_by_amount(int counter_index, int amount);



#endif //KSP_THREAD_POOL_H
