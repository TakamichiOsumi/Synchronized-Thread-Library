CC	= gcc
CFLAGS	= -O0 -Wall
PROGRAM	= exec_thread_sync

all: thread_sync.o $(PROGRAM)

thread_sync.o:
	$(CC) $(CFLAGS) thread_sync.c -c

$(PROGRAM):
	@echo "not yet implemented the main()."

.PHONY:clean

clean: thread_sync.o
	rm -rf $^
