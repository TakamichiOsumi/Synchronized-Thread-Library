#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "../synched_thread_core.h"

typedef enum { T1 = 0, T2 = 1 } thread_id;

static char *
get_thread_id_str(thread_id id){
    switch(id){
	case T1:
	    return "T1";
	case T2:
	    return "T2";
	default:
	    assert(0);
	    return NULL;
    }
}

typedef struct thread_shared_data {
    /* The size of array */
    uintptr_t max_threads_num;
    /* An array of all (synched) threads */
    synched_thread *sync_handlers;
} thread_shared_data;

#define MAX_THREADS_NUM 2
static bool make_T2_paused = false;

/* T1 will make T2 pause and resume */
static void*
thread_T1_cb(void *arg){
    thread_shared_data *shared = (thread_shared_data *) arg;
    synched_thread *thread_T2 = &shared->sync_handlers[T2];

    synched_thread_pause(thread_T2);

    make_T2_paused = true;

    printf("T1 has set the pause flag of T2. Let T1 wait some time...\n");

    sleep(5);

    synched_thread_wake_up(thread_T2);

    printf("T1 has woken up T2\n");
    
    return NULL;
}

static void*
thread_T2_cb(void *arg){
    thread_shared_data *shared = (thread_shared_data *) arg;
    synched_thread *thread_T2 = &shared->sync_handlers[T2];
    time_t t0, t1;

    while(!make_T2_paused)
	;

    t0 = time(NULL);

    assert(IS_BIT_SET(thread_T2->flags,
		      THREAD_MARKED_FOR_PAUSE));

    printf("\tT2 will enter the pause point\n");
    synched_thread_reached_pause_point(thread_T2);
    printf("\tT2 has left the pause point\n");

    t1 = time(NULL);

    assert(t1 - t0 >= 5);

    return NULL;
}

static void *
pause_fn_cb(void *arg){
    synched_thread *thread = (synched_thread *) arg;

    assert(pthread_self() == *(thread->thread));
    assert(thread->thread_id == T2);

    printf("\t%s %s\n",
	   get_thread_id_str(thread->thread_id), __FUNCTION__);

    return NULL;
}

static void *
resume_fn_cb(void *arg){
    synched_thread *thread = (synched_thread *) arg;

    assert(pthread_self() == *(thread->thread));
    assert(thread->thread_id == T2);

    printf("\t%s %s\n",
	   get_thread_id_str(thread->thread_id), __FUNCTION__);

    return NULL;
}


static void *
xmalloc(size_t size){
    void *p;

    if ((p = malloc(size)) == NULL){
	perror("malloc");
	exit(-1);
    }

    return p;
}

int
main(int argc, char **argv){

    thread_shared_data tsd;
    pthread_t handlers[MAX_THREADS_NUM];
    int i;

    tsd.max_threads_num = MAX_THREADS_NUM;

    tsd.sync_handlers = (synched_thread *) xmalloc(sizeof(synched_thread) * MAX_THREADS_NUM);

    for (i = 0; i < MAX_THREADS_NUM; i++){
	synched_thread_gen_empty_instance(&tsd.sync_handlers[i], i, &handlers[i]);
	synched_thread_set_thread_attribute(&tsd.sync_handlers[i], true);
	synched_thread_set_pause_fn(&tsd.sync_handlers[i],
				    i == T2 ? pause_fn_cb : NULL,
				    i == T2 ? &tsd.sync_handlers[i] : NULL);
	synched_thread_set_resume_fn(&tsd.sync_handlers[i],
				     i == T2 ? resume_fn_cb : NULL,
				     i == T2 ? &tsd.sync_handlers[i] : NULL);
	synched_thread_run(&tsd.sync_handlers[i],
			   i == T1 ? thread_T1_cb : thread_T2_cb, (void *) &tsd);
    }

    for (i = 0; i < MAX_THREADS_NUM; i++)
	pthread_join(*(tsd.sync_handlers[i].thread), NULL);

    return 0;
}
