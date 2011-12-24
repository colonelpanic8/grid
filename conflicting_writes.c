#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include "network.h"
#include "constants.h"

#define LISTEN 1
#define DONT 0

void conflicting_writes(host_port *host) {
  int connection, err;
  err = make_connection_with(host->ip, host->port, &connection);
  if(err < 0) {
    problem("Connection failure.\n");
    exit(-1);
  }
  printf("Made connection in second thread, file descriptor is %d\n", connection);
  err = DONT;
  safe_send(connection, &err, sizeof(int));
  err = 3000;
  safe_send(connection, &err, sizeof(int));	     
}

void receive_int(int *connection) {
  int incoming = 0;
  safe_recv(*connection, &incoming, sizeof(int));
  printf("Received %d\n", incoming);
}

void listen_for_connection(host_port *info) {
  pthread_t thread;
  int connection, connect_result, listener;
  connect_result = set_up_listener(info->port, &listener);
  connect_result = -1;
  while(1) {
    do {
      connect_result = wait_for_connection(listener, &connection);
    } while(connect_result < 0);
    connect_result = 0;
    safe_recv(connection, &connect_result, sizeof(int));
    if(connect_result) {
      pthread_create(&thread, NULL, (void *(*)(void *))receive_int, (void *)&connection);
    }
  }
}

int main(int argc, char **argv) {
  int connection, err;
  pthread_t thread;
  host_port host;
  struct timespec req;
  if(argc < 3) {
    host.port = atoi(argv[1]);
    pthread_create(&thread, NULL, (void *(*)(void *))listen_for_connection, &host);
    pthread_join(thread, NULL);
  } else {
    strcpy(host.ip, argv[1]);
    host.port = atoi(argv[2]);
    make_connection_with(host.ip, host.port, &connection);
    printfl("Made connection in main thread, file descriptor is %d", connection);
    err = LISTEN;
    safe_send(connection, &err, sizeof(int));
    pthread_create(&thread, NULL, (void *(*)(void *))conflicting_writes, &host);
    req.tv_sec = 2;
    pthread_join(thread, NULL);
  }
}
