typedef struct _queue{
  node_j *head; 
  node_j *tail;
} queue;

typedef struct _node_j {
  job obj; 
  struct node_j *next;
} node_j;
