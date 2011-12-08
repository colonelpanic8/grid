CC = gcc
CFLAGS = -g

.PHONY: clean

all: server client

server: server.o bulletin.o
	gcc -g -lpthread -o server server.o bulletin.o

server.o: server.c bulletin.h rpc.c server.h rpc.c

client: client.o bulletin.o 

client.o: client.c bulletin.h

bulletin.o: bulletin.c bulletin.h

clean:
	rm *.o
	rm server