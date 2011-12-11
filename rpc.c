void handle_rpc(int connection) {
  int rpc;
  char host[INET_ADDRSTRLEN];
  get_ip(connection, host);
  safe_recv(connection, &rpc, sizeof(rpc));
  printf(BAR);
  printf("RPC %s from %s\n", which_rpc(rpc), host);
  printf(BAR);
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
  case RECEIVE_UPDATE:
    rpc_receive_update(connection);
    break;
  case RECEIVE_JOB_COPY:
    rpc_receive_job_copy(connection);
    break;
  case INFORM_OF_FAILURE:
    rpc_inform_of_failure(connection);
    break;
  case REQUEST_ADD_LOCK:
    rpc_request_add_lock(connection);
  case ADD_JOB:
    rpc_add_job(connection);
  default:
    break;
  }
  close(connection);
  print_server_list();
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
  case RECEIVE_UPDATE:
    return RECEIVE_UPDATE_S;
    break;
  case RECEIVE_JOB_COPY:
    return RECEIVE_JOB_COPY_S;
    break;
  case INFORM_OF_FAILURE:
    return INFORM_OF_FAILURE_S;
    break;
  case REQUEST_ADD_LOCK:
    return REQUEST_ADD_LOCK_S;
  default:
    break;
  }
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

void rpc_send_servers(int connection) {
  send_host_list(connection, server_list);
}


void rpc_receive_update(int connection){
  host_list *new;
  int err;
  err = receive_host_list(connection, &new);
  if(err < 0) return; //Fix?
  if(verify_update(new, server_list)) {
    err = FAILURE;
    safe_send(connection, &err, sizeof(int));
  } else {
    err = OKAY;
    safe_send(connection, &err, sizeof(int));
    free(server_list);
    server_list = new;
  }
}

job *create_job(int num_files, char files[MAX_ARGUMENTS][MAX_ARGUMENT_LEN], int *flags){
  int i;
  job *ajob;
  ajob = malloc(sizeof(job));
  memset(ajob, 0 , sizeof(ajob));
  for(i = 0; i < num_files; i++) {
    if(flags[i]) {
      ajob->dependent_on[i] = atoi(files[i]);
    }
    strcpy(ajob->argv[i], files[i]);
  }
  return ajob;
}

void rpc_add_job(int connection) {
  job *ajob;
  int num_file, i, flags[MAX_ARGUMENTS];
  char files[MAX_ARGUMENTS][MAX_ARGUMENT_LEN], temp[BUFFER_SIZE];
  num_file = atoi(temp);
  for(i = 0; i < num_file; i++) {
    recv_string(connection, temp, BUFFER_SIZE-1);
  }
  close(connection);
  ajob = create_job(num_file, files, flags);
  replicate(ajob);
  add_to_queue(ajob, activeQueue); 
}

int verify_update(host_list *new, host_list *old) {
  return 0;
}

void rpc_serve_job(int connection) {
  
}

void rpc_receive_job_copy(int connection){
}

void rpc_request_job(int connection) { 
  
}

void rpc_inform_of_completion(int connection) {
}

void rpc_inform_of_failure(int connection) {
  int err = OKAY;
  host_port *received_hp, *failed_host;
  err = safe_recv(connection,received_hp,sizeof(host_port));  
  if (err < OKAY) return;

  failed_host = find_host_in_list(received_hp->ip,server_list);
  local_handle_failure(failed_host);
}

// If a job is complete then we need to update the queue to make jobs that depended on that job available.

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

void add_to_queue(job *addJob, queue *Q) {
  job_list_node *n; 
  n = (job_list_node *)malloc(sizeof(job_list_node));
  n->entry = addJob;
  n->next = Q->head;
  Q->head = n;
}

void add_job(job *addJob) {
  replicate(addJob);
  add_to_queue(addJob, activeQueue); 
}
