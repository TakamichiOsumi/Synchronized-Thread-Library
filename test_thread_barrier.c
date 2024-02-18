#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "synched_thread_core.h"

#define THREAD_BARRIER_THRESHOLD 3
synched_thread_barrier *thread_barrier = NULL;

static void
print_thread_indent(uintptr_t thread_no){
    int i;

    for (i = 0; i < thread_no; i++){
	printf("\t");
    }
}

static void *
thread_barrier_function(void *arg){
    uintptr_t thread_id = (uintptr_t) arg;

    synched_thread_barrier_wait(thread_barrier);
    print_thread_indent(thread_id);
    printf("thread=%lu has passed the 1st barrier\n", thread_id);

    synched_thread_barrier_wait(thread_barrier);
    print_thread_indent(thread_id);
    printf("thread=%lu has passed the 2nd barrier\n", thread_id);

    synched_thread_barrier_wait(thread_barrier);
    print_thread_indent(thread_id);
    printf("thread=%lu has passed the 3rd barrier\n", thread_id);

    return NULL;
}

static void
threads_equal_to_threshold(void){
#define MAX_THREADS_NUM 3
    pthread_t handlers[MAX_THREADS_NUM];
    uintptr_t i;

    thread_barrier = synched_thread_barrier_init(THREAD_BARRIER_THRESHOLD);

    for (i = 0; i < MAX_THREADS_NUM; i++){
	pthread_create(&handlers[i], NULL,
		       thread_barrier_function, (void *) i);
    }

    for (i = 0; i < MAX_THREADS_NUM; i++){
	pthread_join(handlers[i], NULL);
    }

    synched_thread_barrier_destroy(thread_barrier);
}

int
main(int argc, char **argv){

    threads_equal_to_threshold();

    return 0;
}
