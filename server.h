void send_identity(int connection);
int get_servers(char *hostname, int port, int add_slots, host_list **list);
void listen_for_connection(int *listener);
int send_update(int connection);
void distribute_update();
void listener_set_up();
void print_server_list();
void finish();
int acquire_add_lock(host_list *list);
int integrate_host(host_port *host);
int relinquish_add_lock(host_list *list);
int tell_to_unlock(int connection);


//RPC
void handle_rpc(int connection);
char *which_rpc(int rpc);

void rpc_unlock(int connection);
void rpc_receive_announce(int connection);
void rpc_serve_job(int connection);
void rpc_send_servers(int connection);
void rpc_request_job(int connection); 
void rpc_inform_of_completion(int connection);
void rpc_receive_update(int connection);
void rpc_add_job(int connection);
void rpc_receive_update(int connection);
void rpc_receive_job_copy(int connection);
void rpc_inform_of_failure(int connection);
void rpc_request_add_lock(int connection);
void rpc_transfer_job(int connection);


int get_remote_job(job **a_job);
job *get_local_job();
host_port *find_job_server();
int transfer_job(host_port *host, job *to_send);
int announce(int connection, host_port *host);
void failure_notify(host_port *fail);
void update_q_job_complete (int jobid, queue *Q);
int contains(job *current, int jobid);
void remove_dependency(job *current, int jobid);
void replicate(job *rep_job);
int receive_file(int connection, data_size *file);
int get_job_id(job *ajob);
int write_files(job *ajob, int num_files, data_size *files);
void copy_job(host_port *hip, job *cop_job);
void selectHost(job *copy_job);
void add_replica(host_port *host, job *rep_job);
void add_to_queue(job *addJob, queue *Q);
void queue_setup();


// linked list and host-port handling
host_list_node *determine_ownership(job *ajob);
void add_host_to_list_by_location(host_port *host, host_list *list);
host_list_node *add_to_host_list(host_port *added_host_port, host_list_node *where_to_add);
void clone_host_list(host_list *old_list, host_list *new_list);
host_port* find_host_in_list(char *hostname, host_list *list);
host_port* get_hostport_from_connection(int connection);
host_list *new_host_list();
void remove_from_host_list(host_port *removed_host_port, host_list *list);
host_port* find_host_in_list(char *hostname, host_list *list);
int receive_host_list(int connection, host_list **list);
void free_host_list(host_list *list, int flag);
int send_host_list(int connection, host_list *list);
void host_port_copy(host_port *src, host_port *dst);

// failure functions
void handle_host_failure_by_connection(int connection);
void handle_host_failure(host_port *failed_host);
void local_handle_failure(host_port *failed_host);
void notify_others_of_failure(host_port *failed_host);
void inform_of_failure(int connection, host_port *failed_host);
void update_q_host_failed (host_port* failed_host, queue *Q);
void replace_host_in_replica_list(host_port* failed_host, job* job);

// job handling functions
void fix_ownership (job_list_node *job);
void send_job(job *job_to_send,int connection);
void rpc_serve_job(int connection);
