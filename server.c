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
host_ip *server_list;
int num_servers;
int my_port;
char my_ip[INET_ADDRSTRLEN];
queue *activeQueue;
queue *backupQueue;

#include "rpc.c"

void print_server_list() {
  int i;
  printf("Servers:\n");
  for(i = 0; i < num_servers; i++) {
    printf("\tIP: %s, port: %d\n", server_list[i].ip, server_list[i].port);
  }
}


int main(int argc, char **argv) {
  char name[INET_ADDRSTRLEN];
  my_port = atoi(argv[1]);
  if(argc < 3) {

    server_list = malloc(sizeof(host_ip));
    num_servers = 1;

    get_my_ip(server_list[0].ip);
    server_list[0].port = my_port;
    print_server_list();
  } else {
    num_servers = get_servers(argv[2], atoi(argv[3]), 1, &server_list);
    server_list[num_servers].port = my_port;
    get_my_ip(server_list[num_servers].ip);
    num_servers++;
    listener_set_up();
    print_server_list();
    distribute_update();
  }
  // initialize queue 
  while(1) { 
    sleep(1000);
  } 
}

void queue_setup() {

}

int get_servers(char *hostname, int port, int add_slots, host_ip **dest) {
  int connection = 0;
  int n_servers, err;
  bulletin_make_connection_with(hostname, port, &connection);
  err = SEND_SERVERS;
  safe_send(connection, &err, sizeof(int));
  safe_recv(connection, &n_servers, sizeof(int));
  *dest = malloc((n_servers+add_slots)*sizeof(host_ip));
  memset(*dest, 0, (n_servers+add_slots)*sizeof(host_ip));
  safe_recv(connection, *dest, n_servers*sizeof(host_ip));
  close(connection);
  return n_servers;
}

void send_update(int connection) {
  int err;
  err = RECEIVE_UPDATE;
  safe_send(connection, &err, sizeof(int));
  safe_send(connection, &num_servers, sizeof(int));
  safe_send(connection, (void*)server_list, num_servers*sizeof(host_ip));
  safe_recv(connection, &err, sizeof(int));
  if(err) {
  }  
}

void distribute_update() {
  int i, connection;
  for(i = 0; i < num_servers; i++) {
    if(strcmp(my_ip, server_list[i].ip)) {
      bulletin_make_connection_with(server_list[i].ip, server_list[i].port, &connection);
      send_update(connection);
      close(connection);
    }
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
  do {
    connect_result = bulletin_wait_for_connection(*listener, &connection);
  } while(connect_result < 0);
  pthread_create(&thread, NULL, (void *(*)(void *))
		listen_for_connection, listener);
  handle_rpc(connection);
  close(connection);
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

void realloc_servers_list() {
  
}
