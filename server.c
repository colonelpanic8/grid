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
host_port *my_hostport;
queue *activeQueue;
queue *backupQueue;

#include "rpc.c"

int main(int argc, char **argv) {
  char name[INET_ADDRSTRLEN];
  server_list = new_host_list();
  queue_setup();

  my_hostport = (host_port *)malloc(sizeof(host_port));
  get_my_ip(my_hostport->ip);
  my_hostport->port = atoi(argv[1]);
  listener_set_up();

  if(argc < 3) {
    add_to_host_list(my_hostport, server_list);
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

int send_host_list(int connection, host_list *list) {
  int err, num;
  host_list_node *runner;
  runner = list->head;
  num = 0;
  
  while(runner) {
    num++;
    runner = runner->next;
  }
  
  err = safe_send(connection, &num, sizeof(int));
  if(err < 0) return err;
  
  runner = list->head;
  while(runner) {
    err = safe_send(connection, runner->host, sizeof(host_port));
    if(err < 0) return err;
    runner = runner->next;
  }
  return OKAY;
}

int receive_host_list(int connection, host_list *list) {
  int err, num, i;
  host_port *temp;
  num = 0;
  err = safe_recv(connection, &num, sizeof(int));
  if(err < OKAY) return err;
 
  for(i = 0; i < num; i++) {
    temp = malloc(sizeof(host_port));
    err = safe_recv(connection, temp, sizeof(host_port));
    if(err < OKAY){
      free_host_list(list, 1);
      return err;
    }
    add_to_host_list(temp, list);
  }
  
  return OKAY;
}

void free_host_list(host_list *list, int flag) {
  host_list_node *runner = list->head;
  while(runner) {
    if(runner->host && flag) free(runner->host);
    runner = runner->next;
  }
  free(list);
}

host_list *new_host_list() {
  host_list *newList;
  newList = malloc(sizeof(host_list));
  newList->head = NULL;
  return newList;
}

void add_to_host_list(host_port *added_host_port, host_list *list) {
  host_list_node *currentNode;
  currentNode = list->head;
  if(!list->head) {
    list->head = (host_list_node *)malloc(sizeof(host_list_node));
    list->head->host = added_host_port;
    list->head->next = NULL;
    return;
  }
  while(currentNode->next) {
    currentNode = currentNode->next;
  }
  currentNode->next = (host_list_node *)malloc(sizeof(host_list_node));;
  currentNode->next->next = NULL;
  currentNode->next->host = added_host_port;
}

void remove_from_host_list(host_port *removed_host_port, host_list *list) {
  host_list_node *currentNode;
  currentNode = list->head;
  if(list->head->host == removed_host_port) {list->head = list->head->next; return;}
  while(currentNode != NULL & currentNode->next->host != removed_host_port)
    { currentNode = currentNode -> next; }
  if (currentNode->next->host == removed_host_port) { currentNode->next = currentNode->next->next; }
}

host_port* find_host_in_list(char *hostname, host_list *list) {
  host_list_node *current_node;
  current_node = list->head;
  while(current_node != NULL) { 
    if (!strcmp(current_node->host->ip,hostname)) { return current_node->host; }
     current_node = current_node->next;
  }
}

host_port* get_hostport_from_connection(int connection) {
  char failed_host_ip[INET_ADDRSTRLEN];
  get_ip(connection,failed_host_ip);
  return find_host_in_list(failed_host_ip,server_list);
}

void clone_host_list(host_list *old_list, host_list *new_list) {
  new_list = new_host_list();
  host_list_node *new_node;
  new_node = (host_list_node *)malloc(sizeof(host_list_node));
  new_list->head = new_node;
  host_list_node *old_node;
  old_node = old_list->head;
  host_list_node *new_successor;
  while(old_node != NULL) { 
    new_node->host = old_node->host;
    new_successor = (host_list_node *)malloc(sizeof(host_list_node));
    new_node->next = new_successor;
    old_node = old_node->next;
    new_node = new_node->next;
  }
}

void handle_host_failure_by_connection(int connection) { // failed on a send
  host_port *failed_host;
  failed_host = get_hostport_from_connection(connection);
  handle_host_failure(failed_host);
}

void handle_host_failure(host_port *failed_host) { // failed on a send
  local_handle_failure(failed_host);
  notify_others_of_failure(failed_host);
}

void local_handle_failure(host_port *failed_host) {
  remove_from_host_list(failed_host,server_list);
  update_q_host_failed(failed_host,activeQueue); //backup queue is received by RPC
}

void notify_others_of_failure(host_port *failed_host) { // tell everyone
  int connection = 0;

  host_list_node* current_node;
  current_node = server_list->head;

  while(current_node != NULL) {
   if(strcmp(current_node->host->ip,my_hostport->ip)) {
    printf("Notifying %s and my ip is %s\n",current_node->host->ip,my_hostport->ip);
    int err;
    err = bulletin_make_connection_with(current_node->host->ip, current_node->host->port, &connection);
    if (err < OKAY) { handle_host_failure(current_node->host); } else { inform_of_failure(connection,failed_host); }
    }
    current_node = current_node->next;
  }
}

void inform_of_failure(int connection, host_port *failed_host) {
  int err = INFORM_OF_FAILURE;
  err = safe_send(connection,&err,sizeof(int));
  if (err < 0) { handle_host_failure_by_connection(connection); return; }
  err = safe_send(connection,failed_host,sizeof(host_port));
  if (err < 0) { handle_host_failure_by_connection(connection); return; }
}

void update_q_host_failed (host_port* failed_host, queue *Q) {
   job_list_node *current;
   current = Q->head;
   while(current != NULL) {
       replace_host_in_replica_list(failed_host, current->entry);
       current = current->next;
   } 
}

void replace_host_in_replica_list(host_port* failed_host, job* job) {
  host_list* replica_list = job->replica_list;
  host_list* remaining_servers_list;
  clone_host_list(server_list,remaining_servers_list);

  host_list_node* current_node;
  current_node = job->replica_list->head;

  while(current_node != NULL) {
    current_node = current_node->next;
    remove_from_host_list(current_node->host,remaining_servers_list);
  }
  
  remove_from_host_list(failed_host,job->replica_list);

  host_port* replacement_host;
  replacement_host = remaining_servers_list->head->host; // not great
  
  if(replacement_host != NULL) { add_replica(replacement_host, job); }
}

void print_server_list() {
  host_list_node* current_node;
  printf("Servers:\n");
  current_node = server_list->head;
  while(current_node != NULL) {
    printf("\tIP: %s, port: %d\n", current_node->host->ip, current_node->host->port);
    current_node = current_node->next;
  }
}

job *get_job() {
  return NULL;
}


void queue_setup() {
  activeQueue = (queue *)malloc(sizeof(queue));
  backupQueue = (queue *)malloc(sizeof(queue));
  activeQueue->head = NULL;
  backupQueue->head = NULL;
}

int get_servers(char *hostname, int port, int add_slots, host_list *server_list) {
  int connection = 0;
  int result = 0;
  bulletin_make_connection_with(hostname, port, &connection);
  int err = SEND_SERVERS;
  safe_send(connection,&err,sizeof(int));
  result = receive_host_list(connection,server_list);
  close(connection);
  return result;
}

int send_update(int connection) {
  int err;
  int list_conflict = RECEIVE_UPDATE;
  err = safe_send(connection, &list_conflict, sizeof(int));
  if (err < 0) return err;
  err = send_host_list(connection, server_list);
  if (err < 0) return err;
  err = safe_recv(connection, &list_conflict, sizeof(int));
  if (err < 0) return err;
  if (list_conflict) {
    //handle conflicts somehow here
    return;
  }
}

void distribute_update() {
  int err;
  int connection;
  host_list_node* current_node;
  current_node = server_list->head;

  while(current_node != NULL) {
    if(strcmp(my_hostport->ip, current_node->host->ip)) {
      err = bulletin_make_connection_with(current_node->host->ip, current_node->host->port, &connection);
      if (err < OKAY) handle_host_failure(current_node->host);
      err = send_update(connection);
      if (err < OKAY) handle_host_failure_by_connection(connection);
      close(connection);
      current_node = current_node->next;
    }
  }
}

void listener_set_up() {
  pthread_t thread;
  int *listener, connect_result;
  listener = malloc(sizeof(int));
  connect_result = bulletin_set_up_listener(my_hostport->port, listener);
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
    copy_job(current_node->host, rep_job);
    current_node = current_node->next;
  }
}

void copy_job(host_port *hip, job *cop_job) {
  int connection = 0;
  int err = OKAY;
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
