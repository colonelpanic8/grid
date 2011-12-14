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
      handle_failure(server->ip, 1);
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
}
