void handle_failure(char *ip, int flag) {
  host_list_node *failed_host_node_prev;
  host_port *failed_host;
  pthread_mutex_lock(&failure_mutex);
  
  if(failed_host_node_prev = find_prev_host_in_list(ip, server_list)) {
    failed_host = failed_host_node_prev->next->host;
    local_handle_failure(failed_host_node_prev);
#ifdef NOTIFY_OTHERS
    if(flag) {
      notify_others_of_failure(failed_host);
    }
#endif
  }

  pthread_mutex_unlock(&failure_mutex);
}

void local_handle_failure(host_list_node *failed_host_prev) {
  pthread_mutex_lock(&server_list_mutex);
  remove_from_host_list(failed_host_prev, server_list);
  //update_q_host_failed(failed_host, activeQueue); fix
  pthread_mutex_unlock(&server_list_mutex);
}

host_list_node *find_prev_host_in_list(char *hostname, host_list *list) {
  //this returns the prev of the item we want
  host_list_node *current_node;
  current_node = list->head;
  do { 
    if (!strcmp(current_node->next->host->ip,hostname))
      return current_node;
    current_node = current_node->next;
  } while(current_node != list->head);
  return NULL;
}

int remove_from_host_list(host_list_node *prev, host_list *list) {
  host_list_node *to_remove = prev->next;
  if(to_remove == list->head) {
    list->head = list->head->next;
    list->head->host->location = 0;
  }
  if(to_remove == my_host)
    problem("Removing myself from the server_list?... Should not happen\n");
  if(to_remove == heartbeat_dest) {
    heartbeat_dest = heartbeat_dest->next;
  }
  prev->next = to_remove->next;
  to_remove->next;
  
  if(failed_hosts) {
    add_node_to_host_list(to_remove, failed_hosts->head);
  } else {
    failed_hosts = new_host_list_by_node(to_remove);
  }
}

host_list_node *add_node_to_host_list(host_list_node *node, host_list_node *where_to_add) {
  node->next = where_to_add->next;
  where_to_add->next = node;
  return node;
}

host_list *new_host_list_by_node(host_list_node *node) {
  host_list *new_list;
  new_list = malloc(sizeof(host_list));
  new_list->head = node;
  new_list->head->next = node;
  return new_list;
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
				 current_node->host->port, 
				 &connection);
      if (err < OKAY) { 
	problem("Handle host failure in a handle host failure\n");
	//handle_failure(current_node->host->ip, 0);
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
    return; 
  }
  err = safe_send(connection,failed_host,sizeof(host_port));
  if (err < 0) {
    problem("handle_host_failure inside a handle_host failure\n");
    problem("failed to send\n");
    //handle_host_failure_by_connection(connection);
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
