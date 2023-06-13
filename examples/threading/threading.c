#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    struct thread_data *td = (struct thread_data *)thread_param;
    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    
    /* Wait to obtain & obtain */
    usleep(td->w_obtain_ms);
    pthread_mutex_lock(td->mutex);

    td->thread_complete_success = true;

    /* Wait to release & release */
    usleep(td->w_obtain_ms);
    pthread_mutex_unlock(td->mutex);

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    struct thread_data *td;
    int ret;
   
    /* Allocate thread_data */
    td = malloc(sizeof(struct thread_data));
    if (!td)
        goto ret_fail;
   
    /* Initialize thread_data structure */
    td->mutex = mutex;
    td->w_obtain_ms = wait_to_obtain_ms;
    td->w_release_ms = wait_to_release_ms;
    td->thread_complete_success = false;
    
    ret = pthread_create(thread, NULL, threadfunc, td);
    if (ret != 0)
        goto ret_free;

    return true;

ret_free:
    free(td);
ret_fail:
    return false;
}

