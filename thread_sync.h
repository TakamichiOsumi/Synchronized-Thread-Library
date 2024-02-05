#ifndef __THREAD_SYNC__
#define __THREAD_SYNC__

#include <stdbool.h>
#include <pthread.h>

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

#define THREAD_RUNNING (1 << 0)
#define THREAD_MARKED_FOR_PAUSE (1 << 1)
#define THREAD_PAUSED (1 << 2)
#define THREAD_BLOCKED (1 << 3)

typedef struct synched_thread {

    char name[32];
    bool thread_created;
    pthread_t thread;
    pthread_attr_t attributes;

    /* Indicate thread status */
    uint32_t flags;

    /* Invoked before pausing the thread */
    void *(*thread_pause_fn)(void *);
    void *pause_arg;

    /* Main thread function */
    void *(*thread_fn)(void *);
    void *arg;

    /* Protect any update of the struct member */
    pthread_mutex_t state_mutex;
    pthread_cond_t state_cv;

} synched_thread;


synched_thread *synched_thread_create(synched_thread *sthread, char *name);

void synched_thread_set_thread_attribute(synched_thread *sthread, bool joinable);
void synched_thread_set_pause_fn(synched_thread *sthread,
				 void *(*thread_pause_fn)(void *),
				 void *pause_arg);

void synched_thread_run(synched_thread *sthread, void *(*thread_fn)(void *), void *arg);
/* Mark the flag to pause only */
void synched_thread_pause(synched_thread *sthread);
void synched_thread_test_and_pause(synched_thread *sthread);
void synched_thread_resume(synched_thread *sthread);
/*
 * Allow user to register a new function the thread will invoke after the pause.
 * It's possible that after the wake up, the involvement of the application changes
 * drastically.
 */
void synched_thread_set_pause_fn(synched_thread *sthread,
				 void *(*thread_pause_fn)(void *),
				 void *pause_arg);
#endif
