#ifndef __THREAD_SYNC__
#define __THREAD_SYNC__

#include <stdbool.h>
#include <pthread.h>

typedef struct synched_thread {
    char name[32];
    bool thread_created;
    pthread_t thread;
    void *arg;
    void *(*thread_fn)(void *);
    pthread_attr_t attributes;
} synched_thread;


synched_thread *synched_thread_create(synched_thread *sthread, char *name);
void synched_thread_set_thread_attribute(synched_thread *sthread, bool joinable);
void synched_thread_run(synched_thread *sthread, void *(*thread_fn)(void *), void *arg);

#endif
