typedef struct _host_ip {
  int port;
  char ip[INET_ADDRSTRLEN];
} host_ip;

typedef struct _job{
  int id, number_inputs, dependent_on[MAX_ARGUMENTS];
  char input_files[MAX_ARGUMENTS][MAX_ARGUMENT_LEN], *outputFile;
  host_ip replica_list[NUM_REPLICAS];
  int inputs_available;
} job;

typedef struct _node_j {
  job *obj; 
  struct _node_j *next;
} node_j;

typedef struct _queue{
  node_j *head; 
  node_j *tail;
} queue;
    
void send_identity(int connection);
void get_my_ip(char *buffer);
int get_servers(char *hostname, int port, int add_slots, host_ip **dest);
void listen_for_connection(int *listener);
void handle_rpc(int connection);
void send_update(int connection);
void distribute_identity();
void listener_set_up();
void print_server_list();

void rpc_serve_job();
void rpc_send_servers(int connection);
void rpc_request_job(int connection); 
void rpc_inform_of_completion(int connection);
void rpc_receive_update(int connection);
void rpc_add_job(int connection);
void rpc_receive_update(int connection);
void rpc_receive_job_copy(int connection);

int verify_update(host_ip *new, int nsize, host_ip* old, int osize);
void failure_notify(host_ip *fail);
void update_q_job_complete (int jobid, queue *Q);
int contains(job *current, int jobid);
job *create_job(int num_files, char files[MAX_ARGUMENTS][BUFFER_SIZE], int *flags);
void remove_dependency(job *current, int jobid);
void check_avail(job *current);
void replicate(job *rep_job);
void copy_job(host_ip *hip, job *cop_job);
void selectHost(job *copy_job);
void add_replica(host_ip host, job *rep_job);
void add_to_queue(job *addJob, queue *Q);
void add_job(job *addJob);
