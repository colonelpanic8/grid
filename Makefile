CC = gcc
CFLAGS = -g

.PHONY: clean

all: server client

server: server.o bulletin.o runner.o hash.o
	gcc -g -lpthread -o server server.o bulletin.o

client: client.o bulletin.o

bulletin.o: bulletin.c bulletin.h constants.h

server.o: server.c bulletin.h rpc.c server.h rpc.c constants.h

runner.o: runner.c runner.h constants.h server.h

client.o: client.c bulletin.h constants.h

hash.o: hash.h hash.c

clean:
	rm *.o
	rm server