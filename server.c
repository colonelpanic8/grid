#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include "bulletin.h"
#include "constants.h"
#include "server.h"

pthread_mutex_t server_list_mutex = PTHREAD_MUTEX_INITIALIZER;
host_ip server_list[MAXIMUM_NODES];
int my_port;
queue *activeQueue;
queue *backupQueue;

#include "rpc.c"

void print_server_list() {
  int i;
  printf("Servers:\n");
  for(i = 0; server_list[i].port != 0; i++) {
    printf("\tIP: %s, port: %d\n", server_list[i].ip, server_list[i].port);
  }
}


int main(int argc, char **argv) {
  if(argc < 3) {
    memset(server_list, 0, MAXIMUM_NODES*sizeof(host_ip));
    /* FIX  add yourself to server list */ 
  } else {
    get_servers(argv[2], atoi(argv[3]));
  }
// initialize queue 
  my_port = atoi(argv[1]);
  printf("spawning thread\n");
  listener_set_up();
  while(1) { 
    sleep(1000);
  } 
}

void replicate(job *rep_job) {
  int i = 0;
  selectHost(rep_job);  
  while(rep_job->replica_list[i].port != 0) {
    copy_job(&rep_job->replica_list[i], rep_job);
    i++;
  }
}

void copy_job(host_ip *hip, job *cop_job) {
  int connection = 0;
  bulletin_make_connection_with(hip->ip, hip->port, &connection);
  send_string(connection, "4");
  send(connection, &cop_job, sizeof(job), 0);
  close(connection);
}

void selectHost(job *copy_job) {
  int i, j;
  i = 0;
  j = 0;
  while(server_list[i].port != 0) i++;
  while(j < NUM_REPLICAS) {
    add_replica(server_list[rand() %i ], copy_job);
    j++;
  }
}

void add_replica(host_ip host, job *rep_job) {
  int i = 0;
  while(rep_job->replica_list[i].port != 0) {
    i++;
  }
  rep_job->replica_list[i] = host;
}

void get_servers(char *hostname, int port) {
  int connection = 0;
  bulletin_make_connection_with(hostname, port, &connection);
  send_string(connection, "0");

  recv(connection, server_list, MAXIMUM_NODES*sizeof(host_ip), 0);
  send_identity(connection);
  close(connection);
  
  distribute_identity();
}


void send_identity(int connection) {
  char buffer[8];
  sprintf(buffer, "%d", my_port);
  send_string(connection, "3");
  send_string(connection, buffer);
}

void distribute_identity() {
  int i, connection;
  for(i = 0; i < MAXIMUM_NODES; i++) {
    if(!server_list[i].port) {
      break;
    }
    bulletin_make_connection_with(server_list[i].ip, server_list[i].port, &connection);
    send_identity(connection);
    close(connection);
  }
}

void listener_set_up() {
  pthread_t thread;
  int *listener, connect_result;
  listener = malloc(sizeof(int));
  connect_result = bulletin_set_up_listener(my_port, listener);
  pthread_create(&thread, NULL, (void *(*)(void *))listen_for_connection, listener);
}

void listen_for_connection(int *listener) {
  int connection, connect_result;
  pthread_t thread;
  connect_result = -1;
  printf("listening\n");
  do {
    connect_result = bulletin_wait_for_connection(*listener, &connection);
  } while(connect_result < 0);
  pthread_create(&thread, NULL, (void *(*)(void *))
		listen_for_connection, listener);
  printf("handling rpc\n");
  handle_rpc(connection);
  printf("closing connection\n");
  close(connection);
}


void handle_rpc(int connection) {
  char rpc[RPC_STR_LEN+1];
  recv_string(connection, rpc, RPC_STR_LEN);
  switch(atoi(rpc)) {
  case 0:
    rpc_send_servers(connection);
    break;
  case 1:
    rpc_serve_job(connection);
    break;
  case 2:
    rpc_inform_of_completion(connection);
    break;
  case 3:
    rpc_receive_identity(connection);
    break;
  case 4:
    rpc_receive_job_copy(connection);
    break;
  default:
    break;
  }
}
