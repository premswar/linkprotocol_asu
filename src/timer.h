#ifndef __TIMER_H__
#define __TIMER_H__


#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <string.h>

#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                               } while (0)
typedef void (callptr) ();

typedef struct ltimer_s
{
  timer_t timerid;
  callptr *callback;
  void *data;
} ltimer_t;

timer_t
ltimerCreate (ltimer_t *tdata, callptr *callback, void *data);
int
ltimerStop(timer_t tid);
int
ltimerStart(timer_t tid, int msec);

#endif
