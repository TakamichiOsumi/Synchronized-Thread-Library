CC	= gcc
CFLAGS	= -O0 -Wall

# Dependent libraries
SUBDIR1 = Glued-Doubly-Linked-List
SUBDIR2	=  Linked-List
SUBDIRS = $(SUBDIR1) $(SUBDIR2)
LIBS	= -L $(CURDIR)/$(SUBDIR1) -L $(CURDIR)/$(SUBDIR2)
MYLIBS	= -lgldll -llinked_list

# Executables
BASIC_OPERATION_TEST	= exec_thread_sync
THREAD_POOL_TEST	= exec_thread_pool
THREAD_BARRIER_TEST	= exec_thread_barrier
WAIT_QUEUE_TEST	= exec_wait_queue

# Sources
TEST_SETS	= tests/$(BASIC_OPERATION_TEST) tests/$(THREAD_POOL_TEST) \
			tests/$(THREAD_BARRIER_TEST) tests/$(WAIT_QUEUE_TEST)
# Final output
TARGET_LIB	= libsynched_thread.a

all: $(TEST_SETS) $(TARGET_LIB)

libgldll.a liblinked_list.a:
	for dir in $(SUBDIRS); do make -C $$dir; done

synched_thread_core.o: libgldll.a
	$(CC) $(CFLAGS) synched_thread_core.c -c

tests/$(BASIC_OPERATION_TEST): synched_thread_core.o
	$(CC) $(CFLAGS) $(LIBS) $(MYLIBS) tests/test_pause_and_wakeup.c $^ -o $@

tests/$(THREAD_POOL_TEST): synched_thread_core.o
	$(CC) $(CFLAGS) $(LIBS) $(MYLIBS) tests/test_thread_pooling.c $^ -o $@

tests/$(THREAD_BARRIER_TEST): synched_thread_core.o
	$(CC) $(CFLAGS) $(LIBS) $(MYLIBS) tests/test_thread_barrier.c $^ -o $@

tests/emulate_traffic_intersection.o: tests/emulate_traffic_intersection.c
	$(CC) $(CFLAGS) -c $< -o $@

tests/$(WAIT_QUEUE_TEST): synched_thread_core.o tests/emulate_traffic_intersection.o
	$(CC) $(CFLAGS) $(LIBS) $(MYLIBS) tests/test_wait_queue.c $^ -o $@

$(TARGET_LIB): synched_thread_core.o
	ar rs $@ $<

.PHONY:clean test

clean: synched_thread_core.o
	for dir in $(SUBDIRS); do cd $$dir; make clean; cd ..; done
	rm -rf $(TEST_SETS) $(TARGET_LIB) *.o tests/*.o

test: $(TEST_SETS)
	@echo "Will execute the thread synchronization basic tests."
	@./tests/$(BASIC_OPERATION_TEST) &> /dev/null && echo "Successful when the value is zero >>> $$?"
	@echo "Will execute the thread barrier tests."
	@./tests/$(THREAD_BARRIER_TEST) &> /dev/null && echo "Successful when the value is zero >>> $$?"
	@echo "Will execute the wait queue tests."
	@./tests/$(WAIT_QUEUE_TEST) &> /dev/null && echo "Successful when the value is zero >>> $$?"
