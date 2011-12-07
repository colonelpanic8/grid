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

void add_job(int num_args, char **files, int *flags) {
  char temp[BUFFER_LENGTH];
  itoa(num_args, temp, BUFFER_LENGTH-1);
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
  int num_args, connection, i, flags[MAX_ARGUMENTS];
  char files[MAX_ARGUMENTS][BUFFER_SIZE];

  printf("Type number of files/job dependencies (including the executable)\n");
  scanf("%d", &num_dep);
  itoa(numDep, num_args, BUFFER_SIZE-1);
  for(i = 0; i < num_dep; i++) {
    printf("Type 0 for file, 1 for job dependency number\n");
    scanf("%d", &flags[i]);
    if(flags[i]) {
      printf("Input a job id number\n");
      fgets(files[i], BUFFER_SIZE-1, stdin);
    } else {
      printf("Input a file number\n");
      fgets(files[i], BUFFER_SIZE-1, stdin);
    }
  }
}

int main(int argc, char **argv) {
}
