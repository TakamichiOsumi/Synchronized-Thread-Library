# Synchronized Thread Library (Work in progress)

Evolve POSIX thread library to enable user to enjoy below thread synchronization features:

* Thread Pause and Resumption
  * Make some thread pause or resume from other thread by indicating pause point safely.

* Thread Pooling
  * Pool ready-to-use threads in the background and deploy those to process user defined functions flexibly.

* Thread Barrier
  * Make threads wait at specific point of code until the number of threads equal to the defined threshold reaches the point.

* Thread Wait Queues
  * Block threads until some user defined condition is satisfied but at the same time encapsulate internal blocking mechanism from user application.
    * Relieve the pains of developer to declare and maintain pthread mutex or condition variables by hiding thread synchronization complexity.

* Thread Monitors (TODO)
  * Set the upper limit of reader/writer threads to be allowed to run in the critical section.
  * Allow multiple writer threads to operate in the critical section.
  * Specify the type of thread that enters the critical section next to be a different type of thead.
  * Replacement Property
    * Replace either non-last reader or non-last writer thread in the critical section with another same type of waiting thread if any.

*Warning* : If the thread barrier threshold set by `synched_thread_barrier_init` is smaller than the number of the threads which call the `synched_thread_barrier_wait`, then user can suffer from the thread permanent block at the wait function. Consider a case when 5 threads (T1 ~ T5) are launched with a threshold equal to 3. It creates an interim when some threads, say thread T1, T2 and T3, arrive the wait function first and satisfy the threshold and exit, before other left threads T4 and T5 call the wait function. The first three threads send signals to each other consequtively and get released but the left T4 and T5 threads end up waiting for signals that never come, which means their resumption doesn't happen.
