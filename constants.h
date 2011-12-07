#define problem(...)       fprintf(stderr, __VA_ARGS__)

#define BAR "------------------------------------------------------------\n"
#define RPC_STR_LEN 1
#define MAXIMUM_NODES 100
#define NUM_REPLICAS 1
#define BUFFER_SIZE 256
#define MAX_ARGUMENTS 10
#define MAX_ARGUMENT_LEN 20

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
