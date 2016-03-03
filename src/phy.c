#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include "common.h"
#include "link.h"
#include "buffer.h"
#include "phy.h"


void
phyReTx (linkParam_t * linfo, unsigned char *buf, int len)
{

  slog (0, SLOG_DEBUG, "%s : type = %02X, seq = %d\n", __FUNCTION__, buf[0],
	buf[1]);
  SendToChannel (buf, len, linfo->txSockId);
}

void
phyTx (linkParam_t * linfo)
{
  int len;
  unsigned char buf[200];

  slog (0, SLOG_DEBUG, "%s\n", __FUNCTION__);
  while (bufferState (&linfo->lbuf, OP_TX))
    {
      len = bufferRead (&linfo->lbuf, buf, OP_TX, 0);
      slog (0, SLOG_DEBUG, "%s : type = %02X, seq = %d\n", __FUNCTION__,
	    buf[0], buf[1]);
      bufferStatusReport (&linfo->lbuf);
      SendToChannel (buf, len, linfo->txSockId);
    }
  slog (0, SLOG_DEBUG, "%s Sucessfully transmitted buff\n", __FUNCTION__);
  bufferStatusReport (&linfo->lbuf);
}

void
phyRx (linkParam_t * linfo)
{

  struct sockaddr_un remote;
  int rem_size;
  char rxMsg[MAX_MSG_SIZE];
  int len, seqNum;

  while (1)
    {
      rem_size = sizeof (remote);
      if ((len =
	   recvfrom (linfo->rxServSockId, (void *) &rxMsg, MAX_MSG_SIZE, 0,
		     (struct sockaddr *) &remote, &rem_size)) < 0)
	{
	  if (errno == EINTR)
	    continue;
	  else
	    {
	      perror ("recvfrom");
	      exit (1);
	    }
	}
      slog (0, SLOG_DEBUG, "[%s]recvdfrom %s.\n", __FUNCTION__,
	    remote.sun_path);
      seqNum = ((rxMsg[1] & 0xFF) << 8) + rxMsg[2];
      slog (0, SLOG_DEBUG, "[phyRx]RCVD packet Flags:%02x Seq no:%d\n ", 
            rxMsg[0],seqNum);

      linkRx (linfo, rxMsg, len);
      bufferStatusReport (&linfo->lbuf);
    }
}

void
SendToChannel (unsigned char *buf, int len, int sockId)
{

  int i, rlen, nbytes;
  struct msgSt txMsg;
  int seq, ctrl, retran;
  struct sockaddr_un remote;

  slog (0, SLOG_INFO, "%s\n", __FUNCTION__);
  remote.sun_family = AF_UNIX;
  strcpy (remote.sun_path, CHANNEL_RX);
  rlen = strlen (remote.sun_path) + sizeof (remote.sun_family);
  txMsg.msgType = 1;
  for (i = 0; i < len; i++)
    txMsg.msg[i] = buf[i];

  if ((nbytes = sendto (sockId, buf, len, 0,
			(struct sockaddr *) &remote, rlen)) < 0)
    {
      perror ("sendto (channel)");
      exit (1);
    }
  slog (0, SLOG_DEBUG, "Trasmitted %d bytes [%d]\n", nbytes, len);
}
