#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include "common.h"
#include "buffer.h"
#include "link.h"
#include <semaphore.h>  /* Semaphore */

#define FRAME_LEN 95
#define RSTIMEOUT_MS 300	//300 millisecs
#define SEEK_ST 0
#define SEEK_ED 1

#if 0
void
ctrlEventInit ()
{

  evCount = 0;
}
#endif 
void
regCtrlEvent (linkParam_t * linfo, int type, int seq)
{
  int evCount;
 
  evCount =  linfo->evCount;
  linfo->ctrlEv[evCount].event[0] = (type >> 8) & 0xFF;
  linfo->ctrlEv[evCount].event[1] = (seq >> 8) & 0xFF;
  linfo->ctrlEv[evCount].event[2] = seq & 0xFF;
  linfo->evCount++;
}

void
sendControl (linkParam_t * linfo)
{

  bufferWrite (&linfo->lbuf, (unsigned char *) linfo->ctrlEv, 
               linfo->evCount * sizeof (ctrlEvntSt_t),
               linfo->evCount, CTRL_FRAME, OP_TX);
  linfo->evCount = 0;
  phyTx (linfo);
}

void
retransmission (linkParam_t * linfo, int seq, int txMsqId)
{

  struct frameSt *frame;

  slog (0, SLOG_DEBUG, "%s : retransmist packet %d\n", __FUNCTION__, seq);
  frame = bufferWaitScan (&linfo->lbuf, OP_TX, seq);
  if (frame == NULL_FRAME)
    {
      slog (0, SLOG_ERROR,
	    "request to retransmit not exisiting frame seq = %d\n", seq);
      return;
      //exit(1);
    }
  frame->data[0] = frame->data[0] | 0x40;	// retransmission indicator set
  SendToChannel (frame->data, frame->len, txMsqId);
}

void
rstCbk (void *data)
{
  linkParam_t *ldata;
  int next_seq;

  ldata = (linkParam_t *) data;
  // this is the last success seq we have recived so send 
  // NACK for next seq
  next_seq = ldata->lbuf.rxSuccessSeq + 1;
  slog (0, SLOG_DEBUG, "%s : timer expired %d nseq\n", __FUNCTION__, next_seq);
  regCtrlEvent (ldata, TYPE_NACK, next_seq);
  sendControl (ldata);
}

void *
recvFromChannel (void *targ)
{

  linkParam_t *ldata;
  int s;

  ldata = (linkParam_t *) targ;
  s = pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
  if (s != 0)
    perror ("pthread_setcancelstate");
  phyRx (ldata);
}

void
linkInit (linkParam_t * lparam)
{
  int rc;
  //ctrlEventInit ();
  bufferInit (&lparam->lbuf);
  lparam->evCount = 0;
  lparam->seqNum = 0;
  lparam->txSockId = socket_create (lparam->linkTxName);
  lparam->rxServSockId = socket_create (lparam->linkRxName);
  // create a receiver thread so that link can listen
  rc = pthread_create (&lparam->linkRxThrdId, NULL, recvFromChannel,
		       (void *) lparam);
  if (rc)
    {
      slog (0, SLOG_FATAL, "ERROR; return code from pthread_create() is %d\n",
	    rc);
      exit (-1);
    }
  /* Create reciver sema for apps to wait on link Recv function*/
  /* initialize mutex to 0 - binary semaphore */
  /* second param = 0 - semaphore is local */
  sem_init(&lparam->recvSema, 0, 0);
  sem_init(&lparam->recvDoneSema, 0, 0);

  lparam->reassembleTimer.timerid = ltimerCreate (&lparam->reassembleTimer,
						  (callptr *) & rstCbk,
						  lparam);
  lparam->sendBuf = (char *)malloc((MAX_PKT_SIZE+2)*sizeof(unsigned char));
  lparam->recvBuf = (char *)malloc((MAX_PKT_SIZE+2)*sizeof(unsigned char));
  /* 
   * slog_init - Initialise slog 
   * First argument is log filename 
   * Second argument is config file
   * Third argument is max log level on console
   * Fourth is max log level on file   
   * Fifth is thread safe flag.
   */
  slog_init (lparam->idString, "slog.cfg", 2, 3, 1);
}

void
linkDeInit (linkParam_t * lparam)
{
  bufferDeInit (&lparam->lbuf);
  free(lparam->sendBuf);
  free(lparam->recvBuf);
  close (lparam->txSockId);
  close (lparam->rxServSockId);
  lparam->txSockId = -1;
  lparam->rxServSockId = -1;
  sem_destroy(&lparam->recvSema); /* destroy semaphore */
  sem_destroy(&lparam->recvDoneSema); /* destroy semaphore */
}

void
makeLLFrame (unsigned char *data, int len, int seq, int flag,
	     struct frameSt *frame)
{

  int i;
  int pLen = 0;

  slog (0, SLOG_DEBUG, "%s\n", __FUNCTION__);
  frame->data[pLen++] = (unsigned char) flag;
  //frame->data[pLen++] = (unsigned char) seq;
  frame->data[pLen++] = (unsigned char) (((seq) >> 8) & 0xFF);
  frame->data[pLen++] = (unsigned char) (seq & 0xFF);
  for (i = 0; i < len; i++, pLen++)
    frame->data[pLen] = data[i];
  frame->seqNum = seq;
  frame->len = pLen;
}

void
segmentation (linkParam_t * linfo, unsigned char *msg, int len)
{

  int i, j;
  int segIdx = 0;
  int fNum, fTail;
  int frameFlag = 0;
  struct frameSt ll_frame;

  fNum = len / FRAME_LEN;
  fTail = len % FRAME_LEN;

  slog (0, SLOG_DEBUG, "%s\n", __FUNCTION__);
  // No need to segment make frame header and write to buffer queue
  if (len < FRAME_LEN)
    {
      makeLLFrame (msg, len, linfo->seqNum++, START_FRAME | END_FRAME, &ll_frame);
      writeToTxBuf (&linfo->lbuf, &ll_frame);
    }
  else
    {				// transmit remaining frames
      fNum = (len / FRAME_LEN);
      fTail = len % FRAME_LEN;
      fNum = (fTail != 0) ? (fNum + 1) : fNum;
      makeLLFrame (&msg[0], FRAME_LEN, linfo->seqNum++, START_FRAME, &ll_frame);
      writeToTxBuf (&linfo->lbuf, &ll_frame);
      for (i = 1; i < fNum - 1; i++)
	{
	  makeLLFrame (&msg[i * FRAME_LEN], FRAME_LEN, linfo->seqNum++, INT_FRAME,
		       &ll_frame);
	  writeToTxBuf (&linfo->lbuf, &ll_frame);
	}
      makeLLFrame (&msg[i * FRAME_LEN], (len - i * FRAME_LEN), linfo->seqNum++,
		   END_FRAME, &ll_frame);
      writeToTxBuf (&linfo->lbuf, &ll_frame);
    }
}

//void reassemble(int txMsqId) {
void
reassemble (linkParam_t * linfo)
{


  static int state = SEEK_ST;
  static unsigned char packet[MAX_PKT_SIZE+2];
  static int idx = 0;

  int i;
  unsigned char buf[1024];
  int len;
  int compFlag = 0;

  slog (0, SLOG_DEBUG, "%s  seekstate:%d\n", __FUNCTION__, state);
  viewRxBuffLists (&linfo->lbuf);
  while (bufferState (&linfo->lbuf,OP_RX))
    {
      len = bufferRead (&linfo->lbuf, buf, OP_RX, linfo->txSockId);
      slog (0, SLOG_INFO, "idx = %d, len = %d\n", idx, len);
      if ((buf[0] & 0x03) == (START_FRAME | END_FRAME))
	{
	  slog (0, SLOG_DEBUG, "S|E");
	  idx = 0;
	  for (i = 0; i < len - 3; i++)
	    packet[idx++] = buf[i + 3];
	  compFlag = 1;
	  slog (0, SLOG_INFO, "idx = %d, len = %d\n", idx, len);
	  break;
	}
      else if (state == SEEK_ST)
	{
	  if ((buf[0] & 0x03) == START_FRAME)
	    {
	      slog (0, SLOG_DEBUG, "S\n");
	      idx = 0;
	      for (i = 0; i < len - 3; i++)
		packet[idx++] = buf[i + 3];
	      state = SEEK_ED;
	      slog (0, SLOG_DEBUG, "ltimer start\n");
	      ltimerStart (linfo->reassembleTimer.timerid, RSTIMEOUT_MS);
	    }
	  else
	    {
	      slog (0, SLOG_DEBUG, "I or E\n");
	    }
	}
      else
	{
	  if ((buf[0] & 0x2) == START_FRAME)
	    {
	      slog (0, SLOG_DEBUG, "S\n");
	      idx = 0;
	      for (i = 0; i < len - 3; i++)
		packet[idx++] = buf[i + 3];
	      // STart the reassembletimer to wait for END_FRAME PKT
	      slog (0, SLOG_DEBUG, "ltimer start\n");
	      ltimerStart (linfo->reassembleTimer.timerid, RSTIMEOUT_MS);
	    }
	  else if ((buf[0] & 0x1) == END_FRAME)
	    {
	      slog (0, SLOG_DEBUG, "E\n");
	      for (i = 0; i < len - 3; i++)
		packet[idx++] = buf[i + 3];
	      state = SEEK_ST;
	      compFlag = 1;
	      //END_FRAME recived stop the reassamble timer
	      slog (0, SLOG_DEBUG, "ltimer stop\n");
	      ltimerStop (linfo->reassembleTimer.timerid);
	      break;
	    }
	  else
	    {
	      slog (0, SLOG_DEBUG, "I\n");
	      for (i = 0; i < len - 3; i++)
		packet[idx++] = buf[i + 3];
	    }
	}
    }

  if (compFlag == 1)
  {
	linfo->lbuf.rxSuccessCount = 0;
	regCtrlEvent (linfo, TYPE_ACK, linfo->lbuf.rxSuccessSeq);
	sendControl (linfo);
    updateRecvr(linfo, packet, idx);
	idx = 0;
    //appRx (packet, idx);
  }
  else if (linfo->lbuf.rxSuccessCount >= ACK_INTERVAL)
  {
    linfo->lbuf.rxSuccessCount = 0;
    regCtrlEvent (linfo, TYPE_ACK, linfo->lbuf.rxSuccessSeq);
    sendControl (linfo);
  }
}
void updateRecvr(linkParam_t * linfo, unsigned char *buf, int len)
{
   //linfo->recvBuf =  (char *)malloc(len*sizeof(unsigned char));
   if(len < MAX_PKT_SIZE+3)
     memcpy(linfo->recvBuf, buf, len);
   else
     printf("ERROR INVALID PKT LENGTH RCVD :%d\n", len);
   linfo->recvBufLen = len;
   sem_post(&linfo->recvSema);
   sem_wait(&linfo->recvDoneSema);
   //free(linfo->recvBuf);
   //linfo->recvBufLen = 0; 
}
/*
  Move these Functions to LINK API file later
*/
void
linkRecv (linkParam_t * linfo, unsigned char *buf, int *len)
{
   int pLen;
   char *rcvBuf;

   sem_wait(&linfo->recvSema);
   rcvBuf = linfo->recvBuf;
   pLen = rcvBuf[0]*0x100 + rcvBuf[1];
   //printf("rcvd pkt plen:%d [%2x %2x %2x %2x]\n", pLen,
   //        rcvBuf[0], rcvBuf[1], rcvBuf[2], rcvBuf[3]);
   if(pLen > linfo->recvBufLen-2)
   { 
     printf("Error in length of recived file:%d Expec:%d\n", 
            linfo->recvBufLen, pLen);
     return;
   }
   memcpy(buf, rcvBuf+2, pLen);
   memcpy(len, &pLen, sizeof(int));
   //*len = linfo->recvBufLen;
   sem_post(&linfo->recvDoneSema);
}
void
linkSend (linkParam_t * linfo, unsigned char *buf, int len)
{
  char *packet;

  slog (0, SLOG_DEBUG, "%s\n", __FUNCTION__);
  /* MALLOC the buffer to send */
  //packet = (char *)malloc((len+2)*sizeof(unsigned char));
  if(len < MAX_PKT_SIZE+1)
  {
    packet = linfo->sendBuf;
    memset(packet,0,MAX_PKT_SIZE+2);
  }
  else
  {
    printf("ERROR PACKET SIZE GREATER THAN MAX SIZE \n");
    return;
  } 
  packet[0] = (len >> 8) & 0xFF;
  packet[1] = (len) & 0xFF;
  memcpy(packet+2,buf,len);
  linkTx (linfo, packet, len+2);
  //free(packet);
}
void
linkTx (linkParam_t * linfo, unsigned char *buf, int len)
{

  slog (0, SLOG_DEBUG, "%s\n", __FUNCTION__);
  segmentation (linfo, buf, len);
  phyTx (linfo);
}

void
linkRx (linkParam_t * linfo, unsigned char *buf, int len)
{

  int packetLen;

  processRxFrame (linfo, buf, len);
  do
    {
      reassemble (linfo);
    }
  while (bufferState (&linfo->lbuf, OP_RX));
}
