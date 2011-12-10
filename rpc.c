void handle_rpc(int connection) {
  int rpc;
  char host[INET_ADDRSTRLEN];
  get_ip(connection, host);
  safe_recv(connection, &rpc, sizeof(rpc));
  printf(BAR);
  printf("RPC %d from %s\n", rpc, host);
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
  default:
    break;
  }
  close(connection);
  print_server_list();
}

void rpc_send_servers(int connection) {
  safe_send(connection, &num_servers, sizeof(int));
  safe_send(connection, (void*)server_list, num_servers*sizeof(host_port));
}


void rpc_receive_update(int connection){
  host_list *new;
  new = malloc(sizeof(host_list));
  int err;
  err = receive_host_list(connection, new);
  if(verify_update(new, server_list) | err) {
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
    strcpy(ajob->input_files[i], files[i]);
  }
  return ajob;
}

void rpc_add_job(int connection) {
  job *ajob;
  int num_file, i, flags[MAX_ARGUMENTS];
  char files[MAX_ARGUMENTS][MAX_ARGUMENT_LEN], temp[BUFFER_SIZE];
  recv_string(connection, temp, BUFFER_SIZE-1);
  num_file = atoi(temp);
  for(i = 0; i < num_file; i++) {
    recv_string(connection, files[i], BUFFER_SIZE-1);
    recv_string(connection, temp, BUFFER_SIZE-1);
    flags[i] = atoi(temp);
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

void failure_notify(host_port *fail) {
  int connection = 0;
  int i;

  host_list_node* current_node;
  current_node = server_list->head;

  while(current_node != NULL) {
   bulletin_make_connection_with(current_node->host->ip, current_node->host->port, &connection);
   // send some RPC to notify
   current_node = current_node->next;
  }
}

// If a job is complete then we need to update the queue to make jobs that depended on that job available.

void update_q_job_complete (int jobid, queue *Q) {
   node_j *current;
   current = Q->head;
   while(current != NULL) {
       if (contains(current->obj, jobid)) {
	 remove_dependency(current->obj, jobid);
         check_avail(current->obj);
       }
       current = current->next;
   } 
}

int contains(job *current, int jobid) {
  int i = 0;
  while (i < current->number_inputs) {
    if (jobid == current->dependent_on[i]) return 1;
    i++;
  }
  return 0;
}

void check_avail(job *current) {
  int i = 0;
  int flag = 0;
  while (i < current->number_inputs && flag != 1) {
    if (current->dependent_on[i] != 0) flag = 1;
    i++;
  }
  if (flag == 0) current->inputs_available = 1;
}

void remove_dependency(job *current, int jobid) {
  int i = 0;
  while (i < current->number_inputs) {
    if (jobid == current->dependent_on[i]) {
      current->dependent_on[i] = 0;
      break;
    }
    i++;
  }
}

void add_to_queue(job *addJob, queue *Q) {
  node_j *n; 
  n = (node_j *)malloc(sizeof(node_j));
  n->obj = addJob;
  n->next = Q->head;
  Q->head = n;
}

void add_job(job *addJob) {
  replicate(addJob);
  add_to_queue(addJob, activeQueue); 
}
