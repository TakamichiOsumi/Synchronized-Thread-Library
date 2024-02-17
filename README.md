# Thread-Synchronization

Based on pthead library, implement another thread utility library that enables user to enjoy below features:

1. Pause and resume threads

2. Thread pooling

3. Thread barrier

4. Wait Queues (TODO)

Note : Support the combination of those features is also one of the TODOs.

*Caution* : If the thread barrier threshold set by `synched_thread_barrier_init` is smaller than the number of the threads which call the `synched_thread_barrier_wait`, then user can suffer from the thread permanent block at the wait function. Imagine a case when 5 threads (T1 ~ T5) are launched with threshold equal to 3. It creates a possibility when some threads, say thread T1, T2 and T3, arrive the wait function first and satisfy the threshold and exit, before other left threads T4 and T5 call the wait function. The first three threads send signals to each other consequtively and gets released one by one. But, the left T4 and T5 started to wait after the first group exit in this scenario. Therefore, the last two threads T4 and T5 get blocked at the `synched_thread_barrier_wait` and their resumption never happens.
