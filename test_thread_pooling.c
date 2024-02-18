#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "synched_thread_core.h"
#include "Glued-Doubly-Linked-List/glthreads.h"

#define MAX_THREADS_NUM 8

static void *
xmalloc(size_t size){
    void *p;

    if ((p = malloc(size)) == NULL){
	perror("malloc");
	exit(-1);
    }

    return p;
}

void *
user_request_function(void *arg){
    printf("\t<%s>\n", (char *) arg);
    return NULL;
}

void *
test_thread_pool_function(void *arg){
    synched_thread_pool *thread_pool = (synched_thread_pool *) arg;

    /*
     * Exclude this thread itself to check the number of pooled threads,
     * since it's launched to execute this test.
     */
    if (glthread_list_length(thread_pool->glued_list_container) != (MAX_THREADS_NUM - 1)){
	printf("NG : Failed to manage thread pool and dispatched threads\n");
	exit(-1);
    }else{
	printf("OK : Succeeded in managing thread pool and dispatched threads\n");
    }

    return NULL;
}

int
main(int argc, char **argv){
    synched_thread_pool *thread_pool;
    synched_thread *thread;
    pthread_t handlers[MAX_THREADS_NUM];
    char thread_name_buf[THREAD_NAME_LEN];
    int i;

    thread_pool = synched_thread_pool_init(MAX_THREADS_NUM);

    for (i = 0; i < MAX_THREADS_NUM; i++){
	/* Create standby threads */
	thread = (synched_thread *) xmalloc(sizeof(synched_thread));
	snprintf(thread_name_buf, sizeof(thread_name_buf),
		 "%s=%d", "thread", i);
	synched_thread_gen_empty_instance(thread, i,
					  thread_name_buf, &handlers[i]);
	synched_thread_set_thread_attribute(thread, true);

	/* This thread is ready to be pooled. Make it wait in the background */
	synched_thread_insert_thread_into_pool(thread_pool, thread);
    }

    /* Let the waiting threads launch by a test function */
    sleep(3);

    /* Run the threads */
    synched_thread_dispatch_thread(thread_pool,
				   user_request_function, "foo");
    synched_thread_dispatch_thread(thread_pool,
				   user_request_function, "bar");

    sleep(3);

    /* Conduct some tests */
    synched_thread_dispatch_thread(thread_pool,
				   test_thread_pool_function,
				   thread_pool);

    pthread_exit(0);

    return 0;
}
