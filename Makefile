CC	= gcc
CFLAGS	= -O0 -Wall
PROGRAM	= exec_thread_sync

all: thread_sync.o $(PROGRAM)

thread_sync.o:
	$(CC) $(CFLAGS) thread_sync.c -c

$(PROGRAM): thread_sync.o
	$(CC) $(CFLAGS) test_pause_and_wakeup.c $^ -o $@

.PHONY:clean test

clean: thread_sync.o
	rm -rf $^ $(PROGRAM)

test: $(PROGRAM)
	@echo "Will execute the inbuilt test. It consumes some time..."
	@./$(PROGRAM) &> /dev/null && echo "Successful if the return value is zero >>> $$?"
