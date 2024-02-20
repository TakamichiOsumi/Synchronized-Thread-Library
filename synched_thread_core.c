#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "synched_thread_core.h"
#include "Glued-Doubly-Linked-List/glthreads.h"

static void *
smalloc(size_t size){
    void *p;

    if ((p = malloc(size)) == NULL){
	perror("malloc");
	exit(-1);
    }

    return p;
}

synched_thread *
synched_thread_gen_empty_instance(synched_thread *sync_thread,
				  uintptr_t thread_id, pthread_t *handler){

    sync_thread->thread_id = thread_id;
    sync_thread->thread = handler;
    pthread_attr_init(&sync_thread->attributes);

    sync_thread->thread_launched = false;
    sync_thread->flags = THREAD_CREATED;

    /* function triggered before the pause */
    sync_thread->thread_pause_fn = NULL;
    sync_thread->pause_arg = NULL;

    /* thread main function */
    sync_thread->thread_fn = NULL;
    sync_thread->arg = NULL;

    /* function triggered after the resume */
    sync_thread->thread_pause_fn = NULL;
    sync_thread->pause_arg = NULL;

    pthread_cond_init(&sync_thread->state_cv, NULL);
    pthread_mutex_init(&sync_thread->state_mutex, NULL);

    return sync_thread;
}

static void *
synched_thread_standby_run(void *arg){
    synched_thread *thread = (synched_thread *) arg;

    while(1){
	printf("[%lu : %p] Will wait for pthread_cond_signal()...\n",
	       thread->thread_id, pthread_self());

	pthread_mutex_lock(&thread->state_mutex);
	pthread_cond_wait(&thread->state_cv,
			  &thread->state_mutex);
	pthread_mutex_unlock(&thread->state_mutex);

	printf("[%lu : %p] Woke up...\n", thread->thread_id, pthread_self());

	if (thread->deferred_main_fn){
	    printf("[%lu : %p] Will execute the user request main function...\n",
		   thread->thread_id, pthread_self());
	    (thread->deferred_main_fn)(thread->deferred_main_arg);
	}

	printf("[%lu : %p] All works are done. Loop back to the beginning.\n",
	       thread->thread_id, pthread_self());

	/* Reset */
	thread->deferred_main_fn = NULL;
	thread->deferred_main_arg = NULL;

	/*
	 * This thread is already fetched from the thread pool.
	 *
	 * Add this thread to the waiting list again for reuse.
	 */
	pthread_mutex_lock(&thread->state_mutex);
	thread->glue.next = NULL;
	thread->glue.prev = NULL;
	glthread_insert_entry(thread->thread_pool->glued_list_container,
			      &thread->glue);
	pthread_mutex_unlock(&thread->state_mutex);
    }

    return NULL;
}

void
synched_thread_set_thread_attribute(synched_thread *sync_thread, bool joinable){
    pthread_attr_setdetachstate(&sync_thread->attributes,
				joinable ?
				PTHREAD_CREATE_JOINABLE : PTHREAD_CREATE_DETACHED);
}

void
synched_thread_set_pause_fn(synched_thread *sync_thread,
			    void *(*thread_pause_fn)(void *),
			    void *pause_arg){
    sync_thread->thread_pause_fn = thread_pause_fn;
    sync_thread->pause_arg = pause_arg;
}

void
synched_thread_set_resume_fn(synched_thread *sync_thread,
			    void *(*thread_resume_fn)(void *),
			    void *resume_arg){
    sync_thread->thread_resume_fn = thread_resume_fn;
    sync_thread->resume_arg = resume_arg;
}

void
synched_thread_run(synched_thread *sync_thread,
		   void *(*thread_fn)(void *), void *arg){

    assert(IS_BIT_SET(sync_thread->flags, THREAD_CREATED) == 1);

    sync_thread->thread_fn = thread_fn;
    sync_thread->arg = arg;
    sync_thread->thread_launched = true;

    SET_BIT(sync_thread->flags, THREAD_RUNNING);
    UNSET_BIT(sync_thread->flags, THREAD_CREATED);

    if (pthread_create(sync_thread->thread,
		       &sync_thread->attributes, thread_fn, arg) != 0){
	perror("pthread_create");
	exit(-1);
    }
}

void
synched_thread_pause(synched_thread *sync_thread){
    pthread_mutex_lock(&sync_thread->state_mutex);
    if (IS_BIT_SET(sync_thread->flags, THREAD_RUNNING)){
	UNSET_BIT(sync_thread->flags, THREAD_RUNNING);
	SET_BIT(sync_thread->flags, THREAD_MARKED_FOR_PAUSE);
    }
    pthread_mutex_unlock(&sync_thread->state_mutex);
}

void
synched_thread_wake_up(synched_thread *sync_thread){
    pthread_mutex_lock(&sync_thread->state_mutex);
    if (IS_BIT_SET(sync_thread->flags, THREAD_PAUSED)){
	pthread_cond_signal(&sync_thread->state_cv);
    }
    pthread_mutex_unlock(&sync_thread->state_mutex);
}

void
synched_thread_reached_pause_point(synched_thread *sync_thread){
    pthread_mutex_lock(&sync_thread->state_mutex);
    if (IS_BIT_SET(sync_thread->flags, THREAD_MARKED_FOR_PAUSE)){

	SET_BIT(sync_thread->flags, THREAD_PAUSED);
	UNSET_BIT(sync_thread->flags, THREAD_MARKED_FOR_PAUSE);
	UNSET_BIT(sync_thread->flags, THREAD_RUNNING);

	if (sync_thread->thread_pause_fn)
	    (sync_thread->thread_pause_fn)(sync_thread->pause_arg);

	pthread_cond_wait(&sync_thread->state_cv,
			  &sync_thread->state_mutex);

	SET_BIT(sync_thread->flags, THREAD_RUNNING);
	UNSET_BIT(sync_thread->flags, THREAD_PAUSED);

	if (sync_thread->thread_resume_fn)
	    (sync_thread->thread_resume_fn)(sync_thread->resume_arg);

    }
    pthread_mutex_unlock(&sync_thread->state_mutex);
}

static int
synched_thread_key_match_cb(void *thread, void *thread_id){
    synched_thread *sync_thread = (synched_thread *) thread;

    if (sync_thread->thread_id == (uintptr_t) thread_id)
	return 0;
    else
	return -1;
}

synched_thread_pool *
synched_thread_pool_init(uintptr_t max_threads_num){
    synched_thread_pool *sth_pool;

    assert(max_threads_num >= 0);
    sth_pool = smalloc(sizeof(synched_thread_pool));
    sth_pool->max_threads_num = max_threads_num;
    pthread_mutex_init(&sth_pool->mutex, NULL);
    sth_pool->glued_list_container = glthread_create_list(synched_thread_key_match_cb,
							  NULL, NULL,
							  offsetof(synched_thread, glue));
    return sth_pool;
}

void
synched_thread_insert_thread_into_pool(synched_thread_pool *sth_pool,
				       synched_thread *thread){
    assert(sth_pool != NULL);
    assert(thread != NULL);

    pthread_mutex_lock(&sth_pool->mutex);
    /* Ensure that this thread is just an empty instance of one thread */
    assert(IS_BIT_SET(thread->flags, THREAD_CREATED) == 1);
    assert(thread->thread_pause_fn == NULL);
    assert(thread->pause_arg == NULL);
    assert(thread->thread_fn == NULL);
    assert(thread->arg == NULL);
    assert(thread->thread_resume_fn == NULL);
    assert(thread->resume_arg == NULL);
    /* Insert the passed thead to the pool */
    glthread_insert_entry(sth_pool->glued_list_container,
			  &thread->glue);
    /*
     * Make the thread remember the thread pool address at the same time.
     * This pointer will be used in synched_thread_standby_run() to add
     * this thread to the thread pool again.
     */
    thread->thread_pool = sth_pool;

    pthread_mutex_unlock(&sth_pool->mutex);

    /*
     * Now, ready to execute the thread in the background.
     *
     * The third argument is the thread itself, but this data
     * will get updated in synched_thread_dispatch_thread() so that
     * it can respond to and process any new user requested functions
     * as callbacks. At this moment, just ensure that those are null.
     */
    assert(thread->deferred_main_fn == NULL);
    assert(thread->deferred_main_arg == NULL);
    synched_thread_run(thread, synched_thread_standby_run, thread);
}

glthread_node *
synched_thread_get_thread(synched_thread_pool *sth_pool){
    glthread_node *node;

    pthread_mutex_lock(&sth_pool->mutex);
    node = glthread_get_first_entry(sth_pool->glued_list_container);
    assert(node != NULL);
    pthread_mutex_unlock(&sth_pool->mutex);

    return node;
}

void
synched_thread_dispatch_thread(synched_thread_pool *sth_pool,
			       void *(*thread_fn)(void *),
			       void *arg){
    glthread_node *node;
    synched_thread *thread;

    node = synched_thread_get_thread(sth_pool);
    thread = (synched_thread *) glthread_get_app_structure(sth_pool->glued_list_container,
								node);
    thread->deferred_main_fn = thread_fn;
    thread->deferred_main_arg = arg;

    printf("Done with sending a signal to pooled thread\n");

    pthread_cond_signal(&thread->state_cv);
}

synched_thread_barrier *
synched_thread_barrier_init(uint32_t threshold){
    synched_thread_barrier *sync_barrier;

    assert(threshold > 0);

    sync_barrier = (synched_thread_barrier *) smalloc(sizeof(synched_thread_barrier));
    sync_barrier->threshold = threshold;
    sync_barrier->curr_wait_count = 0;
    pthread_cond_init(&sync_barrier->cv, NULL);
    pthread_mutex_init(&sync_barrier->mutex, NULL);
    pthread_cond_init(&sync_barrier->busy_cv, NULL);
    sync_barrier->releasing_barriered_threads = false;

    return sync_barrier;
}

/*
 * Thread barrier feature uses two condition variables, 'cv' and 'busy_cv'
 * defined in the synched_thread_barrier.
 *
 * 'cv' is utilized for the threads blocked by the barrier until the 'curr_wait_count'
 * reaches 'threshold'. The thread which arrived last starts to wake up other
 * threads with this condition variable.
 *
 * 'busy_cv' works to block any threads trying to enter the barrier, from the moment
 * when one thread reached the 'threshold', until the first waiting thread blocked by
 * the barrier gets released.
 */
void
synched_thread_barrier_wait(synched_thread_barrier *synched_barrier){
    pthread_mutex_lock(&synched_barrier->mutex);

    while (synched_barrier->releasing_barriered_threads == true){
	pthread_cond_wait(&synched_barrier->busy_cv,
			  &synched_barrier->mutex);
    }

    if (synched_barrier->curr_wait_count + 1 == synched_barrier->threshold){
	/* This thread will just wake up one thread and goes on its work on return */
	synched_barrier->releasing_barriered_threads = true;
	pthread_cond_signal(&synched_barrier->cv);
	pthread_mutex_unlock(&synched_barrier->mutex);
	return;
    }

    synched_barrier->curr_wait_count++;
    pthread_cond_wait(&synched_barrier->cv,
		      &synched_barrier->mutex);
    synched_barrier->curr_wait_count--;

    if (synched_barrier->curr_wait_count == 0){
	assert(!(synched_barrier < 0));
	/*
	 * If this is the first waiting thread blocked by the barrier,
	 * then there is no other threads which need to be woken up with
	 * 'cv' condition variable.
	 *
	 * Meanwhile, there can be some threads waiting outside
	 * the barrier with 'busy_cv'. Wake up them by pthread_cond_broadcast.
	 *
	 * All the process to release the barriered threads is done.
	 */
	synched_barrier->releasing_barriered_threads = false;
	pthread_cond_broadcast(&synched_barrier->busy_cv);
    }else{
	/* Wake up another thread blocked by this barrier consecutively */
	pthread_cond_signal(&synched_barrier->cv);
    }

    pthread_mutex_unlock(&synched_barrier->mutex);
}

void
synched_thread_barrier_destroy(synched_thread_barrier *synched_barrier){
    assert(synched_barrier->curr_wait_count == 0);
    assert(synched_barrier->releasing_barriered_threads == false);
    pthread_cond_destroy(&synched_barrier->cv);
    pthread_mutex_destroy(&synched_barrier->mutex);
    pthread_cond_destroy(&synched_barrier->busy_cv);
    free(synched_barrier);
}

synched_thread_wait_queue *
synched_thread_wait_queue_init(void){
    synched_thread_wait_queue *wq;

    wq = (synched_thread_wait_queue *) smalloc(sizeof(synched_thread_wait_queue));
    wq->thread_wait_count = 0;
    pthread_cond_init(&wq->cv, NULL);
    wq->app_mutex = (pthread_mutex_t *) smalloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(wq->app_mutex, NULL);

    return wq;
}

/*
 * The caller is responsible for unlocking the application's mutex which is locked
 * on return of this function, by synched_thread_wait_queue_unlock().
 */
void
synched_thread_wait_queue_test_and_wait(synched_thread_wait_queue *wq,
					synched_thread_wait_queue_cond_fn cond_fn,
					void *arg){
    bool should_block;
    pthread_mutex_t *locked_app_mutex = NULL;

    should_block = cond_fn(arg, &locked_app_mutex); /* locking mode */
    wq->app_mutex = locked_app_mutex;

    while(should_block){
	wq->thread_wait_count++;
	/*
	 * Monitor application specific condition variable which is
	 * signaled in synched_thread_wait_queue_signal.
	 */
	pthread_cond_wait(&wq->cv, wq->app_mutex);
	wq->thread_wait_count--;
	should_block = cond_fn(arg, NULL); /* non-locking mode */
    }
}

void
synched_thread_wait_queue_unlock(synched_thread_wait_queue *wq){
    pthread_mutex_unlock(wq->app_mutex);
}

/*
 * If the caller of this API has a lock on the mutex of wait queue already,
 * then 'lock_mutex' should be false to avoid taking the lock again.
 *
 * Send signals to waiting threads blocked by
 * synched_thread_wait_queue_test_and_wait, after the caller
 * change the status of interested application data.
 */
void
synched_thread_wait_queue_signal(synched_thread_wait_queue *wq,
				 bool lock_mutex, bool broadcast){
    if (!wq->app_mutex)
	return;

    if (lock_mutex)
	pthread_mutex_lock(wq->app_mutex);

    if (wq->thread_wait_count <= 0){
	if (lock_mutex)
	    pthread_mutex_unlock(wq->app_mutex);
	return;
    }

    if (broadcast)
	pthread_cond_broadcast(&wq->cv);
    else
	pthread_cond_signal(&wq->cv);

    if (lock_mutex)
	pthread_mutex_unlock(wq->app_mutex);
}

void
synched_thread_wait_queue_destroy(synched_thread_wait_queue *wq){
    pthread_cond_destroy(&wq->cv);
    wq->app_mutex = NULL;
}
