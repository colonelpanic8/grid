CC = gcc
CFLAGS = -g

.PHONY: clean

all: server

server: server.o bulletin.o
	gcc -g -lpthread -o server server.o bulletin.o

server.o: server.c bulletin.h rpc.c server.h rpc.c

client.o: client.c bulletin.h dicth.h

bulletin.o: bulletin.c bulletin.h

clean:
	rm *.o
	rm server