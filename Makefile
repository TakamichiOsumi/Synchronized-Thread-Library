CC	= gcc
CFLAGS	= -O0 -Wall
PROGRAM	= exec_thread_sync

all: thread_sync.o $(PROGRAM)

thread_sync.o:
	$(CC) $(CFLAGS) thread_sync.c -c

$(PROGRAM): thread_sync.o
	$(CC) $(CFLAGS) test_pause_and_wakeup.c $^ -o $@

.PHONY:clean

clean: thread_sync.o
	rm -rf $^ $(PROGRAM)
