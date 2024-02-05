
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "thread_sync.h"

synched_thread *
synched_thread_create(synched_thread *sthread, char *name){
    if (!sthread){
	sthread = calloc(1, sizeof(synched_thread));
    }

    strncpy(sthread->name, name, sizeof(sthread->name));
    sthread->thread_created = false;
    sthread->arg = NULL;
    sthread->thread_fn = NULL;
    pthread_attr_init(&sthread->attributes);

    return sthread;
}

void
synched_thread_set_thread_attribute(synched_thread *sthread, bool joinable){
    pthread_attr_setdetachstate(&sthread->attributes,
				joinable ?
				PTHREAD_CREATE_JOINABLE : PTHREAD_CREATE_DETACHED);
}

void
synched_thread_set_pause_fn(synched_thread *sthread,
			    void *(*thread_pause_fn)(void *),
			    void *pause_arg){
    sthread->thread_pause_fn = thread_pause_fn;
    sthread->pause_arg = pause_arg;
}

void
synched_thread_run(synched_thread *sthread, void *(*thread_fn)(void *), void *arg){
    sthread->thread_fn = thread_fn;
    sthread->arg = arg;
    sthread->thread_created = true;
    pthread_create(&sthread->thread,
		   &sthread->attributes,
		   thread_fn,
		   arg);
}

void
synched_thread_pause(synched_thread *sthread){
    pthread_mutex_lock(&sthread->state_mutex);
    if (IS_BIT_SET(sthread->flags, THREAD_RUNNING)){
	SET_BIT(sthread->flags, THREAD_MARKED_FOR_PAUSE);
    }
    pthread_mutex_unlock(&sthread->state_mutex);
}

void
synched_thread_test_and_pause(synched_thread *sthread){
    pthread_mutex_lock(&sthread->state_mutex);
    if (IS_BIT_SET(sthread->flags, THREAD_PAUSED)){
	pthread_cond_signal(&sthread->state_cv);
    }
    pthread_mutex_unlock(&sthread->state_mutex);
}

void
synched_thread_resume(synched_thread *sthread){
    pthread_mutex_lock(&sthread->state_mutex);

    if (IS_BIT_SET(sthread->flags, THREAD_MARKED_FOR_PAUSE)){
	SET_BIT(sthread->flags, THREAD_PAUSED);
	UNSET_BIT(sthread->flags, THREAD_MARKED_FOR_PAUSE);
	UNSET_BIT(sthread->flags, THREAD_RUNNING);
	pthread_cond_wait(&sthread->state_cv,
			  &sthread->state_mutex);

	UNSET_BIT(sthread->flags, THREAD_PAUSED);
	SET_BIT(sthread->flags, THREAD_RUNNING);
	(sthread->thread_pause_fn)(sthread->pause_arg);

	pthread_mutex_unlock(&sthread->state_mutex);
    }else
	pthread_mutex_unlock(&sthread->state_mutex);
}
