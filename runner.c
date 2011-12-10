#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "constants.h"
#include "server.h"

int run_a_job(job *to_run) {
  int forkint, err;
  FILE *out;
  char path[BUFFER_SIZE];
  
  if(forkint = fork()) {
    if(forkint < 0) {
      problem("Unsuccessful fork\n");
      return FAILURE;
    }
    return OKAY;
  } else {
    out = fopen("/jobs/
    execvp(


}
