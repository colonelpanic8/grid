#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "constants.h"
#include <sys/wait.h>
#include "server.h"
#include "runner.h"

int runner() {
  job *to_run = NULL;
  int status, i;
  struct timespec req;
#ifdef VERBOSE
  printf("Runner intialized\n");
#endif
  req.tv_sec = RUNNER_REST;
  while(1) {

    while(!(to_run = get_job_for_runner())) {
#ifdef SHOW_RUNNER_STATUS
      printf("No jobs to run, sleeping\n");
#endif
#ifdef ENABLE_HEARTBEAT
      heartbeat();
#endif
      nanosleep(&req, NULL);
    }
#ifdef VERBOSE
    printf(BAR);
    printfl("Executing: %s |  Job Id: %d", to_run->name, to_run->id);
    for(i = 0; i < to_run->argc; i++)
      printf("%s ", to_run->argv[i]);
    printf("\n");
    printf(BAR);
#endif
#ifdef RUN_JOBS
    
    printf("running %d\n", to_run->id);
    status = run_a_job(to_run);
    if(status < 0) {
      to_run->status = status;
      
    } else {
      //Job Complete
      to_run->status = COMPLETED;
    }
#endif
  }
}

int run_a_job(job *to_run) {
  int forkint, err, i;
  FILE *out, *in;
  char outpath[BUFFER_SIZE], inpath[BUFFER_SIZE];
  char *error;
  char **argv;
  argv = malloc(sizeof(char *)*(to_run->argc+1));
  for(i = 0; i < to_run->argc; i++) {
    argv[i] = to_run->argv[i];
  }  
  argv[to_run->argc] = NULL;
  if(forkint = fork()) {
    if(forkint < 0) {
      problem("Unsuccessful fork\n");
      return FAILURE;
    }
    waitpid(forkint, &err, 0);
    return err;
  } else {
    sprintf(inpath, "./jobs/%d/", to_run->id);
    chdir(inpath);
    out = fopen("output.txt", "w");
    if(!out) {
      problem("output file failed to open, this is very bad std out will not be redirected!!!!!!\n");
    }
    if(dup2(fileno(out), STDOUT_FILENO) < 0) {
      problem("dup2 failed, this is very bad (output)\n");
    }
    sprintf(inpath, "input.txt", to_run->id);

/*

Error:
runner.c: In function ‘run_a_job’:
runner.c:84:21: warning: too many arguments for format [-Wformat-extra-args]
   84 |     sprintf(inpath, "input.txt", to_run->id);
      |              

Not altered but, Possible Fixes:

     sprintf(inpath, "input%d.txt", to_run->id);

or 

    sprintf(inpath, "%d.txt", to_run->id);

*/

    in = fopen(inpath, "r");
    if(!in) {
#ifdef VERBOSE
      problem("No input file\n");
#endif
    } else {
      if(dup2(fileno(out), STDIN_FILENO) < 0) {
	problem("dup2 failed (input)\n");
      }
    }
    execvp(argv[0], argv);
    problem("Fatal error for job %d,execvp failed to run with errno: %d\n", to_run->id, errno);
    problem("strerror says:\n");
    error = strerror(errno);
    problem("%s\n", error);
    exit(-1);
  }
  wait(NULL);
}
