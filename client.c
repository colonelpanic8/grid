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
#include "client.h"


void add_job_std_in(char *host, int port) {
  int num_args, num_jobs, connection, i, flags[MAX_ARGUMENTS];
  job *jobs;
  char job_names[MAX_JOBS][MAX_ARGUMENT_LEN], files[MAX_ARGUMENTS][BUFFER_SIZE];
  char temp[BUFFER_SIZE];
  jobs = malloc(sizeof(job)*num_jobs);


  printf("How many jobs would you like to enqueue?\n");
  fgets(temp, BUFFER_SIZE, stdin);
  sscanf(temp, "%d", &num_jobs);
  
  

  for(i = 0; i < num_jobs; i++) {
    printfl("Give the name of job %d", i);
    fgets(temp, MAX_ARGUMENT_LEN, stdin);
    lowercase(temp);
    strcpy(job_names[i], temp);
  }


  for(i = 0; i < num_jobs; i++) {
    do{
      printfl("Enter the names of the jobs that %s depends on separated by commas(only immediate dependence)",
	      job_names[i]);
      fgets(temp, BUFFER_SIZE-1, stdin);
    }  while(parse_dependencies(temp, job_names, &jobs[i]));
  }
  
}
void lowercase(char *str) {
  int i=0;
     while(str[i]) {
       str[i] = (char)tolower(str[i]);
       i++;
     }
}



int parse_dependencies(char *str, char job_names[MAX_JOBS][MAX_ARGUMENT_LEN], job *j) {
  char temp[BUFFER_SIZE];
  char *point;
  while(point = strchr(str, (int)',')) {
    strncpy(temp, str, str-point);
    printfl("%s", temp);
    str = point;
    str++;


  }
  
  return 0;
}



int main(int argc, char **argv) {
  add_job_std_in(NULL, 0);
}
