void handle_rpc(int connection) {
  int rpc;
  char host[INET_ADDRSTRLEN];
  get_ip(connection, host);
  safe_recv(connection, &rpc, sizeof(rpc));
#ifdef SHOW_HEARTBEAT
  printf(BAR);
  printf("RPC %s from %s\n", which_rpc(rpc), host);
  printf(BAR);
#else
  if(rpc != HEARTBEAT) {
    printf(BAR);
    printf("RPC %s from %s\n", which_rpc(rpc), host);
    printf(BAR);
  }
#endif
  switch(rpc) {
  case SEND_SERVERS:
    rpc_send_servers(connection);
    break;
  case SERVE_JOB:
    rpc_serve_job(connection);
    break;
  case JOB_COMPLETE:
    rpc_inform_of_completion(connection);
    break;
  case RECEIVE_JOB_COPY:
    rpc_receive_job_copy(connection);
    break;
  case RECEIVE_UPDATE:
    rpc_receive_update(connection);
    break;
  case INFORM_OF_FAILURE:
    rpc_inform_of_failure(connection);
    break;
  case REQUEST_ADD_LOCK:
    rpc_request_add_lock(connection);
    break;
  case ADD_JOB:
    rpc_add_job(connection);
    break;
  case ANNOUNCE:
    rpc_receive_announce(connection);
    break;
  case UNLOCK:
    rpc_unlock(connection);
    break;
  case TRANSFER_JOB:
    rpc_transfer_job(connection);
    break;
  case HEARTBEAT:
    rpc_heartbeat(connection);
  default:
    break;
  }
  close(connection);
#ifdef SHOW_HEARTBEAT
  print_server_list();
#else
  if(rpc != HEARTBEAT) { 
    print_server_list();
  }
#endif
}

char *which_rpc(int rpc) {
  switch(rpc) {
  case SEND_SERVERS:
    return SEND_SERVERS_S;
    break;
  case SERVE_JOB:
    return SERVE_JOB_S;
    break;
  case JOB_COMPLETE:
    return JOB_COMPLETE_S;
    break;
  case RECEIVE_JOB_COPY:
    return RECEIVE_JOB_COPY_S;
    break;
  case RECEIVE_UPDATE:
    return RECEIVE_UPDATE_S;
    break;
  case INFORM_OF_FAILURE:
    return INFORM_OF_FAILURE_S;
    break;
  case REQUEST_ADD_LOCK:
    return REQUEST_ADD_LOCK_S;
  case ADD_JOB:
    return ADD_JOB_S;
    break;
  case ANNOUNCE:
    return ANNOUNCE_S;
    break;
  case UNLOCK:
    return UNLOCK_S;
    break;
  case TRANSFER_JOB:
    return TRANSFER_JOB_S;
    break;
  case HEARTBEAT:
    return HEARTBEAT_S;
  default:
    break;
  }
}

void rpc_heartbeat(int connection) {
  host_list *incoming;
  host_port *host;
  my_host->host->jobs = my_queue->active_jobs;
  host = malloc(sizeof(host_port));
  send_host_list(connection, server_list);
  safe_recv(connection, host, sizeof(host_port));
  free(host);
}

void rpc_transfer_job(int connection) {
  int err;
  job *incoming;
  incoming = malloc(sizeof(job));
  err = safe_recv(connection, incoming, sizeof(job));
  if(err < 0) {
    problem("Job transfer failed.\n");
  } else {
    add_to_queue(incoming, my_queue);
  }
}

void rpc_receive_job_copy(int connection) {
  int err;
  job *incoming;
  incoming = malloc(sizeof(job));
  err = safe_recv(connection, incoming, sizeof(job));
  if(err < 0) {
    problem("Job transfer failed.\n");
  } else {
    add_to_queue(incoming, backup_queue);
  }
}

void rpc_receive_announce(int connection) {
  int status;
  host_port *incoming;
  incoming = malloc(sizeof(host_port));
  status = safe_recv(connection, incoming, sizeof(host_port));
  add_host_to_list_by_location(incoming, server_list);
  if(my_host->prev->host == incoming) { //We only need to redistribute when the host added is our prev
    redistribute_jobs(my_queue);
  }
}

void add_host_to_list_by_location(host_port *host, host_list *list) {
  host_list_node *runner = list->head;
  while(runner->next->host->location < host->location && runner->next->host->location != 0) {
    runner = runner->next;
  }
  add_to_host_list(host, runner);
}

void rpc_request_add_lock(int connection) {
  int result = FAILURE;
  pthread_mutex_lock(&d_add_mutex);
  if(!d_add_lock) {
    d_add_lock = LOCKED;
    get_ip(connection, who);
    result = OKAY;
  }
  pthread_mutex_unlock(&d_add_mutex);
  safe_send(connection, &result, sizeof(int));
}

void rpc_unlock(int connection) {
  int result = FAILURE;
  char ip[INET_ADDRSTRLEN];
  get_ip(connection, ip);
  pthread_mutex_lock(&d_add_mutex);
  if(!strcmp(who, ip)) {
    d_add_lock = UNLOCKED;
    result = OKAY;
  }
  pthread_mutex_unlock(&d_add_mutex);
  safe_send(connection, &result, sizeof(int));
}

void rpc_send_servers(int connection) {
  send_host_list(connection, server_list);
}

void rpc_receive_update(int connection){
  host_list *old = server_list;
  int err;
  err = receive_host_list(connection, &server_list);
  if(err < 0) { 
    problem("Receive Update failed\n");
    return; //Fix?
  }
  err = OKAY;
  safe_send(connection, &err, sizeof(int));
  free(old);
}

void rpc_add_job(int connection) {
  int num_files, i, err;
  data_size *files;
  job *new;
  new = malloc(sizeof(job));
  err = safe_recv(connection, new, sizeof(job));
  if(err < 0) problem("Recv job failed, %d\n",err);
  
  safe_recv(connection, &num_files, sizeof(int));
  if(num_files > 0) {
    files = malloc(sizeof(data_size)*num_files);
    for(i = 0; i < num_files; i++) {
      receive_file(connection, &files[i]);
    }
  }

  i = get_job_id(new);
  write_files(new, num_files, files);
  send_meta_data(new);
  safe_send(connection, &i, sizeof(int));
  
  if(num_files > 0) {
    for(i = 0; i < num_files; i++) {
      free(files[i].data);
    }
    free(files);
  }
}

int get_job_id(job *ajob) {
  pthread_mutex_lock(&count_mutex);
  ajob->id = my_host->host->location + counter;
  counter++;
  pthread_mutex_unlock(&count_mutex);
  return ajob->id;
}

int receive_file(int connection, data_size *file) {
  int err;
  err = safe_recv(connection, &(file->size), sizeof(size_t));
  if(err < 0) problem("Recv failed size\n");
  file->data = malloc(file->size);
  err = safe_recv(connection, file->data, file->size);
  if(err < 0) problem("Recv failed data\n");
  err = safe_recv(connection, file->name, MAX_ARGUMENT_LEN*sizeof(char));
  if(err < 0) problem("Recv failed name\n");
}

void rpc_serve_job(int connection) {
  int err;
  int msg = FAILURE;
  job *ajob = get_local_job();
  if(ajob) {
    msg = OKAY;
#ifdef VERBOSE
    printf("Sending job %s, %d\n", ajob->name, ajob->id);
#endif
    safe_send(connection, &msg, sizeof(int));
    safe_send(connection, ajob, sizeof(job));
    return;
  }
  do_rpc(&msg);
}


void rpc_inform_of_completion(int connection) {
  int err = OKAY;
  int jobid;
  err = safe_recv(connection,&jobid,sizeof(int));  
  if (err < OKAY) return;

  update_q_job_complete(jobid,backup_queue);
}


void rpc_inform_of_failure(int connection) {
  int err = OKAY;
  host_port *received_hp;
  host_list_node *failed_host;
  received_hp = malloc(sizeof(host_port));
  err = safe_recv(connection,received_hp,sizeof(host_port));  
  if (err < OKAY){
    problem("did not receive failed host");
    return;
  }
  
  handle_failure(received_hp->ip, 0);
}

// If a job is complete then we need to update the queue to make jobs that depended on that job available.

int contains(job *current, int jobid) {
  int i = 0;
  while (i < current->argc) {
    if (jobid == current->dependent_on[i]) return 1;
    i++;
  }
  return 0;
}


void remove_dependency(job *current, int jobid) {
  int i = 0;
  while (i < current->argc) {
    if (jobid == current->dependent_on[i]) {
      current->dependent_on[i] = 0;
      break;
    }
    i++;
  }
}


