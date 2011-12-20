void handle_failure(host_port *failed_host_c, int flag) {
  host_list_node *failed_host_node;
#ifdef VERBOSE
  printf("Call to handle_failure\n");
#endif
  
  pthread_mutex_lock(&failure_mutex);
  
  if(failed_host_node = find_host_in_list(failed_host_c->id, server_list)) {
    
#ifdef VERBOSE
    printf("Host failure %s (id: %d)\n", failed_host_c->ip, failed_host_c->id);
#endif

    local_handle_failure(failed_host_node);

#ifdef NOTIFY_OTHERS
    if(flag) {
      notify_others_of_failure(failed_host_c);
    }
#endif
    
  }

  pthread_mutex_unlock(&failure_mutex);
}

void local_handle_failure(host_list_node *failed_host) {
  int flag = 0;
  if(failed_host == my_host->next) {
    flag = 1;
  }
  remove_from_host_list(failed_host, server_list);
  if(flag) {
    update_q_host_failed();
  }
}


host_list_node *find_host_in_list(unsigned int id, host_list *list) {
  host_list_node *current_node;
  current_node = list->head;
  do { 
    if (id == current_node->host->id)
      return current_node;
    current_node = current_node->next;
  } while(current_node != list->head);
  return NULL;
}

int remove_from_host_list(host_list_node *to_remove, host_list *list) {
  host_list_node *prev = to_remove->prev;
  int flag = 0;
  if(to_remove == list->head) {
#ifdef VERBOSE
    printf("Head changed from %d to %d\n", list->head->host->id, list->head->next->host->id);
#endif
    list->head = list->head->next;
    list->head->host->location = 0;
    if(list->head == my_host) {
      flag = 1;
    }
  }
  if(to_remove == my_host)
    problem("Removing myself from the server_list?... Should not happen\n");
  if(to_remove == heartbeat_dest) {
    heartbeat_dest = my_host;
  }
  
  prev->next = to_remove->next;
  to_remove->next->prev = prev;
  
  if(failed_hosts) {
    add_node_to_host_list(to_remove, failed_hosts->head);
  } else {
    failed_hosts = new_host_list_by_node(to_remove);
  }
  if(flag) {
    redistribute_jobs(my_queue);
  }
}

host_list_node *add_node_to_host_list(host_list_node *node, host_list_node *where_to_add) {
  node->prev = where_to_add;
  node->next = where_to_add->next;
  node->next->prev = node;
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
    if(current_node != my_host) {
#ifdef VERBOSE
      printf("Notifying %s and my ip is %s\n", current_node->host->ip,my_host->host->ip);
#endif
      err = make_connection_with(current_node->host->ip, 
				 current_node->host->port, 
				 &connection);
      if (err < OKAY) { 
	problem("Handle host failure in a handle host failure\n");
	problem("failed to connect to %s, with id, %d\n", current_node->host->ip, current_node->host->id);
	print_server_list(server_list);
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
    problem("handle_failure inside a handle_host failure\n");
    problem("failed to send\n");
    //handle_host_failure_by_connection(connection);
    return; 
  }
}


