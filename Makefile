CC = gcc
CFLAGS = -g

.PHONY: clean

all: server client hash_test

server: network.o server.o runner.o hash.o
	gcc -g -lpthread -o server network.o server.o runner.o hash.o

client: client.o network.o

hash_test: hash.o hash_test.o

network.o: network.c network.h constants.h

server.o: server.c network.h server.h rpc.c jobs.c failure.c constants.h

runner.o: runner.c runner.h constants.h server.h

client.o: client.c network.h constants.h

hash.o: hash.h hash.c

hash_test.o: hash_test.c hash.h

clean:
	rm *.o
	rm server
	rm client
	rm hash_test
	rm -r jobs