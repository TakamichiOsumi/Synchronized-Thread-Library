# Synchronized Thread Library (WIP)

Based on pthead library, implement another thread library that enables user to enjoy below thread synchronization features:

* Thread Pause and Resumption

* Thread Pooling

* Thread Barrier

* Thread Wait Queues

*Warning* : If the thread barrier threshold set by `synched_thread_barrier_init` is smaller than the number of the threads which call the `synched_thread_barrier_wait`, then user can suffer from the thread permanent block at the wait function. Consider a case when 5 threads (T1 ~ T5) are launched with a threshold equal to 3. It creates an interim when some threads, say thread T1, T2 and T3, arrive the wait function first and satisfy the threshold and exit, before other left threads T4 and T5 call the wait function. The first three threads send signals to each other consequtively and get released but the left T4 and T5 threads end up waiting for signals that never come, which means their resumption doesn't happen.
