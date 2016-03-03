#include "timer.h"

static void
handler (int sig, siginfo_t * si, void *uc)
{
  ltimer_t *tdata;
  /* Note: calling printf() from a signal handler is not
     strictly correct, since printf() is not async-signal-safe;
     see signal(7) */

  //printf ("Caught signal %d\n", sig);
  tdata = (ltimer_t *) si->si_value.sival_ptr;
  ltimerStop(tdata->timerid);
  tdata->callback (tdata->data);
  //print_siginfo(si);
  //signal (sig, SIG_IGN);
}

timer_t
ltimerCreate (ltimer_t *tdata, callptr * callback, void *data)
{
  timer_t timerid;
  struct sigevent sev;
  sigset_t mask;
  struct sigaction sa;
  //ltimer_t tdata;
  /* Establish handler for timer signal */

  printf ("Establishing handler for signal %d\n", SIG);
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = handler;
  sigemptyset (&sa.sa_mask);
  if (sigaction (SIG, &sa, NULL) == -1)
    errExit ("sigaction");

  /* Block timer signal temporarily */
  /*
  printf ("Blocking signal %d\n", SIG);
  sigemptyset (&mask);
  sigaddset (&mask, SIG);
  if (sigprocmask (SIG_SETMASK, &mask, NULL) == -1)
    errExit ("sigprocmask");*/

  /* Create the timer */
  tdata->callback = callback;
  tdata->data = data;
  sev.sigev_notify = SIGEV_SIGNAL;
  sev.sigev_signo = SIG;
  //sev.sigev_value.sival_ptr = &timerid;
  sev.sigev_value.sival_ptr = tdata;
  if (timer_create (CLOCKID, &sev, &timerid) == -1)
    errExit ("timer_create");
  memcpy (&tdata->timerid, &timerid, sizeof (timer_t));
  printf ("timer ID is 0x%lx\n", (long) timerid);
  return timerid;
}

int
ltimerStop(timer_t tid)
{
  struct itimerspec its;

  /* Start the timer */

  its.it_value.tv_sec = 0;
  its.it_value.tv_nsec = 0;
  its.it_interval.tv_sec = 0;
  its.it_interval.tv_nsec = 0;

  if (timer_settime (tid, 0, &its, NULL) == -1)
    errExit ("timer_settime");
  return 0;
} 

int
ltimerStart(timer_t tid, int msec)
{
  struct itimerspec its;

  /* Start the timer */
  //printf("\n sec:%d,nsec:%d\n",msec/1000,(msec % 1000)*1000000);
  its.it_value.tv_sec = msec / 1000;
  its.it_value.tv_nsec = (msec % 1000) * 1000000;
  its.it_interval.tv_sec = its.it_value.tv_sec;
  its.it_interval.tv_nsec = its.it_value.tv_nsec;

  if (timer_settime (tid, 0, &its, NULL) == -1)
    errExit ("timer_settime");
  return 0;
} 
//timer_delete(timerid1);

