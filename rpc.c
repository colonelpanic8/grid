void handle_rpc(int connection) {
  char rpc[RPC_STR_LEN+1];
  char host[INET_ADDRSTRLEN], temp[INET_ADDRSTRLEN];
  get_ip(connection, host);
  recv_string(connection, rpc, RPC_STR_LEN);
  printf(BAR);
  printf("RPC %s from %s\n", rpc, host);
  printf(BAR);
  switch(atoi(rpc)) {
  case 0:
    rpc_send_servers(connection);
    break;
  case 1:
    rpc_serve_job(connection);
    break;
  case 2:
    rpc_inform_of_completion(connection);
    break;
  case 3:
    rpc_receive_update(connection);
    break;
  case 4:
    rpc_receive_job_copy(connection);
    break;
  default:
    break;
  }
  close(connection);
  print_server_list();
}

void rpc_send_servers(int connection) {
  send(connection, &num_servers, sizeof(int), 0);
  send(connection, (void*)server_list, num_servers*sizeof(host_ip), 0);
}

// Learns of new connection and adds to list of current servers. 

void rpc_receive_update(int connection){
  host_ip *name;
  char buffer[9], ip[INET_ADDRSTRLEN];
  int num_servers_temp;
  recv(connection, &num_servers, sizeof(int), 0);
  recv(connection, (void*)server_list, num_servers*sizeof(host_ip), 0);
}

int verify_update(host_ip *new, int nsize, host_ip* old, int osize) {
  
}

void rpc_receive_identity(int connection){
  host_ip *name;
  char buffer[9], ip[INET_ADDRSTRLEN];
  int i = 0;
  get_ip(connection, ip);
  recv_string(connection, buffer, 8);
  while( server_list[i].port != 0) { 
    if(!strcmp(server_list[i].ip, buffer))
      return;
    i++;
  }
  name = &server_list[i]; 
  name->port = atoi(buffer);
  strcpy(name->ip,ip);
}


void rpc_serve_job(int connection) {
}

void rpc_receive_job_copy(int connection){
}

// A runner on another server requests a job from this server.
void rpc_request_job(int connection) { 
  
}

void rpc_inform_of_completion(int connection) {
}

void failure_notify(host_ip *fail) {
  int connection = 0;
  int i;
  for(i = 0; i < num_servers; i++) {  
    if(fail != &server_list[i]) {
      bulletin_make_connection_with(server_list[i].ip, server_list[i].port, &connection);
    
    }
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

job *create_job(int num_files, char files[MAX_ARGUMENTS][BUFFER_SIZE], int *flags){
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
  char files[MAX_ARGUMENTS][BUFFER_SIZE], temp[BUFFER_SIZE];
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
