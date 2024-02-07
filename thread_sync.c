#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "thread_sync.h"
#include "Glued-Doubly-Linked-List/glthreads.h"

synched_thread *
synched_thread_create(synched_thread *sync_thread,
		      uintptr_t thread_id, char *name, pthread_t *handler){

    if (!sync_thread){
	if ((sync_thread = calloc(1, sizeof(synched_thread))) == NULL){
	    perror("calloc");
	    exit(-1);
	}
    }

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
synched_thread_key_match_cb(void *sync_thread, void *thread_id){
    return 0;
}

static int
synched_thread_compare_cb(void *sync_thread1, void *sync_thread2){
    return 0;
}

static void
synched_thread_free_cb(void **lists, void *sync_thread){
    return;
}

synched_thread_pool *
synched_thread_pool_init(){
    synched_thread_pool *sth_pool;

    if ((sth_pool = (synched_thread_pool *) malloc(sizeof(synched_thread_pool))) == NULL){
	perror("malloc");
	exit(-1);
    }

    pthread_mutex_init(&sth_pool->mutex, NULL);
    sth_pool->pool_head = glthread_create_list(synched_thread_key_match_cb,
					       synched_thread_compare_cb,
					       synched_thread_free_cb,
					       offsetof(synched_thread, glue));
    return sth_pool;
}

void
synched_thread_insert_new_thread(synched_thread_pool *sth_pool,
				 glthread_node *node){
    synched_thread *sync_thread;
    if (!sth_pool || !node)
	return;

    pthread_mutex_lock(&sth_pool->mutex);
    sync_thread = (synched_thread *) glthread_get_app_structure(sth_pool->pool_head,
								node);
    assert(IS_BIT_SET(sync_thread->flags, THREAD_CREATED) == 1);
    assert(sync_thread->thread_fn == NULL);

    glthread_insert_entry(sth_pool->pool_head, node);
    pthread_mutex_unlock(&sth_pool->mutex);
}

glthread_node *
synched_thread_get_thread(synched_thread_pool *sth_pool){
    glthread_node *node;

    pthread_mutex_lock(&sth_pool->mutex);
    /*
     * Thread pool must ensure it has enough threads allocation to serve
     * the all application requests. If this is violated, raise an assertion
     * failures.
     */
    assert((node = glthread_get_first_entry(sth_pool->pool_head)) != NULL);
    pthread_mutex_unlock(&sth_pool->mutex);

    return node;
}

void
synched_thread_dispatch_thread(synched_thread_pool *sth_pool,
			       void *(*thread_fn)(void *),
			       void *arg){
    glthread_node *node;
    synched_thread *sync_thread;

    assert(!sth_pool || !thread_fn);

    node = synched_thread_get_thread(sth_pool);
    sync_thread = (synched_thread *) glthread_get_app_structure(sth_pool->pool_head,
								node);
    synched_thread_run(sync_thread, thread_fn, arg);
}
