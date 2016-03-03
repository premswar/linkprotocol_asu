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

struct my_data
{
  int val;
  timer_t *timerid;
};

void
print_siginfo (void * si)
{
  timer_t *tidp;
  int or;
  struct my_data *mdata;

  mdata = (struct my_data *) si;
  tidp =  mdata->timerid;
  printf ("    sival_ptr = %p; ", si);
  printf ("    *sival_ptr = %d\n", mdata->val);

  or = timer_getoverrun (*tidp);
  if (or == -1)
    errExit ("timer_getoverrun");
  else
    printf ("    overrun count = %d\n", or);
}

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
  printf("\n sec:%d,nsec:%d\n",msec/1000,(msec % 1000)*1000000);
  its.it_value.tv_sec = msec / 1000;
  its.it_value.tv_nsec = (msec % 1000) * 1000000;
  its.it_interval.tv_sec = its.it_value.tv_sec;
  its.it_interval.tv_nsec = its.it_value.tv_nsec;

  if (timer_settime (tid, 0, &its, NULL) == -1)
    errExit ("timer_settime");
  return 0;
} 

int
main (int argc, char *argv[])
{
  timer_t timerid1,timerid2;
  struct my_data mdata1,mdata2;
  int prem = 5;
  struct sigevent sev;
  struct itimerspec its;
  int freq_msecs;
  sigset_t mask;
  struct sigaction sa;
  ltimer_t tdata1;
  ltimer_t tdata2;

  if (argc != 3)
    {
      fprintf (stderr, "Usage: %s <sleep-secs> <freq-nanosecs>\n", argv[0]);
      exit (EXIT_FAILURE);
    }
  freq_msecs = atoll(argv[2]);
  mdata1.val = 10;
  mdata1.timerid = &timerid1;
  timerid1 = ltimerCreate(&tdata1, (callptr *)&print_siginfo, (void *)&mdata1);
  ltimerStart(timerid1,freq_msecs);
  mdata2.val = 20;
  mdata2.timerid = &timerid2;
  timerid2 = ltimerCreate(&tdata2, (callptr *)&print_siginfo, (void *)&mdata2);
  ltimerStart(timerid2,freq_msecs);
  /* Sleep for a while; meanwhile, the timer may expire
     multiple times */
  printf ("Sleeping for %d seconds\n", atoi (argv[1]));
  sleep (atoi (argv[1]));

  /* Unlock the timer signal, so that timer notification
     can be delivered */

  /*printf ("Unblocking signal %d\n", SIG);
  sigemptyset (&mask);
  sigaddset (&mask, SIG);
  if (sigprocmask (SIG_UNBLOCK, &mask, NULL) == -1)
    errExit ("sigprocmask");*/
  //sleep (atoi (argv[1]));
  timer_delete(timerid1);
  timer_delete(timerid2);
  exit (EXIT_SUCCESS);
}
