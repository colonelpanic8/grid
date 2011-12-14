//These are used to control program behavior
#define VERBOSE
#define SHOW_RUNNER_STATUS
#undef SHOW_RUNNER_STATUS
#define GREEDY
#undef GREEDY
#define ENABLE_HEARTBEAT
#define NOTIFY_OTHERS
#define RUN_JOBS

#define do_rpc(...) safe_send(connection, __VA_ARGS__, sizeof(int))
#define problem(...)       fprintf(stderr, __VA_ARGS__)
#define printfl(...)       printf(__VA_ARGS__); printf("\n")

#define BAR "------------------------------------------------------------\n"
#define RPC_STR_LEN 1
#define MAXIMUM_NODES 100
#define NUM_REPLICAS 2
#define BUFFER_SIZE 256
#define MAX_ARGUMENTS 10
#define MAX_ARGUMENT_LEN 40
#define MAX_JOBS 5 //per submission

#define TRANSMISSION_ERROR (-5)
#define RECEIVER_ERROR (-6)

#define OKAY 0
#define FAILURE (-1)


//RPCs
#define SEND_SERVERS 0
#define SEND_SERVERS_S "Send Servers"

#define RECEIVE_UPDATE 1
#define RECEIVE_UPDATE_S "Receive Update"

#define SERVE_JOB 2
#define SERVE_JOB_S "Serve Job"

#define JOB_COMPLETE 3
#define JOB_COMPLETE_S "Job Complete"

#define RECEIVE_JOB_COPY 4
#define RECEIVE_JOB_COPY_S "Receive Job Copy"

#define INFORM_OF_FAILURE 5
#define INFORM_OF_FAILURE_S "Inform of Failure"

#define REQUEST_ADD_LOCK 6
#define REQUEST_ADD_LOCK_S "Request Add Lock"

#define RECEIVE_IDENTITY 7
#define RECEIVE_IDENTITY_S "Receive Identity"

#define ADD_JOB 8
#define ADD_JOB_S "Add Job"

#define ANNOUNCE 9
#define ANNOUNCE_S "Announce"

#define UNLOCK 10
#define UNLOCK_S "Unlock"

#define TRANSFER_JOB 11
#define TRANSFER_JOB_S "Transfer Job"

#define HEARTBEAT 12
#define HEARTBEAT_S "Heartbeat"

//Job Status
#define NOT_READY 3
#define READY 0
#define RUNNING 1
#define COMPLETED 2

//runner

typedef struct _host_port {
  int port, jobs;
  unsigned int location, time_stamp;
  char ip[INET_ADDRSTRLEN];
} host_port;

typedef struct _host_list_node {
  host_port *host;
  struct _host_list_node *next;
  pthread_mutex_t lock;
} host_list_node;

typedef struct _host_list {
  host_list_node *head;
} host_list;

typedef struct _job{
  int id, argc, status, dependent_on[MAX_ARGUMENTS];
  char name[MAX_ARGUMENT_LEN], argv[MAX_ARGUMENTS][MAX_ARGUMENT_LEN];
} job;

typedef struct _job_node {
  job *entry;
  struct _job_node *next;
  struct _job_node *prev;
  pthread_mutex_t lock;
} job_list_node;

typedef struct _queue{
  job_list_node *head;
  job_list_node *tail;
  pthread_mutex_t head_lock;
  pthread_mutex_t tail_lock;
} queue;

typedef struct _data_size {
  char *data;
  size_t size;
  char name[MAX_ARGUMENT_LEN];
} data_size;
