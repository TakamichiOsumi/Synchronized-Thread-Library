CC	= gcc
CFLAGS	= -O0 -Wall
SUBDIRS = Glued-Doubly-Linked-List
LIBS	= -L $(CURDIR)/$(SUBDIRS)
MYLIBS	= -lgldll
BASIC_OPERATION_TEST	= exec_thread_sync
THREAD_POOL_TEST	= exec_thread_pool
THREAD_BARRIER_TEST	= exec_thread_barrier
WAIT_QUEUE_TEST	= exec_wait_queue
TEST_SETS	= $(BASIC_OPERATION_TEST) $(THREAD_POOL_TEST) \
			$(THREAD_BARRIER_TEST) $(WAIT_QUEUE_TEST)
TARGET_LIB	= libsynched_thread.a

all: $(TEST_SETS) $(TARGET_LIB)

libgldll.a:
	for dir in $(SUBDIRS); do make -C $$dir; done

synched_thread_core.o: libgldll.a
	$(CC) $(CFLAGS) synched_thread_core.c -c

$(BASIC_OPERATION_TEST): synched_thread_core.o
	$(CC) $(CFLAGS) $(LIBS) $(MYLIBS) test_pause_and_wakeup.c $^ -o $@

$(THREAD_POOL_TEST): synched_thread_core.o
	$(CC) $(CFLAGS) $(LIBS) $(MYLIBS) test_thread_pooling.c $^ -o $@

$(THREAD_BARRIER_TEST): synched_thread_core.o
	$(CC) $(CFLAGS) $(LIBS) $(MYLIBS) test_thread_barrier.c $^ -o $@

$(WAIT_QUEUE_TEST): synched_thread_core.o
	$(CC) $(CFLAGS) $(LIBS) $(MYLIBS) test_wait_queue.c $^ -o $@

$(TARGET_LIB): synched_thread_core.o
	ar rs $@ $<

.PHONY:clean test

clean: synched_thread_core.o
	for dir in $(SUBDIRS); do cd $$dir; make clean; cd ..; done
	rm -rf $^ $(TEST_SETS) $(TARGET_LIB)

test: $(BASIC_OPERATION_TEST) $(THREAD_POOL_TEST) $(THREAD_BARRIER_TEST)
	@echo "Will execute the inbuilt basic tests. It will consume some time..."
	@./$(BASIC_OPERATION_TEST) &> /dev/null && echo "Successful if the return value is zero >>> $$?"
	@echo "Will execute the thread barrier tests. It will consume some time..."
	@./$(THREAD_BARRIER_TEST) &> /dev/null && echo "Successful if the return value is zero >>> $$?"
