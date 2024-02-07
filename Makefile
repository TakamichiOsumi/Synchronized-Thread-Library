CC	= gcc
CFLAGS	= -O0 -Wall
SUBDIRS = Glued-Doubly-Linked-List
LIBS	= -L $(CURDIR)/$(SUBDIRS)
MYLIBS	= -lgldll
PROGRAM	= exec_thread_sync

all: thread_sync.o $(PROGRAM)

libgldll.a:
	for dir in $(SUBDIRS); do make -C $$dir; done

thread_sync.o: libgldll.a
	$(CC) $(CFLAGS) thread_sync.c -c

$(PROGRAM): thread_sync.o
	$(CC) $(CFLAGS) $(LIBS) $(MYLIBS) test_pause_and_wakeup.c $^ -o $@

.PHONY:clean test

clean: thread_sync.o
	for dir in $(SUBDIRS); do cd $$dir; make clean; cd ..; done
	rm -rf $^ $(PROGRAM)

test: $(PROGRAM)
	@echo "Will execute the inbuilt test. It consumes some time..."
	@./$(PROGRAM) &> /dev/null && echo "Successful if the return value is zero >>> $$?"
