CC	= gcc
CFLAGS	= -O0 -Wall
SUBDIR1 = Glued-Doubly-Linked-List
SUBDIR2	=  Linked-List
SUBDIRS = $(SUBDIR1) $(SUBDIR2)
LIBS	= -L $(CURDIR)/$(SUBDIR1) -L $(CURDIR)/$(SUBDIR2)
MYLIBS	= -lgldll -llinked_list
BASIC_OPERATION_TEST	= exec_thread_sync
THREAD_POOL_TEST	= exec_thread_pool
THREAD_BARRIER_TEST	= exec_thread_barrier
WAIT_QUEUE_TEST	= exec_wait_queue
TEST_SETS	= $(BASIC_OPERATION_TEST) $(THREAD_POOL_TEST) \
			$(THREAD_BARRIER_TEST) $(WAIT_QUEUE_TEST)
TARGET_LIB	= libsynched_thread.a

all: $(TEST_SETS) $(TARGET_LIB)

libgldll.a liblinked_list.a:
	for dir in $(SUBDIRS); do make -C $$dir; done

synched_thread_core.o: libgldll.a
	$(CC) $(CFLAGS) synched_thread_core.c -c

$(BASIC_OPERATION_TEST): synched_thread_core.o
	$(CC) $(CFLAGS) $(LIBS) $(MYLIBS) test_pause_and_wakeup.c $^ -o $@

$(THREAD_POOL_TEST): synched_thread_core.o
	$(CC) $(CFLAGS) $(LIBS) $(MYLIBS) test_thread_pooling.c $^ -o $@

$(THREAD_BARRIER_TEST): synched_thread_core.o
	$(CC) $(CFLAGS) $(LIBS) $(MYLIBS) test_thread_barrier.c $^ -o $@

emulate_traffic_intersection.o: emulate_traffic_intersection.c
	$(CC) $(CFLAGS) -c $< -o $@

$(WAIT_QUEUE_TEST): synched_thread_core.o emulate_traffic_intersection.o
	$(CC) $(CFLAGS) $(LIBS) $(MYLIBS) test_wait_queue.c $^ -o $@

$(TARGET_LIB): synched_thread_core.o
	ar rs $@ $<

.PHONY:clean test

clean: synched_thread_core.o
	for dir in $(SUBDIRS); do cd $$dir; make clean; cd ..; done
	rm -rf $(TEST_SETS) $(TARGET_LIB) *.o

test: $(TEST_SETS)
	@echo "Will execute the thread synchronization basic tests."
	@./$(BASIC_OPERATION_TEST) &> /dev/null && echo "Successful when the value is zero >>> $$?"
	@echo "Will execute the thread barrier tests."
	@./$(THREAD_BARRIER_TEST) &> /dev/null && echo "Successful when the value is zero >>> $$?"
	@echo "Will execute the wait queue tests."
	@./$(WAIT_QUEUE_TEST) &> /dev/null && echo "Successful when the value is zero >>> $$?"
