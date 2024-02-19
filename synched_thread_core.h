#ifndef __THREAD_SYNC__
#define __THREAD_SYNC__

#include <stdbool.h>
#include <pthread.h>
#include "Glued-Doubly-Linked-List/glthreads.h"

#define SET_BIT(n, BIT_F)			\
    (n |= BIT_F)
#define UNSET_BIT(n, BIT_F) \
    (n &= ~BIT_F)
#define IS_BIT_SET(n, BIT_F) \
    (n & BIT_F)
/* Use TOGGLE_BIT as a stmnt, not as a expr */
#define TOGGLE_BIT(n, BIT_F)                    \
    if (IS_BIT_SET(n, BIT_F)){			\
	UNSET_BIT(n, BIT_F);			\
    }else{					\
	SET_BIT(n, BIT_F);			\
    }

#define THREAD_CREATED (1 << 0)
#define THREAD_RUNNING (1 << 1)
#define THREAD_MARKED_FOR_PAUSE (1 << 2)
#define THREAD_PAUSED (1 << 3)
#define THREAD_BLOCKED (1 << 4)

struct synched_thread_pool;

typedef struct synched_thread {

    /* Basic information about the pthread itself */
    uintptr_t thread_id;
    pthread_t *thread;
    pthread_attr_t attributes;

    /* Indicate the thread status */
    bool thread_launched;
    uint32_t flags;

    /*
     * -------------------------------------
     * Variables to pause and resume threads
     * -------------------------------------
     */

    /* Invoked before the pausing */
    void *(*thread_pause_fn)(void *);
    void *pause_arg;

    /* Main thread function */
    void *(*thread_fn)(void *);
    void *arg;

    /* Invoked after the resumption */
    void *(*thread_resume_fn)(void *);
    void *resume_arg;

    /*
     * -------------------------------------
     * Variables to pool threads
     * -------------------------------------
     */
    void *(*deferred_main_fn)(void *);
    void *deferred_main_arg;
    struct synched_thread_pool *thread_pool;

    /* Protect any update of this struct member */
    pthread_mutex_t state_mutex;
    pthread_cond_t state_cv;

    /* Glue to be connected to the thread pool */
    glthread_node glue;

} synched_thread;

/* Thread pool definition */
typedef struct synched_thread_pool {
    uintptr_t max_threads_num;
    gldll *glued_list_container;
    pthread_mutex_t mutex;
} synched_thread_pool;

/* Thread barrier definition */
typedef struct synched_thread_barrier {
    uint32_t threshold;
    uint32_t curr_wait_count;
    pthread_cond_t cv;
    pthread_mutex_t mutex;
    pthread_cond_t busy_cv;
    bool releasing_barriered_threads;
} synched_thread_barrier;

/* Wait Queue */
typedef struct synched_thread_wait_queue {
    uintptr_t thread_wait_count;
    /* CV on which multiple threads in wait-queue are blocked */
    pthread_cond_t cv;
    /* Application owned mutex cached by wait-queue */
    pthread_mutex_t *app_mutex;
} synched_thread_wait_queue;

synched_thread *synched_thread_gen_empty_instance(synched_thread *sync_thread,
						  uintptr_t thread_id, pthread_t *handler);
void synched_thread_set_thread_attribute(synched_thread *sync_thread,
					 bool joinable);
void synched_thread_run(synched_thread *sync_thread,
			void *(*thread_fn)(void *), void *arg);
/*
 * Only mark the flag to pause.
 *
 * The pause operation will be done in the next pause point where
 * the thread doesn't create any invariants and can wait gracefully.
 */
void synched_thread_pause(synched_thread *sync_thread);

/* If thread is set to THREAD_MARKED_FOR_PAUSE state, start to wait */
void synched_thread_reached_pause_point(synched_thread *sync_thread);
void synched_thread_wake_up(synched_thread *sync_thread);

/*
 * Allow user to register functions synched thread will invoke before and
 * after the pause.
 *
 * It's possible that before the pause, the thread wants to process something
 * and after the wake up, the involvement of the application changes a lot.
 */
void synched_thread_set_pause_fn(synched_thread *sync_thread,
				 void *(*thread_pause_fn)(void *),
				 void *pause_arg);
void synched_thread_set_resume_fn(synched_thread *sync_thread,
				  void *(*thread_resume_fn)(void *),
				  void *resume_arg);

/* Support thread pool functionality */
synched_thread_pool *synched_thread_pool_init(uintptr_t max_threads_num);
void synched_thread_insert_thread_into_pool(synched_thread_pool *sth_pool,
					    synched_thread *thread);
void synched_thread_pool_thread();
glthread_node *synched_thread_get_thread(synched_thread_pool *sth_pool);
void synched_thread_dispatch_thread(synched_thread_pool *sth_pool,
				    void *(*thread_fn)(void *),
				    void *arg);

/* Support thread barrier functionality */
synched_thread_barrier *synched_thread_barrier_init(uint32_t threshold);
void synched_thread_barrier_wait(synched_thread_barrier *synched_barrier);
void synched_thread_barrier_destroy(synched_thread_barrier *synched_barrier);

/* Support wait queue functionality */
typedef bool (*synched_thread_wait_queue_cond_fn)(void *app_arg,
						  pthread_mutex_t *out_mutex);
synched_thread_wait_queue *synched_thread_wait_queue_init(void);
void synched_thread_wait_queue_test_and_wait(synched_thread_wait_queue *wq,
					     synched_thread_wait_queue_cond_fn cond_fn,
					     void *arg);
void synched_thread_wait_queue_signal(synched_thread_wait_queue *wq,
				      bool lock_mutex, bool broadcast);
void synched_thread_wait_queue_destroy(synched_thread_wait_queue *wq);

#endif
