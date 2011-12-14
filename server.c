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
#include "server.h"
#include "hash.h"
#include "runner.h"

#define LOCKED 1
#define UNLOCKED 0

pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
int counter = 0;

pthread_mutex_t server_list_mutex = PTHREAD_MUTEX_INITIALIZER;
host_list *server_list = NULL;
int num_servers, *listener;

host_list_node *my_host = NULL;
host_list_node *heartbeat_dest = NULL;

queue *activeQueue;
queue *backupQueue;

pthread_mutex_t d_add_mutex = PTHREAD_MUTEX_INITIALIZER;
int d_add_lock = UNLOCKED;
char who[INET_ADDRSTRLEN];

#include "rpc.c"

int main(int argc, char **argv) {
  char name[INET_ADDRSTRLEN];
  pthread_t runner_thread;
  host_port *my_hostport;
  
  signal (SIGINT, finish);
  //setup jobs folder
  if(mkdir("./jobs", S_IRWXU)) {
    if(errno == EEXIST) {
#ifdef VERBOSE
      problem("directory already exists...\n");
#endif
    } else {
      problem("mkdir failed with %d\n", errno);
    }
  }
  
  queue_setup();
  my_hostport = malloc(sizeof(host_port));
  get_my_ip(my_hostport->ip);
  my_hostport->port = atoi(argv[1]);
  my_hostport->jobs = 0;
  my_hostport->time_stamp = 0;
  my_hostport->location = 0;
  listener_set_up(my_hostport);

  if(argc < 3) {
    server_list = new_host_list(my_hostport);
    my_host = server_list->head;
    print_server_list();
  } else {
    if(get_servers(argv[2], atoi(argv[3]), 1, &server_list) < 0) {
      exit(FAILURE);
    }
    if(acquire_add_lock(server_list)) {
      problem("Fatal error. Failed to acquire add lock.\n");
      relinquish_add_lock(server_list);
      finish(0);
      exit(-1);
    }
    my_host = integrate_host(my_hostport);
    print_server_list();
    distribute_update();
    relinquish_add_lock(server_list);
  }
  heartbeat_dest = my_host->next;

  pthread_create(&runner_thread, NULL, (void *(*)(void *))runner, NULL);
  pthread_join(runner_thread, NULL);
}

host_list_node *integrate_host(host_port *host) {
  host_list_node *runner, *max;
  int max_distance, dist;
  max_distance = 0;
  runner = server_list->head;
  do {
    dist = distance(runner->host->location, runner->next->host->location);
    if(!dist) dist = HASH_SPACE_SIZE;
    if(max_distance < dist) {
      max = runner;
      max_distance = dist;
    }
    runner = runner->next;
  } while(runner != server_list->head);
  host->location = max->host->location + (max_distance/2);
  return add_to_host_list(host, max);
}


void finish(int sig) {
  printf("\nFinishing\n");
  close(*listener);
  free(listener);
  free_host_list(server_list,1);
  exit(0);
}

int heartbeat() {
  int connection, err;
  if(heartbeat_dest != my_host) {
    host_list *incoming;
    err = make_connection_with(heartbeat_dest->host->ip, heartbeat_dest->host->port, &connection);
    if (err < OKAY) {
      handle_host_failure(heartbeat_dest->host);
      return FAILURE;
    }
    err = HEARTBEAT;
    do_rpc(&err);
    receive_host_list(connection, &incoming);
    pthread_mutex_lock(&(my_host->lock));
    my_host->host->time_stamp++;
    pthread_mutex_unlock(&(my_host->lock));
    safe_send(connection, my_host->host, sizeof(host_port));
    update_job_counts(incoming);
    free_host_list(incoming, 1);
  }
  heartbeat_dest = heartbeat_dest->next;
}

int update_job_counts(host_list *update) {
  host_list_node *my_runner, *update_runner;
  my_runner = server_list->head;
  update_runner = update->head;
  do {
#ifdef CAREFUL
    if(strcmp(my_runner->host->ip, update_runner->host->ip)) {
      problem("Heartbeat update did not agree with our current serverlist.\n");
    }
#endif
    if(my_runner->host->time_stamp < update_runner->host->time_stamp) {
      my_runner->host->time_stamp = update_runner->host->time_stamp;
      my_runner->host->jobs = update_runner->host->jobs;
    }
    my_runner = my_runner->next;
    update_runner = update_runner->next;
  } while(my_runner != server_list->head);

  
}

int acquire_add_lock(host_list *list) {
  int err, connection;
  host_list_node *runner;
  runner = list->head;
  do {
    if(runner != my_host) {
      err = make_connection_with(runner->host->ip, runner->host->port, &connection);
      if (err < OKAY){
	handle_host_failure(runner->host);
	return FAILURE;
      }
      err = request_add_lock(connection);
      if (err < OKAY){
	relinquish_add_lock(list);
	return FAILURE;
      }
      close(connection);
      runner = runner->next;
    }
  }  while(runner != list->head);
  return OKAY;
}

int relinquish_add_lock(host_list *list) {
  int err, connection;
  host_list_node *runner;
  runner = list->head;
  do {
    if(runner != my_host) { //possibly needs to be changed give the current update structure
      //consider changing updates to work by sending only the identity of the current node?
      err = make_connection_with(runner->host->ip, runner->host->port, &connection);
      if (err < OKAY) {
	handle_host_failure(runner->host);
      } else {
	err = tell_to_unlock(connection);
      }
      close(connection);
    }
    runner = runner->next;
  }  while(runner != list->head);
  return OKAY;
}

int tell_to_unlock(int connection) {
  int num = UNLOCK;
  do_rpc(&num);
  num = FAILURE;
  safe_recv(connection, &num, sizeof(int));
  return num;
}

int request_add_lock(int connection) {
  int num = REQUEST_ADD_LOCK;
  do_rpc(&num);
  num = FAILURE;
  safe_recv(connection, &num, sizeof(int));
  return num;
}

int transfer_job(host_port *host, job *to_send) {
  int connection, err;
  err = make_connection_with(host->ip, host->port, &connection);
  if (err < OKAY) {
    handle_host_failure(host);
    return FAILURE;
  }
  err = TRANSFER_JOB;
  do_rpc(&err);
  err = safe_send(connection, to_send, sizeof(job));
  if (err < OKAY) {
    handle_host_failure(host);
    return FAILURE;
  }
  return 0;
}

int announce(int connection, host_port *send) {
  int status = ANNOUNCE;
  status = do_rpc(&status);
  if(status < 0){
    problem("Failed to acknowledge announce\n");
    handle_host_failure_by_connection(connection);
    return status;
  }
  status = safe_send(connection, send, sizeof(host_port));
  if(status < 0) handle_host_failure_by_connection(connection);
  return status;
}

int send_update(int connection) {
  int okay;
  int err = RECEIVE_UPDATE;
  err = do_rpc(&err);
  if (err < 0) return err;
  err = send_host_list(connection, server_list);
  if (err < 0) return err;
  err = safe_recv(connection, &okay, sizeof(int));
  if (err < 0) return err;
  if(okay) {
    problem("unsuccessful update\n");
  }
}

void distribute_update() {
  int err;
  int connection;
  host_list_node* current_node;
  current_node = server_list->head;

  do {
    if(strcmp(current_node->host->ip, my_host->host->ip)) {
      err = make_connection_with(current_node->host->ip, 
				 current_node->host->port, &connection);
      if (err < OKAY) handle_host_failure(current_node->host);
      err = announce(connection, my_host->host);
      close(connection);
    }
    current_node = current_node->next;
  } while(current_node != server_list->head);
}

int send_host_list(int connection, host_list *list) {
  int err, num;
  host_list_node *runner;
  host_port *hosts;
  runner = list->head;
  num = 0;

  //count number of hosts
  do {
    num++;
    runner = runner->next;
  } while(runner != list->head);
  hosts = malloc(sizeof(host_port)*num);
  runner = list->head;
  
  //pack the hosts into an array
  for(err = 0; err < num; err++) {
    host_port_copy(runner->host, &hosts[err]);
    runner = runner->next;
  }
  
  //send
  err = safe_send(connection, &num, sizeof(int));
  if(err < 0) return err;
  err = safe_send(connection, hosts, sizeof(host_port)*num);
  if(err < 0) return err;

  return OKAY;
}

int receive_host_list(int connection, host_list **list) {
  int err, num, i;
  host_port *hosts, *temp;
  host_list_node *runner;
  num = 0;
  //size of array
  err = safe_recv(connection, &num, sizeof(int));
  if(err < OKAY) return err;
  hosts = malloc(sizeof(host_port)*num);
  err = safe_recv(connection, hosts, sizeof(host_port)*num);
  if(err < OKAY) return err;
  
  //we malloc new host ports so that freeing is easy later
  //we cant just use the memory we allocated earlier as an array
  //or else it is impossible to free individual elements of the list
  temp = malloc(sizeof(host_port));
  host_port_copy(&hosts[0], temp);
  *list = new_host_list(temp);
  runner = (*list)->head;
  for(i = 1; i < num; i++) {
    temp = malloc(sizeof(host_port));
    host_port_copy(&hosts[i], temp);
    runner = add_to_host_list(temp, runner);
  }
  return OKAY;
}

void host_port_copy(host_port *src, host_port *dst) {
  dst->jobs = src->jobs;
  dst->time_stamp;
  dst->port = src->port;
  dst->location = src->location;
  strcpy(dst->ip, src->ip);
}

void free_host_list(host_list *list, int flag) {
  host_list_node *runner = list->head;
  host_list_node *prev;
  do {
    if(runner->host && flag) free(runner->host);
    prev = runner;
    runner = runner->next;
    free(prev);
  } while(runner != list->head) ;
  free(list);
}

host_list *new_host_list(host_port *initial_host_port) {
  host_list *new_list;
  new_list = malloc(sizeof(host_list));
  new_list->head = malloc(sizeof(host_list_node));
  new_list->head->host = initial_host_port;
  new_list->head->next = new_list->head;
  return new_list;
}

host_list_node *add_to_host_list(host_port *added_host_port, host_list_node *where_to_add) {
  host_list_node *new_node;
  new_node = (host_list_node *)malloc(sizeof(host_list_node));
  pthread_mutex_init(&(new_node->lock), NULL);
  new_node->next = where_to_add->next;
  new_node->host = added_host_port;
  where_to_add->next = new_node;
  return new_node;
}

void remove_from_host_list(host_port *removed_host_port, host_list *list) {
  host_list_node *current_node;
  host_list_node *node_to_remove;
  if(list->head->host == removed_host_port) {
    node_to_remove = list->head;
    list->head = node_to_remove->next;
    list->head->host->location = 0;
    free(node_to_remove);
    free(removed_host_port);
    return;}
  current_node = list->head;
  do { if (current_node->next->host == removed_host_port) { 
      node_to_remove = current_node->next;
      current_node->next = current_node->next->next; 
      free(node_to_remove);
      free(removed_host_port);
      return; }
    current_node = current_node -> next; }
    while(current_node != list->head) ;
}

host_port* find_host_in_list(char *hostname, host_list *list) {
  host_list_node *current_node;
  current_node = list->head;
  do { 
    if (!strcmp(current_node->host->ip,hostname)) return current_node->host;
    current_node = current_node->next;
  } while(current_node != list->head) ;
}

host_port* get_hostport_from_connection(int connection) {
  char failed_host_ip[INET_ADDRSTRLEN];
  get_ip(connection,failed_host_ip);
  return find_host_in_list(failed_host_ip,server_list);
}

void clone_host_list(host_list *old_list, host_list *new_list) {
  new_list = new_host_list(old_list->head->host);
  host_list_node *old_node;
  old_node = old_list->head;
  host_list_node *new_node;
  new_node = new_list->head;
  host_list_node *new_successor;
  while(old_node != old_list->head) { 
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

void handle_host_failure(host_port *failed_host) { // failed on a connect
  problem("%s at %d failed\n", failed_host->ip, failed_host->location);
  local_handle_failure(failed_host);
  notify_others_of_failure(failed_host);
}

void local_handle_failure(host_port *failed_host) {
  remove_from_host_list(failed_host,server_list);
  update_q_host_failed(failed_host,activeQueue); //backup queue is received by RPC
}

void notify_others_of_failure(host_port *failed_host) { // tell everyone
  int connection = 0;
  int err;
  host_list_node* current_node;
  current_node = server_list->head;

  do {
    if(strcmp(current_node->host->ip,my_host->host->ip)) {
#ifdef VERBOSE
      printf("Notifying %s and my ip is %s\n", current_node->host->ip,my_host->host->ip);
#endif
      err = make_connection_with(current_node->host->ip, 
					  current_node->host->port, &connection);
      if (err < OKAY) { 
	handle_host_failure(current_node->host);
      } else { 
	inform_of_failure(connection,failed_host);
      }
    }
    current_node = current_node->next;
  } while(current_node != server_list->head);
}

void inform_of_failure(int connection, host_port *failed_host) {
  int err = INFORM_OF_FAILURE;
  err = do_rpc(&err);
  if (err < 0) {
    handle_host_failure_by_connection(connection); 
    return; 
  }
  err = safe_send(connection,failed_host,sizeof(host_port));
  if (err < 0) {
    problem("failed to send");
    handle_host_failure_by_connection(connection);
    return; 
  }
}

void update_q_host_failed (host_port* failed_host, queue *Q) {
   job_list_node *current;
   current = Q->head;
   while(current != NULL) {
       current = current->next;
   } 
}

void print_server_list() {
  host_list_node* current_node;
  printf("Servers:\n");
  current_node = server_list->head;
  do {
    printf("\tIP: %s, port: %d, location: %d\n",
	   current_node->host->ip, current_node->host->port, current_node->host->location);
    current_node = current_node->next;
  } while(current_node != server_list->head);
}

void queue_setup() {
  activeQueue = (queue *)malloc(sizeof(queue));
  backupQueue = (queue *)malloc(sizeof(queue));
  activeQueue->head = NULL;
  backupQueue->head = NULL;
  activeQueue->tail = NULL;
  backupQueue->tail = NULL;
  pthread_mutex_init(&(activeQueue->head_lock), NULL); 
  pthread_mutex_init(&(activeQueue->tail_lock), NULL); 
  pthread_mutex_init(&(backupQueue->head_lock), NULL); 
  pthread_mutex_init(&(backupQueue->tail_lock), NULL); 
}

int get_servers(char *hostname, int port, int add_slots, host_list **list) {
  int connection = 0;
  int result = 0;
  int err;
  err = make_connection_with(hostname, port, &connection);
  if(err < 0) {
    problem("Failed to connect to retrive servers");
    return FAILURE;
  }
  err = SEND_SERVERS;
  do_rpc(&err);
  result = receive_host_list(connection, list);
  close(connection);
  return result;
}

void listener_set_up(host_port *info) {
  pthread_t thread;
  int connect_result;
  listener = malloc(sizeof(int));
  connect_result = set_up_listener(info->port, listener);
  pthread_create(&thread, NULL, (void *(*)(void *))listen_for_connection, listener);
}

void listen_for_connection(int *listener) {
  int connection, connect_result;
  pthread_t thread;
  connect_result = -1;
  do {
    connect_result = wait_for_connection(*listener, &connection);
  } while(connect_result < 0);
  pthread_create(&thread, NULL, (void *(*)(void *))
		listen_for_connection, listener);
  handle_rpc(connection);
}

void add_replica(host_port *host, job *rep_job) {
//  add_to_host_list(host,rep_job->replica_list);
}

int get_job_for_runner(job **ptr) {
  if(*ptr = get_local_job()) {
    return OKAY;
  }
  if(get_remote_job(ptr) < 0) {
    return FAILURE;
  }
  return OKAY;
}

job *get_local_job() {
  job_list_node *current_node;
  current_node = activeQueue->head;
  while (current_node && current_node->entry && current_node->entry->status != READY) {
    current_node = current_node->next;
  }
  if(current_node && current_node->entry) {
#ifdef SHOW_RUNNER_STATUS
	printfl("current_node was NULL OR entry was null");
#endif
    pthread_mutex_lock(&(current_node->lock));
    if(current_node->entry->status == READY) {
      pthread_mutex_lock(&(my_host->lock));
      my_host->host->jobs--;
#ifdef VERBOSE
      printfl("I have %d jobs", my_host->host->jobs);
#endif
      pthread_mutex_unlock(&(my_host->lock));
      current_node->entry->status = RUNNING;
      return current_node->entry;
    }
#ifdef SHOW_RUNNER_STATUS
    else {
      printfl("Status was not ready");
    }
#endif
    pthread_mutex_unlock(&(current_node->lock));
  }
  
  return NULL;
}

int get_remote_job(job **ptr) {
  int err;
  int status;
  int connection;
  host_port *server;
  do {
    server = find_job_server();
    if(!server || server == my_host->host) {
      //find_job_server only fails when no one has any jobs
      return FAILURE;
    }
    err = make_connection_with(server->ip, server->port, &connection);
    if (err < OKAY) { 
      handle_host_failure(server);
      return FAILURE;
    }
    err = SERVE_JOB;
    do_rpc(&err);
    status = FAILURE;
    err = safe_recv(connection, &status, sizeof(int));
    if(err < 0) {
      return err;
    }
    if(status < 0) {
      //set stuff up so we can try again
      server->jobs = 0;
      close(connection);
    }
  } while(status < 0);
  *ptr = malloc(sizeof(job));
  err = safe_recv(connection, *ptr, sizeof(job));
  close(connection);
  return OKAY;
}

host_port *find_job_server() {
  host_list_node *n;
  host_port *p;
  n = server_list->head;
  p = n->host;
  do {
    if (p->jobs < n->host->jobs) {
      p = n->host;
    }
    n = n-> next;
  } while(n != server_list->head);
  if(p->jobs) {
    return p;
  } else {
    return NULL;
  }
}

int inform_of_completion(job *completed) {
  return OKAY;
}

void add_to_queue(job *addJob, queue *Q) {
  job_list_node *n, *prev;

  n = (job_list_node *)malloc(sizeof(job_list_node));
  pthread_mutex_init(&(n->lock), NULL);

  pthread_mutex_lock(&(n->lock));
  pthread_mutex_lock(&(Q->tail_lock));
  
  
  n->next = NULL;
  n->entry = addJob;
  if(Q->tail) {
    Q->tail->next = n;
    Q->tail = n;
  } else {
    Q->tail = n;
    Q->head = n;
  }
  
  pthread_mutex_unlock(&(Q->tail_lock));
  pthread_mutex_unlock(&(n->lock));

  pthread_mutex_lock(&(my_host->lock));
  my_host->host->jobs++;
#ifdef VERBOSE
  printfl("I have %d jobs", my_host->host->jobs);
#endif
  pthread_mutex_unlock(&(my_host->lock));
}

