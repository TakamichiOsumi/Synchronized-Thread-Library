CC	= gcc
CFLAGS	= -O0 -Wall
SUBDIRS = Glued-Doubly-Linked-List
LIBS	= -L $(CURDIR)/$(SUBDIRS)
MYLIBS	= -lgldll
BASIC_OPERATION_TEST	= exec_thread_sync
THREAD_POOL_TEST	= exec_thread_pool

all: thread_sync.o $(BASIC_OPERATION_TEST) $(THREAD_POOL_TEST)

libgldll.a:
	for dir in $(SUBDIRS); do make -C $$dir; done

thread_sync.o: libgldll.a
	$(CC) $(CFLAGS) thread_sync.c -c

$(BASIC_OPERATION_TEST): thread_sync.o
	$(CC) $(CFLAGS) $(LIBS) $(MYLIBS) test_pause_and_wakeup.c $^ -o $@

$(THREAD_POOL_TEST): thread_sync.o
	$(CC) $(CFLAGS) $(LIBS) $(MYLIBS) test_thread_pooling.c $^ -o $@

.PHONY:clean test

clean:
	for dir in $(SUBDIRS); do cd $$dir; make clean; cd ..; done
	rm -rf $^ $(BASIC_OPERATION_TEST) $(THREAD_POOL_TEST)

test: $(BASIC_OPERATION_TEST) $(THREAD_POOL_TEST)
	@echo "Will execute the inbuilt basic test. It consumes some time..."
	@./$(BASIC_OPERATION_TEST) &> /dev/null && echo "Successful if the return value is zero >>> $$?"
