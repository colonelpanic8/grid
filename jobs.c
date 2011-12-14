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

job *get_local_job() { //dequeue local job
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

void print_job_queue(queue *Q) {
  
}

void redistribute_jobs(queue *Q) {
#ifdef GREEDY
#else
  job_list_node *runner = Q->head;
  host_list_node *dest;
  while(runner != NULL) {
     dest = determine_ownership(runner->entry);
     if(dest != my_host) {
       remove_job(runner->entry);
       transfer_job(dest->host, runner->entry);
       free_job_node(runner);
     }
     runner = runner->next;
  }
#endif
}

void free_queue(queue *Q) {
  job_list_node *runner = Q->head;
  if(runner) {
    while(runner->next) {
      if(runner->prev) {
	free_job_node(runner->prev);
      }
      runner = runner->next;    
    }
    free_job_node(runner);
  }
  pthread_mutex_destroy(&(Q->head_lock));
  pthread_mutex_destroy(&(Q->tail_lock));
  free(Q);
}

void free_job_node(job_list_node *item) {
  free(item->entry);
  pthread_mutex_destroy(&(item->lock));
  free(item);
}

int remove_job(job_list_node *item, queue *list) {
  if(item == list->head){
    pthread_mutex_lock(&(list->head_lock));
  } else {
    pthread_mutex_lock(&(item->prev->lock));
  }
  pthread_mutex_lock(&(item->lock));
  if(item == list->tail) {
    pthread_mutex_lock(&(item->prev->lock));
  } else {
    pthread_mutex_lock(&(item->next->lock));
  }  
  item->prev->next = item->next;
  item->next->prev = item->prev;
  if(item == list->head){
    list->head = item->next;
    pthread_mutex_unlock(&(list->head_lock));
  } else {
    pthread_mutex_unlock(&(item->prev->lock));
  }
  pthread_mutex_unlock(&(item->lock));
  if(item == list->tail) {
    list->tail = item->prev;
    pthread_mutex_unlock(&(item->prev->lock));
  } else {
    pthread_mutex_unlock(&(item->next->lock));
  }
}

int transfer_job(host_port *host, job *to_send) {
  int connection, err;
  err = make_connection_with(host->ip, host->port, &connection);
  if (err < OKAY) {
    handle_failure(host->ip, 1);
    return FAILURE;
  }
  err = TRANSFER_JOB;
  do_rpc(&err);
  err = safe_send(connection, to_send, sizeof(job));
  if (err < OKAY) {
    problem("Transfer Job failed\n");
    return FAILURE;
  }
  return 0;
}

int send_meta_data(job *ajob) {
  ajob->status = READY; //needs to be changed once we add dependencies
  host_list_node *dest = determine_ownership(ajob);
#ifdef GREEDY
  add_to_queue(ajob, activeQueue);
  pthread_mutex_lock(&(my_host->lock));
  my_host->host->jobs++;
#ifdef VERBOSE
  printfl("I have %d jobs", my_host->host->jobs);
#endif
  pthread_mutex_unlock(&(my_host->lock));
#else
  if(dest == my_host) {
    add_to_queue(ajob, activeQueue);
    
    //
    pthread_mutex_lock(&(my_host->lock));
    my_host->host->jobs++;
#ifdef VERBOSE
    printfl("I have %d jobs", my_host->host->jobs);
#endif
    pthread_mutex_unlock(&(my_host->lock));
    //

  } else {
    transfer_job(dest->host, ajob);
    free(ajob);
  }
#endif
}

host_list_node *determine_ownership(job *ajob) {
  char buffer[BUFFER_SIZE];
  int job_hash;
  host_list_node *runner;
  job_hash = hash(ajob->name, ajob->id);
#ifdef VERBOSE
  printf("%s, %d hashes to %d\n", ajob->name, ajob->id, job_hash);
#endif
  runner = server_list->head;
  while(runner->next->host->location <= job_hash && runner->next->host->location != 0) {
    runner = runner->next;
  }
#ifdef VERBOSE
  printf("So it belongs to %s\n", runner->host->ip);
#endif
  return runner;
}

int write_files(job *ajob, int num_files, data_size *files) {
  char buffer[BUFFER_SIZE], back[BUFFER_SIZE];
  FILE *temp;
  int i;
  sprintf(buffer,"./jobs/%d/", ajob->id); 
  if(mkdir(buffer, S_IRWXU)) {
    if(errno == EEXIST) {
#ifdef VERBOSE
      problem("directory already exists...\n");
#endif
    } else {
      problem("mkdir failed with %d\n", errno);
    }
  }
  for(i = 0; i < num_files; i++) {
    printf("%s\n", files[i].name); 
    sprintf(buffer,"jobs/%d/%s", ajob->id, files[i].name);
    temp = NULL;
    temp = fopen(buffer, "w");
    if(temp) {
      fwrite(files[i].data, files[i].size, 1, temp);
    } else {
      problem("failed to open file %s, errno: %d\n", files[i].name, errno);
    }
    fclose(temp);

    //
    char mode[] = "0777";
    int i;
    i = strtol(mode, 0, 8);
    if (chmod (buffer,i) < 0)
      {
	problem("chmod failed");
      }
    //
  }
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

void update_q_job_complete (int jobid, queue *Q) {
   job_list_node *current;
   current = Q->head;
   while(current != NULL) {
       if (contains(current->entry, jobid)) {
	 remove_dependency(current->entry, jobid);
         //check_avail(current->entry);
       }
       current = current->next;
   } 
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

