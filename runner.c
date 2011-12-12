#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "constants.h"
#include "server.h"
#include "runner.h"

int runner() {
  job *to_run;
  int status;
  struct timespec req;
#ifdef VERBOSE
  printf("runner intialized\n");
#endif
  req.tv_sec = 1;
  while(1) {
    while(get_job_for_runner(&to_run) < 0) {
#ifdef VERBOSE
      printf("No jobs to run, sleeping\n");
#endif
      nanosleep(&req, NULL);
    }
    printf("running %d\n", to_run->id);
    status = run_a_job(to_run);
    if(status < 0) {
    } else {
    }
  }
}

int run_a_job(job *to_run) {
  int forkint, err, i;
  FILE *out, *in;
  char outpath[BUFFER_SIZE], inpath[BUFFER_SIZE];
  char **argv;
  argv = malloc(sizeof(char *)*to_run->argc);
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

#ifdef VERBOSE
    printfl("executing: with %d args", to_run->argc);
    for(i = 0; i < to_run->argc; i++)
      printf("%s", argv[i]);
#endif

    if(!out) {
      problem("output file failed to open, this is very bad std out will not be redirected!!!!!!\n");
    }
    if(dup2(fileno(out), STDOUT_FILENO) < 0) {
      problem("dup2 failed, this is very bad, stdout will not be redirected!!!!!!!!!!!!!\n");
    }
    sprintf(inpath, "./jobs/%d/input.txt", to_run->id);
    in = fopen(inpath, "r");
    if(!in) {
#ifdef VERBOSE
      problem("input file failed to open, proceeding without file for standard in");
#endif
    } else {
      if(dup2(fileno(out), STDIN_FILENO) < 0) {
	problem("dup2 failed, this is very bad, stdin will not be redirected!!!!!!!!!!!!!\n");
      }
    }
    execvp(argv[0], argv);
    problem("Fatal error for job %d,execvp failed to run with errno: %d\n", to_run->id, errno);
    return FAILURE;
  }
  wait(NULL);
}
