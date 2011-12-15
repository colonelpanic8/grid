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

pthread_mutex_t failure_mutex = PTHREAD_MUTEX_INITIALIZER;
host_list *failed_hosts = NULL;

int num_servers, *listener;
pthread_t *listener_thread = NULL;

host_list_node *my_host = NULL;
host_list_node *heartbeat_dest = NULL;

queue *activeQueue;
queue *backupQueue;

pthread_mutex_t d_add_mutex = PTHREAD_MUTEX_INITIALIZER;
int d_add_lock = UNLOCKED;
char who[INET_ADDRSTRLEN];

//Not really viewed as separate from server.c, simply to divide
//the functions so they are more easily viewed
#include "rpc.c"
#include "failure.c"
#include "jobs.c"

int main(int argc, char **argv) {
  char name[INET_ADDRSTRLEN];
  pthread_t runner_thread;
  host_port *my_hostport;  
  signal(SIGINT, finish);
  //setup jobs folder
  if(mkdir("./jobs", S_IRWXU)) {
    if(errno == EEXIST) {
#ifdef VERBOSE
      //problem("Jobs directory already exists...\n");
      //We probably don't need to see this any more at all
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
  printf("\nFinishing:\n");
  if(listener_thread) {
#ifdef VERBOSE
    printf("Killing listener thread.\n");
#endif
    pthread_kill(*listener_thread, SIGABRT);
  }
#ifdef VERBOSE
  printf("Closing listener socket.\n");
#endif
  close(*listener);
  free(listener);
#ifdef VERBOSE
  printf("Freeing queues.\n");
#endif
  free_queue(activeQueue);
  free_queue(backupQueue);
  if(server_list) {
#ifdef VERBOSE
    printf("Freeing server list.\n");
#endif
    free_host_list(server_list, 1);
  }
  if(failed_hosts) {
#ifdef VERBOSE
    printf("Freeing failed hosts list.\n");
#endif
    free_host_list(failed_hosts, 1);
  }
#ifdef VERBOSE
  printf("Destroying mutexes.\n");
#endif
  pthread_mutex_destroy(&count_mutex);
  pthread_mutex_destroy(&server_list_mutex);
  pthread_mutex_destroy(&failure_mutex);
  exit(0);
}

int heartbeat() {
  int connection, err;
  if(heartbeat_dest != my_host) {
    host_list *incoming;
    err = make_connection_with(heartbeat_dest->host->ip, heartbeat_dest->host->port, &connection);
    if (err < OKAY) {
      handle_failure(heartbeat_dest->host->ip, 1);
      return FAILURE;
    }
    err = HEARTBEAT;
    do_rpc(&err);
    receive_host_list(connection, &incoming);
    pthread_mutex_lock(&(my_host->lock));
    my_host->host->time_stamp++;
    my_host->host->jobs = activeQueue->active_jobs;
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
	handle_failure(runner->host->ip, 1);
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

int announce(int connection, host_port *send) {
  int status = ANNOUNCE;
  status = do_rpc(&status);
  if(status < 0){
    problem("Failed to acknowledge announce\n");
    problem("Send Failed\n");
    return status;
  }
  status = safe_send(connection, send, sizeof(host_port));
  if(status < 0) problem("Send Failed\n");
  return status;
}

int send_update(int connection) { //No longer used
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
      if (err < OKAY) handle_failure(current_node->host->ip, 1);
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
    memcpy(&hosts[err], runner->host, sizeof(host_port));
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
  memcpy(temp, &hosts[0], sizeof(host_port));
  *list = new_host_list(temp);
  runner = (*list)->head;
  for(i = 1; i < num; i++) {
    temp = malloc(sizeof(host_port));
    memcpy(temp, &hosts[i], sizeof(host_port));
    runner = add_to_host_list(temp, runner);
  }
  return OKAY;
}

void free_host_list(host_list *list, int flag) {
  host_list_node *runner = list->head;
  host_list_node *prev;
  do {
    if(runner->host && flag) free(runner->host);
    prev = runner;
    runner = runner->next;
    pthread_mutex_destroy(&(prev->lock));
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
  pthread_mutex_init(&(new_list->head->lock), NULL);
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

void print_server_list() {
  host_list_node* current_node;
  printf("Servers:\n");
  current_node = server_list->head;
  do {
    printf("\tIP: %s, port: %d, location: %d, jobs: %d, timestamp: %d\n",
	   current_node->host->ip, current_node->host->port,
	   current_node->host->location, current_node->host->jobs, current_node->host->time_stamp);
    current_node = current_node->next;
  } while(current_node != server_list->head);
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
  listener_thread = &thread;
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
  listener_thread = &thread;
  pthread_create(&thread, NULL, (void *(*)(void *))
		listen_for_connection, listener);
  handle_rpc(connection);
}
