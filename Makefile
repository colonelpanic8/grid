CC = gcc
CFLAGS = -g

.PHONY: clean

all: server

server: server.o bulletin.o
	gcc -g -lpthread -o server server.o bulletin.o

server.o: server.c bulletin.h rpc.c server.h

client.o: client.c bulletin.h dicth.h

bulletin.o: bulletin.c

clean:
	rm *.o
	rm server