#define problem(...)       fprintf(stderr, __VA_ARGS__)
#define printfl(...)       printf(__VA_ARGS__); printf("\n")

#define BAR "------------------------------------------------------------\n"
#define RPC_STR_LEN 1
#define MAXIMUM_NODES 100
#define NUM_REPLICAS 2
#define BUFFER_SIZE 256
#define MAX_ARGUMENTS 10
#define MAX_ARGUMENT_LEN 20
#define MAX_JOBS 5 //per submission

#define TRANSMISSION_ERROR (-5)
#define RECEIVER_ERROR (-6)

#define OKAY 0
#define FAILURE -1 


//RPCs
#define SEND_SERVERS 0
#define RECEIVE_UPDATE 1
#define SERVE_JOB 2
#define JOB_COMPLETE 3
#define RECEIVE_JOB_COPY 4
#define INFORM_OF_FAILURE 5

//Job Status
#define NOT_READY -1
#define READY 0
#define RUNNING 1
#define COMPLETED 2

typedef struct _host_port {
  int port;
  char ip[INET_ADDRSTRLEN];
} host_port;

typedef struct _host_list_node {
  host_port *host;
  struct _host_list_node *next;
} host_list_node;

typedef struct _host_list {
  host_list_node *head;
} host_list;

typedef struct _job{
  int id, number_inputs, status, dependent_on[MAX_ARGUMENTS];
  char input_files[MAX_ARGUMENTS][MAX_ARGUMENT_LEN], *outputFile;
  host_list *replica_list;
  int inputs_available;
} job;

typedef struct _job_node {
  job *entry;
  struct _job_node *next;
  pthread_mutex_t lock;
} job_list_node;

typedef struct _queue{
  job_list_node *head; 
  //job_node *tail;
} queue;
