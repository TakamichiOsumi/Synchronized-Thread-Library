#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "thread_sync.h"

typedef enum { T1 = 0, T2 = 1 } thread_id;

typedef struct thread_shared_data {
    /* The size of array */
    uintptr_t max_threads_num;
    /* An array of all (synched) threads */
    synched_thread *sync_handlers;
} thread_shared_data;

#define MAX_THREADS_NUM 2
#define THREAD_NAME_LEN 256

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

    printf("T2 will enter the pause point\n");
    synched_thread_reached_pause_point(thread_T2);
    printf("T2 has left the pause point\n");

    t1 = time(NULL);

    assert(t1 - t0 >= 5);

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
    pthread_t handlers[THREAD_NAME_LEN];
    char thread_name[THREAD_NAME_LEN];
    int i;

    tsd.max_threads_num = MAX_THREADS_NUM;
    tsd.sync_handlers = (synched_thread *) xmalloc(sizeof(synched_thread) * MAX_THREADS_NUM);

    for (i = 0; i < MAX_THREADS_NUM; i++){
	snprintf(thread_name, sizeof(thread_name), "%s%d", "thread", i);
	synched_thread_create(&tsd.sync_handlers[i], i, thread_name, &handlers[i]);
	synched_thread_run(&tsd.sync_handlers[i],
			   i == T1 ? thread_T1_cb : thread_T2_cb, (void *) &tsd);
    }

    pthread_exit(0);

    return 0;
}
