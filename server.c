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
host_port *server_list;
int num_servers;
int my_port;
char my_ip[INET_ADDRSTRLEN];
queue *activeQueue;
queue *backupQueue;

#include "rpc.c"

void add_to_host_list(host_port *added_host_port, host_list *list) {
  host_list_node *currentNode;
  host_list_node *newNode;
  currentNode = list->head;
  while(currentNode->next != NULL) { currentNode = currentNode->next; }
  newNode = (host_list_node *)malloc(sizeof(host_list_node));
  newNode->host_port = added_host_port;
  currentNode->next = newNode;
}

void remove_from_host_list(host_port *removed_host_port, host_list *list) {
  host_list_node *currentNode;
  currentNode = list->head;
  if(list->head->host_port == removed_host_port) {list->head = list->head->next; return;}
  while(currentNode != NULL & currentNode->next->host_port != removed_host_port) { currentNode = currentNode -> next; }
  if (currentNode->next->host_port == removed_host_port) { currentNode->next = currentNode->next->next; }
}

void handle_host_failure(int connection) {
  char failed_host_ip[INET_ADDRSTRLEN];
  get_ip(connection,failed_host_ip);
  int i;
  for(i = 0; i < num_servers; i++) {
    if (!strcmp(server_list[i].ip,failed_host_ip)) {
      server_list[i] = server_list[i+1];
    }
  }
  update_q_host_failed(failed_host_ip,activeQueue); //backup queue is received by RPC
}

void update_q_host_failed (char* failed_host_ip, queue *Q) {
   node_j *current;
   current = Q->head;
   while(current != NULL) {
       remove_host_from_replica_list(failed_host_ip, current->obj);
       current = current->next;
   } 
}

void remove_host_from_replica_list(char* failed_host_ip, job* job) {
  host_port** remaining_server_list = malloc(sizeof(host_port**)*num_servers);
  host_port* replica_list = job->replica_list;
  int i;
  int j;
  for(i = 0; i < num_servers; i++) {
    remaining_server_list[i] = &server_list[i];
  }
  i = 0;
  while(replica_list[i].port != 0 & strcmp(job->replica_list[i].ip,failed_host_ip)) {
    for(j = 0; j < num_servers; j++) { // remove server found from available servers
      if ( !strcmp(server_list[j].ip,replica_list[i].ip) ) { remaining_server_list[j] = NULL; }
    }
    i++;
  }
  while(replica_list[i].port != 0) {
    replica_list[i] = replica_list[i+1];
    for(j = 0; j < num_servers; j++) { // remove remaining good servers from available servers
      if ( !strcmp(server_list[j].ip,replica_list[i+1].ip) ) { remaining_server_list[j] = NULL; }
    }
    i++;
  }
  i = 0;
  while(remaining_server_list[i] == NULL) {
    i++;
  }  
  add_replica(*remaining_server_list[i], job);
}

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
  listener_set_up();
  if(argc < 3) {

    server_list = malloc(sizeof(host_port));
    num_servers = 1;

    get_my_ip(server_list[0].ip);
    server_list[0].port = my_port;
    print_server_list();
  } else {
    num_servers = get_servers(argv[2], atoi(argv[3]), 1, &server_list);
    server_list[num_servers].port = my_port;
    get_my_ip(server_list[num_servers].ip);
    strcpy(my_ip, server_list[num_servers].ip);
    num_servers++;
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

int get_servers(char *hostname, int port, int add_slots, host_port **dest) {
  int connection = 0;
  int n_servers, err;
  bulletin_make_connection_with(hostname, port, &connection);
  err = SEND_SERVERS;
  safe_send(connection, &err, sizeof(int));
  safe_recv(connection, &n_servers, sizeof(int));

  *dest = malloc((n_servers+add_slots)*sizeof(host_port));
  memset(*dest, 0, (n_servers+add_slots)*sizeof(host_port));
  safe_recv(connection, *dest, n_servers*sizeof(host_port));
  close(connection);
  return n_servers;
}

void send_update(int connection) {
  int err;
  int list_conflict = RECEIVE_UPDATE;
  if (safe_send(connection, &list_conflict, sizeof(int))) {
    handle_host_failure(connection); } else if (safe_send(connection, &num_servers, sizeof(int))) {
    handle_host_failure(connection); } else if (safe_send(connection, (void*)server_list, num_servers*sizeof(host_port))) {
    handle_host_failure(connection); } else if (safe_recv(connection, &list_conflict, sizeof(int))) {
    handle_host_failure(connection); } else if (list_conflict) {
    //handle conflicts somehow here
    return;
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

void copy_job(host_port *hip, job *cop_job) {
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

void add_replica(host_port host, job *rep_job) {
  int i = 0;
  while(rep_job->replica_list[i].port != 0) {
    i++;
  }
  rep_job->replica_list[i] = host;
}

void realloc_servers_list() {
  
}
