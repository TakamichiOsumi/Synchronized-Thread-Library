CC	= gcc
CFLAGS	= -O0 -Wall
SUBDIRS = Glued-Doubly-Linked-List
LIBS	= -L $(CURDIR)/$(SUBDIRS)
MYLIBS	= -lgldll
BASIC_OPERATION_TEST	= exec_thread_sync
THREAD_POOL_TEST	= exec_thread_pool
THREAD_BARRIER_TEST	= exec_thread_barrier
TEST_SETS	= $(BASIC_OPERATION_TEST) $(THREAD_POOL_TEST) $(THREAD_BARRIER_TEST)
TARGET_LIB	= libsynched_thread.a

all: $(TEST_SETS) $(TARGET_LIB)

libgldll.a:
	for dir in $(SUBDIRS); do make -C $$dir; done

thread_sync.o: libgldll.a
	$(CC) $(CFLAGS) thread_sync.c -c

$(BASIC_OPERATION_TEST): thread_sync.o
	$(CC) $(CFLAGS) $(LIBS) $(MYLIBS) test_pause_and_wakeup.c $^ -o $@

$(THREAD_POOL_TEST): thread_sync.o
	$(CC) $(CFLAGS) $(LIBS) $(MYLIBS) test_thread_pooling.c $^ -o $@

$(THREAD_BARRIER_TEST): thread_sync.o
	$(CC) $(CFLAGS) $(LIBS) $(MYLIBS) test_thread_barrier.c $^ -o $@

$(TARGET_LIB): thread_sync.o
	ar rs $@ $<

.PHONY:clean test

clean:
	for dir in $(SUBDIRS); do cd $$dir; make clean; cd ..; done
	rm -rf $^ $(TEST_SETS) $(TARGET_LIB)

test: $(BASIC_OPERATION_TEST) $(THREAD_POOL_TEST) $(THREAD_BARRIER_TEST)
	@echo "Will execute the inbuilt basic tests. It will consume some time..."
	@./$(BASIC_OPERATION_TEST) &> /dev/null && echo "Successful if the return value is zero >>> $$?"
	@echo "Will execute the thread barrier tests. It will consume some time..."
	@./$(THREAD_BARRIER_TEST) &> /dev/null && echo "Successful if the return value is zero >>> $$?"
