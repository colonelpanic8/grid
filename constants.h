#define TESTING                 //Enable to make the configureable settings below take effect.
//#undef  TESTING

#ifdef TESTING

#define VERBOSE                 //Adds error messages, and general logging all over the place
//#undef  VERBOSE

#define VERBOSE2                //Adds extremely superflous log messages
#undef  VERBOSE2

#define SHOW_RUNNER_STATUS      //Pretty annoying, probably best to leave off except for debugging
#undef  SHOW_RUNNER_STATUS

#define GREEDY                  //Servers will take jobs that are added to them no matter where they should go
#undef  GREEDY

#define ENABLE_HEARTBEAT        //Turns heartbeat on or off
//#undef  ENABLE_HEARTBEAT

#define SHOW_HEARTBEAT          //Pretty annoying as well, shows every heartbeat message
#undef  SHOW_HEARTBEAT

#define NOTIFY_OTHERS           //...of failure
#undef  NOTIFY_OTHERS

#define RUN_JOBS                //Controls whether or not the runner thread actually runs jobs.
//#undef  RUN_JOBS

#define RUN_LOCAL               //When this is undefined, only remote jobs will be processed
//#undef  RUN_LOCAL

#define DEPENDENCIES            //Enables dependency handling
#undef  DEPENDENCIES

#define NO_DEQUEUE              //This prevents the runner from ever dequeing jobs, so that we can see what happens when nodes maintain large queues
//#undef  NO_DEQUEUE

#else

#define ENABLE_HEARTBEAT
#define NOTIFY_OTHERS
#define RUN_JOBS
#define RUN_LOCAL
#define DEPENDENCIES

#endif

//Macros
#define do_rpc(...)        safe_send(connection, __VA_ARGS__, sizeof(int))
#define problem(...)       fprintf(stderr, __VA_ARGS__)
#define printfl(...)       printf(__VA_ARGS__); printf("\n")

#define RUNNER_REST 1 //How long the runner waits between heartbeats (in seconds)
#define DISPLAY 5 //How many heartbeats to wait to display info

#define BAR "--------------------------------------------------------------------------------------------------------------\n"
#define RPC_STR "-/R/P/C/- "
#define NUM_REPLICAS 2
#define BUFFER_SIZE 256
#define MAX_ARGUMENTS 10
#define MAX_ARGUMENT_LEN 40

#define EXIT "exit\n"

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


//Data Types
typedef struct _host_port {
  unsigned int location, time_stamp, id, port, jobs;
  char ip[INET_ADDRSTRLEN];
} host_port;

typedef struct _host_list_node {
  host_port *host;
  struct _host_list_node *next;
  struct _host_list_node *prev;
  pthread_mutex_t lock;
} host_list_node;

typedef struct _host_list {
  host_list_node *head;
  unsigned int id;
} host_list;

typedef struct _job{
  unsigned int id;
  int argc, status, dependent_on[MAX_ARGUMENTS];
  char name[MAX_ARGUMENT_LEN], argv[MAX_ARGUMENTS][MAX_ARGUMENT_LEN];
} job;

typedef struct _job_node {
  job *entry;
  struct _job_node *next;
  struct _job_node *prev;
  pthread_mutex_t lock;
} job_list_node;

typedef struct _queue{
  int active_jobs;
  job_list_node *head;
  job_list_node *tail;
  pthread_mutex_t active_jobs_lock;
  pthread_mutex_t head_lock;
  pthread_mutex_t tail_lock;
} queue;

typedef struct _data_size {
  char *data;
  size_t size;
  char name[MAX_ARGUMENT_LEN];
} data_size;
