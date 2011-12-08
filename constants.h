#define problem(...)       fprintf(stderr, __VA_ARGS__)
#define printfl(...)       printf(__VA_ARGS__); printf("\n")

#define BAR "------------------------------------------------------------\n"
#define RPC_STR_LEN 1
#define MAXIMUM_NODES 100
#define NUM_REPLICAS 1
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

typedef struct _host_port {
  int port;
  char ip[INET_ADDRSTRLEN];
} host_port;

typedef struct _job{
  int id, number_inputs, dependent_on[MAX_ARGUMENTS];
  char input_files[MAX_ARGUMENTS][MAX_ARGUMENT_LEN], *outputFile;
  host_port replica_list[NUM_REPLICAS];
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
