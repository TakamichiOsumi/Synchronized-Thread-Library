
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

    return NULL;
}

void
synched_thread_set_thread_attribute(synched_thread *sthread, bool joinable){
    pthread_attr_setdetachstate(&sthread->attributes,
				joinable ?
				PTHREAD_CREATE_JOINABLE : PTHREAD_CREATE_DETACHED);
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
