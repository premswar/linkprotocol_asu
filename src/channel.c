#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/un.h>

#include "common.h"
#define CHANNEL_ACTION_DROP 1
#define CHANNEL_ACTION_NONE 0

typedef struct channel_s
{
  int rxfd;
  int txfd;
} channel_t;

static channel_t channel_l;

void
channel_init (channel_t * channel)
{
  /*
   * Create RX and TX Sockets
   */
  channel->rxfd = socket_create (CHANNEL_RX);
  channel->txfd = socket_create (CHANNEL_TX);
}

void
channel_dinit (channel_t * channel)
{
  close (channel->rxfd);
  close (channel->txfd);
  unlink (CHANNEL_RX);
  unlink (CHANNEL_TX);
  remove (CHANNEL_TX);
  remove (CHANNEL_RX);
  channel->rxfd = -1;
  channel->txfd = -1;
}

int
forwardto (channel_t * channel, const char *dest_addr, char *msg, int msg_len)
{

  struct sockaddr_un remote;
  int len, nbytes;
  remote.sun_family = AF_UNIX;
  strcpy (remote.sun_path, dest_addr);
  len = strlen (remote.sun_path) + sizeof (remote.sun_family);
  if ((nbytes = sendto (channel->txfd, msg, msg_len, 0,
			(struct sockaddr *) &remote, len)) < 0)
    {
      perror ("forwardto (client)");
      return 0;
    }
  return 1;
}

int
find_dest (channel_t * channel, const char *src, char *dest)
{

  if (strcmp (src, TX_MACHINE_TX) == 0)
    {
      strcpy (dest, RX_MACHINE_RX);
    }
  else if (strcmp (src, RX_MACHINE_TX) == 0)
    {
      strcpy (dest, TX_MACHINE_RX);
    }
  else
    {
      printf ("unknown source :%s\n ", src);
      return 0;
    }
    //printf("Dest is :%s\n", dest);
    return 1;
}

static void 
catch_interrupt(int signal) 
{
    printf ("User Interrupt signal caught Terminating program\n");
    channel_dinit (&channel_l);
    exit (1); 
}
void
applyChannelFilter(channel_t *channel, char *msg,int len,int *action){
   int seq, ctrl, retran;

   *action = CHANNEL_ACTION_NONE; // DO not Drop this PACKET
   ctrl = msg[0] & CTRL_FRAME;
   retran = msg[0] & RETRANS_FRAME; 
   if((ctrl != CTRL_FRAME) && (retran != RETRANS_FRAME)) {
      //seq = msg[1]&0xFF;
      seq = ((msg[1] & 0xFF) << 8) + msg[2];
      if((seq == 1) || ((seq+1)%7 == 0) ) {
         printf("------> channel : drop seq %d\n", seq);
         *action = CHANNEL_ACTION_DROP; // Drop this PACKET
      }
   }
   return;
}

int
main (int argc, char *argv[])
{

  struct sockaddr_un remote;
  int rem_size, ret, n,action;
  char msg[MAX_MSG_SIZE];
  char dest[16];

  if (signal(SIGINT, catch_interrupt) == SIG_ERR)
     printf("\ncan't catch SIGINT\n");

  channel_init (&channel_l);

  fflush (stdout);
  printf ("Running...\n");
  /*
   * Listen for connections
   */
  while (1)
    {
      rem_size = sizeof (remote);
      if ((n =
	   recvfrom (channel_l.rxfd, msg, MAX_MSG_SIZE, 0,
		     (struct sockaddr *) &remote, &rem_size)) < 0)
	{
	  perror ("recvfrom");
	  exit (1);
	}
      //printf ("Recvd from %s.\n", remote.sun_path);
      if (find_dest (&channel_l, remote.sun_path, dest) == 0)
      {
         printf ("unable to find destination \n");
      }
      applyChannelFilter(&channel_l, msg,n, &action);
      if(action == CHANNEL_ACTION_DROP)
      {
        // Do not forward so it dropped
      }
      else
      {
        //printf ("forward to %s.\n", dest);
        ret = forwardto (&channel_l, dest, msg, n);
        if (ret == 0)
        {
           printf ("Unable to forward msg to :%s\n", dest);
        }
      }
   }
  channel_dinit (&channel_l);
  return 0;
}
