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
host_list *server_list;
int num_servers;
int my_port;
char my_ip[INET_ADDRSTRLEN];
queue *activeQueue;
queue *backupQueue;

#include "rpc.c"

host_list *new_host_list() {
  host_list *newList;
  newList = malloc(sizeof(host_list));
  return newList;
}

void add_to_host_list(host_port *added_host_port, host_list *list) {
  host_list_node *currentNode;
  host_list_node *newNode;
  newNode = (host_list_node *)malloc(sizeof(host_list_node));
  if (list->head = NULL) { list->head = newNode; } else {
    currentNode = list->head;
    while(currentNode->next != NULL) { currentNode = currentNode->next; }
    currentNode->next = newNode;
    }
  newNode->host_port = added_host_port;
}

void remove_from_host_list(host_port *removed_host_port, host_list *list) {
  host_list_node *currentNode;
  currentNode = list->head;
  if(list->head->host_port == removed_host_port) {list->head = list->head->next; return;}
  while(currentNode != NULL & currentNode->next->host_port != removed_host_port) { currentNode = currentNode -> next; }
  if (currentNode->next->host_port == removed_host_port) { currentNode->next = currentNode->next->next; }
}

void get_hostport_from_connection(int connection, host_port *result) {
  char failed_host_ip[INET_ADDRSTRLEN];
  get_ip(connection,failed_host_ip);
  result = NULL;
  host_list_node *current_node;
  current_node = server_list->head;
  while(current_node != NULL & result == NULL) { 
     if (!strcmp(current_node->host_port->ip,failed_host_ip)) { result = current_node->host_port; }
     current_node = current_node->next;
 }
}

void clone_host_list(host_list *old_list, host_list *new_list) {
  new_list = (host_list *)malloc(sizeof(host_list));
  host_list_node *new_node;
  new_node = (host_list_node *)malloc(sizeof(host_list_node));
  new_list->head = new_node;
  host_list_node *old_node;
  old_node = old_list->head;
  host_list_node *new_successor;
  while(old_node != NULL) { 
    new_node->host_port = old_node->host_port;
    new_successor = (host_list_node *)malloc(sizeof(host_list_node));
    new_node->next = new_successor;
    old_node = old_node->next;
    new_node = new_node->next;
  }
}

void handle_host_failure(int connection) {
  host_port *failed_host;
  get_hostport_from_connection(connection,failed_host);
  update_q_host_failed(failed_host,activeQueue); //backup queue is received by RPC
}

void update_q_host_failed (host_port* failed_host, queue *Q) {
   node_j *current;
   current = Q->head;
   while(current != NULL) {
       replace_host_in_replica_list(failed_host, current->obj);
       current = current->next;
   } 
}

void replace_host_in_replica_list(host_port* failed_host, job* job) {
  host_list* replica_list = job->replica_list;
  remove_from_host_list(failed_host,server_list);
  host_list* remaining_servers_list;
  clone_host_list(server_list,remaining_servers_list);

  host_list_node* current_node;
  current_node = job->replica_list->head;

  while(current_node != NULL) {
    current_node = current_node->next;
    remove_from_host_list(current_node->host_port,remaining_servers_list);
  }
  
  remove_from_host_list(failed_host,job->replica_list);

  host_port* replacement_host;
  replacement_host = remaining_servers_list->head->host_port; // not great
  
  if(replacement_host != NULL) { add_replica(replacement_host, job); }
}

void print_server_list() {
  printf("Servers:\n");

  host_list_node* current_node;
  current_node = server_list->head;

  while(current_node != NULL) {
    printf("\tIP: %s, port: %d\n", current_node->host_port->ip, current_node->host_port->port);
    current_node = current_node->next;
  }
}

int main(int argc, char **argv) {
  char name[INET_ADDRSTRLEN];
  my_port = atoi(argv[1]);
  listener_set_up();
  server_list = new_host_list();

  char my_ip[INET_ADDRSTRLEN];
  host_port* my_hostport;
  my_hostport = (host_port *)malloc(sizeof(host_port));
  get_my_ip(my_hostport->ip);
    
  my_hostport->port = my_port;

  if(argc < 3) {
    add_to_host_list(my_hostport,server_list);
    print_server_list();
  } else {
    get_servers(argv[2], atoi(argv[3]), 1, server_list);
    add_to_host_list(my_hostport,server_list);
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

int get_servers(char *hostname, int port, int add_slots, host_list *server_list) {
  int connection = 0;
  int result = 0;
  bulletin_make_connection_with(hostname, port, &connection);
  result = get_server_list(connection,server_list);
  close(connection);
  return result;
}

void send_update(int connection) {
  int err;
  int list_conflict = RECEIVE_UPDATE;
  if (safe_send(connection, &list_conflict, sizeof(int)) < 0) {
    handle_host_failure(connection); } else if (safe_send(connection, &num_servers, sizeof(int)) < 0) {
    handle_host_failure(connection); } else if (safe_send(connection, (void*)server_list, num_servers*sizeof(host_port)) < 0) {
    handle_host_failure(connection); } else if (safe_recv(connection, &list_conflict, sizeof(int)) < 0) {
    handle_host_failure(connection); } else if (list_conflict) {
    //handle conflicts somehow here
    return;
  }
}

void distribute_update() {
  int connection;
  host_list_node* current_node;
  current_node = server_list->head;

  while(current_node != NULL) {
    if(strcmp(my_ip, current_node->host_port->ip)) {
      bulletin_make_connection_with(current_node->host_port->ip, current_node->host_port->port, &connection);
      send_update(connection);
      close(connection);
      current_node = current_node->next;
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
  host_list_node* current_node;
  current_node = rep_job->replica_list->head;

  while(current_node != NULL) {
    copy_job(current_node->host_port, rep_job);
    current_node = current_node->next;
  }
}

void copy_job(host_port *hip, job *cop_job) {
  int connection = 0;
  bulletin_make_connection_with(hip->ip, hip->port, &connection);
  send_string(connection, "4");
  send(connection, &cop_job, sizeof(job), 0);
  close(connection);
}

// void selectHost(job *copy_job) {
//  int i, j;
//  i = 0;
//  j = 0;
//  while(server_list[i].port != 0) i++;
//  while(j < NUM_REPLICAS) {
//    add_replica(server_list[rand() %i ], copy_job);
//    j++;
//  }
// }

void add_replica(host_port *host, job *rep_job) {
  add_to_host_list(host,rep_job->replica_list);
}

void realloc_servers_list() {
  
}
