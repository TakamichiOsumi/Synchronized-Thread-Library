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

typedef struct synched_thread {

    uintptr_t thread_id;
    char name[32];
    pthread_t *thread;
    pthread_attr_t attributes;

    bool thread_launched;

    /* Indicate thread status */
    uint32_t flags;

    /* Invoked before the pausing */
    void *(*thread_pause_fn)(void *);
    void *pause_arg;

    /* Main thread function */
    void *(*thread_fn)(void *);
    void *arg;

    /* Invoked after the resumption */
    void *(*thread_resume_fn)(void *);
    void *resume_arg;

    /* Protect any update of this struct member */
    pthread_mutex_t state_mutex;
    pthread_cond_t state_cv;

    glthread_node glue;

} synched_thread;

/* Thread pool definition */
typedef struct synched_thread_pool {
    gldll *pool_head;
    pthread_mutex_t mutex;
} synched_thread_pool;

synched_thread *synched_thread_create(synched_thread *sync_thread,
				      uintptr_t thread_id, char *name, pthread_t *handler);
void synched_thread_set_thread_attribute(synched_thread *sync_thread, bool joinable);
void synched_thread_run(synched_thread *sthread,
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
 * Allow user to register functions synched thread will invoke before and after the pause.
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
synched_thread_pool *synched_thread_pool_init();
void synched_thread_insert_new_thread(synched_thread_pool *sth_pool,
				      glthread_node *node);
synched_thread *synched_thread_get_thread(synched_thread_pool *sth_pool);
void synched_thread_dispatch_thread(synched_thread_pool *sth_pool,
				    void *(*thread_fn)(void *),
				    void *arg);

#endif
