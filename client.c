#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>
#include "bulletin.h"
#include "constants.h"

void add_job(int num_args, char files[][MAX_ARGUMENT_LEN], int *flags) {
  char temp[BUFFER_LENGTH];
  (num_args, temp, BUFFER_LENGTH-1);
  bulletin_make_connection_with(hostname, port, &connection);
  send_string(connection, temp);
  for(i = 0; i < num_dep; i++) {
    send_string(connection, files[i]);
    itoa(flags[i], temp, BUFFER_SIZE-1);
    send_string(connection, temp);
  }
  close(connection);
}

void add_job_std_in(char *host, int port) {
  int num_args, num_jobs, connection, i, flags[MAX_ARGUMENTS];
  job *jobs;
  char job_names[MAX_JOBS][MAX_ARGUMENT_LEN], files[MAX_ARGUMENTS][BUFFER_SIZE];
  char temp[BUFFER_SIZE];


  printf("How many jobs would you like to enqueue?\n");
  scanf("%d", &num_jobs);
  if(num_jobs == 1) {
    get_job();
  }
  
  for(i = 0; i < num_jobs; i++) {
    printf("Enter the name of job (case insensitive %d\n", i);
    fgets(job_names[i], MAX_ARGUMENT_LEN-1, stdin);
    lowercase(job_names[i]);
  }
  jobs = malloc(sizeof(job)*num_jobs);
  for(i = 0; i < num_jobs; i++) {
    do{
      printfl("Enter the names of the jobs that %s depends on separated by commas(only immediate dependence)");
      fgets(temp, "%s", stdin);
    }  while(parse_dependencies(temp, job_names, jobs[i]))
  }
  
    if(flags[i]) {
      printf("Input a job id number\n");
      fgets(files[i], BUFFER_SIZE-1, stdin);
    } else {
      printf("Input a file number\n");
      fgets(files[i], BUFFER_SIZE-1, stdin);
    }
  }
}

void lowercase(char *str) {
     while(str[i]) {
       str[i] = (char)tolower(str[i]);
     }
}



int parse_dependencies(char *temp, char job_names[MAX_JOBS][MAX_ARGUMENT_LEN], job *j) {
  
}



int main(int argc, char **argv) {
}
