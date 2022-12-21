CC = gcc
CFLAGS = -g -pthread

.PHONY: clean setup

all: server client hash_test conflicting_writes setup

server: network.o server.o runner.o hash.o
	$(CC) $(CFLAGS) -o server network.o server.o runner.o hash.o

client: client.o network.o
	$(CC) $(CFLAGS) -o client client.o network.o

hash_test: hash.o hash_test.o
	$(CC) $(CFLAGS) -o hash_test hash_test.o hash.o

conflicting_writes: conflicting_writes.o network.o
	$(CC) $(CFLAGS) -o conflicting_writes conflicting_writes.o network.o

network.o: network.c network.h constants.h
	$(CC) $(CFLAGS) -c -o network.o network.c

server.o: server.c network.h server.h rpc.c jobs.c failure.c constants.h
	$(CC) $(CFLAGS) -c -o server.o server.c

runner.o: runner.c runner.h constants.h server.h
	$(CC) $(CFLAGS) -c -o runner.o runner.c

client.o: client.c network.h constants.h
	$(CC) $(CFLAGS) -c -o client.o client.c

hash.o: hash.h hash.c
	$(CC) $(CFLAGS) -c -o hash.o hash.c

hash_test.o: hash_test.c hash.h
	$(CC) $(CFLAGS) -c -o hash_test.o hash_test.c

conflicting_writes.o: conflicting_writes.c network.c constants.h network.h
	$(CC) $(CFLAGS) -c -o conflicting_writes.o conflicting_writes.c

clean:
	rm *.o
	rm server
	rm client
	rm hash_test
	rm conflicting_writes
	-rm -r jobs
