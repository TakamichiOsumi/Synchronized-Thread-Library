#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "thread_sync.h"
#include "Glued-Doubly-Linked-List/glthreads.h"

synched_thread *
synched_thread_gen_empty_instance(synched_thread *sync_thread,
				  uintptr_t thread_id, char *name,
				  pthread_t *handler){

    sync_thread->thread_id = thread_id;
    strncpy(sync_thread->name, name, sizeof(sync_thread->name));
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
	printf("[%p] Will wait for pthread_cond_signal()...\n",
	       pthread_self());

	pthread_mutex_lock(&thread->state_mutex);
	pthread_cond_wait(&thread->state_cv,
			  &thread->state_mutex);
	pthread_mutex_unlock(&thread->state_mutex);

	printf("[%p] Woke up...\n", pthread_self());

	if (thread->deferred_main_fn){
	    printf("[%p] Will execute the user request main function...\n",
		   pthread_self());
	    (thread->deferred_main_fn)(thread->deferred_main_arg);
	}

	printf("[%p] All works are done. Loop back to the beginning.\n",
	       pthread_self());

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

/* TODO */
static int
synched_thread_compare_cb(void *sync_thread1, void *sync_thread2){
    return 0;
}

static void
synched_thread_free_cb(void **lists, void *sync_thread){
    return;
}

synched_thread_pool *
synched_thread_pool_init(uintptr_t max_threads_num){
    synched_thread_pool *sth_pool;

    assert(max_threads_num >= 0);
    if ((sth_pool = (synched_thread_pool *) malloc(sizeof(synched_thread_pool))) == NULL){
	perror("malloc");
	exit(-1);
    }

    sth_pool->max_threads_num = max_threads_num;
    pthread_mutex_init(&sth_pool->mutex, NULL);
    sth_pool->glued_list_container = glthread_create_list(synched_thread_key_match_cb,
							  synched_thread_compare_cb,
							  synched_thread_free_cb,
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
