#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "constants.h"
#include "server.h"
#include "runner.h"

int run_a_job(job *to_run) {
  int forkint, err, i;
  FILE *out, *in;
  char outpath[BUFFER_SIZE], inpath[BUFFER_SIZE];
  char **argv;
  argv = malloc(sizeof(char)*to_run->argc);
  for(i = 0; i < to_run->argc; i++) {
    argv[i] = to_run->argv[i];
  }
  
  if(forkint = fork()) {
    if(forkint < 0) {
      problem("Unsuccessful fork\n");
      return FAILURE;
    }
    return OKAY;
  } else {
    sprintf(outpath, "./jobs/%d/output.txt", to_run->id);
    out = fopen(outpath, "w");
    if(!out) {
      problem("output file failed to open, this is very bad std out will not be redirected!!!!!!\n");
      if(dup2(fileno(out), STDOUT_FILENO) < 0) {
	problem("dup2 failed, this is very bad, stdout will not be redirected!!!!!!!!!!!!!\n");
      }
    }    
    sprintf(inpath, "./jobs/%d/input.txt", to_run->id);
    in = fopen(inpath, "r");
    if(!in) {
      problem("input file failed to open, this is very bad std in will not be redirected!!!!!!\n");
      if(dup2(fileno(out), STDIN_FILENO) < 0) {
	problem("dup2 failed, this is very bad, stdin will not be redirected!!!!!!!!!!!!!\n");
      }
    }
    execvp(argv[0], argv);
    problem("Fatal error for job %d, with name - execvp failed to run", to_run->id);
    return FAILURE;
  }
  wait(NULL);
}
