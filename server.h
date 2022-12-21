//Start-Up
int get_servers(char *hostname, int port, int add_slots, host_list **list);
int acquire_add_lock(host_list *list);
void listener_set_up();
void listen_for_connection(int *listener);
void distribute_update();
int relinquish_add_lock(host_list *list);
int tell_to_unlock(int connection);

//General
int connect_to(host_list_node *server, int *connection);
void finish(int sig);
void pipe_error(int sig);
int print_method();
int heartbeat();

//Server List
host_list_node *integrate_host(host_port *host);
void print_server_list();
void add_host_to_list_by_location(host_port *host, host_list *list);
host_list_node *add_to_host_list(host_port *added_host_port, host_list_node *where_to_add);
host_list *new_host_list(host_port *initial_host_port);
host_port *get_hostport_from_connection(int connection);
int receive_host_list(int connection, host_list **list);
void free_host_list(host_list *list, int flag);
int send_host_list(int connection, host_list *list);
void host_port_copy(host_port *src, host_port *dst);

//RPC handles
void handle_rpc(int connection);
char *which_rpc(int rpc);
//RPCs
void rpc_heartbeat(int connection);
void rpc_serve_job(int connection);
void rpc_unlock(int connection);
void rpc_receive_announce(int connection);
void rpc_send_servers(int connection);
void rpc_request_job(int connection); 
void rpc_inform_of_completion(int connection);
void rpc_add_job(int connection);
void rpc_receive_update(int connection);
void rpc_receive_job_copy(int connection);
void rpc_inform_of_failure(int connection);
void rpc_request_add_lock(int connection);
void rpc_transfer_job(int connection);

//Queue/Jobs
void queue_setup();
void print_job_queue(queue *Q);
void init_queue(queue *Q);
int update_job_counts(host_list *update);
job *get_remote_job();
job *get_local_job();
job *get_job_for_runner();
host_port *find_job_server();
int redistribute_jobs(queue *Q);
int transfer_job(host_port *host, job *to_send);
int announce(int connection, host_port *host);
job_list_node *contains(unsigned int id, queue *Q);
int receive_file(int connection, data_size *file);
int get_job_id(job *ajob);
int inform_of_completion(job *completed);
int write_files(job *ajob, int num_files, data_size *files);
void copy_job(host_port *hip, job *cop_job);
void add_replica(host_port *host, job *rep_job);
host_list_node *determine_ownership(job *ajob);
void add_to_queue(job *addJob, queue *Q);
void add_node_to_queue(job_list_node *item, queue *Q);
void free_job_node(job_list_node *item);
void free_queue(queue *Q);
void update_job_count(queue *Q, int update);
void replicate_my_jobs();

// linked list and host-port handling

// failure functions
void handle_failure(host_port *failed_host_c, int flag);
void local_handle_failure(host_list_node *failed_host);
host_list_node *find_host_in_list(unsigned int id, host_list *list);
int remove_from_host_list(host_list_node *to_remove, host_list *list);
host_list_node *add_node_to_host_list(host_list_node *added_node, host_list_node *where_to_add);
host_list *new_host_list_by_node(host_list_node *node);
void notify_others_of_failure(host_port *failed_host);
void inform_of_failure(int connection, host_port *failed_host);
void update_q_host_failed ();


int replicate_job(job *to_send);
int send_meta_data(job *ajob);
int remove_job(job_list_node *item, queue *list);
int request_add_lock(int connection);
