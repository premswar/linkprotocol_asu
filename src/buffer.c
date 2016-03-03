#include <stdio.h>
#include <stdlib.h>
#include "list.h"
#include "link.h"
#include "buffer.h"

void
bufferStatusReport (linkBuf_t * lbuf)
{
#if 1 
  slog (0, SLOG_INFO, "txI=%d ", lbuf->txIdleList->nodeNum);
  slog (0, SLOG_INFO, "txA=%d ", lbuf->txActiveList->nodeNum);
  slog (0, SLOG_INFO, "txW=%d ", lbuf->txWaitList->nodeNum);
  slog (0, SLOG_INFO, "rxI=%d ", lbuf->rxIdleList->nodeNum);
  slog (0, SLOG_INFO, "rxA=%d ", lbuf->rxActiveList->nodeNum);
  slog (0, SLOG_INFO, "rxW=%d ", lbuf->rxWaitList->nodeNum);
#endif
}

void
bufferInit (linkBuf_t * lbuf)
{

  int i;

  lbuf->rxBufState = 0;
  lbuf->rxSuccessSeq = 0;
  lbuf->rxSuccessCount = 0;

  lbuf->txIdleList = (struct listSt *) malloc (sizeof (struct listSt));
  lbuf->txActiveList = (struct listSt *) malloc (sizeof (struct listSt));
  lbuf->txWaitList = (struct listSt *) malloc (sizeof (struct listSt));
  lbuf->rxIdleList = (struct listSt *) malloc (sizeof (struct listSt));
  lbuf->rxActiveList = (struct listSt *) malloc (sizeof (struct listSt));
  lbuf->rxWaitList = (struct listSt *) malloc (sizeof (struct listSt));

  listInit (lbuf->txIdleList);
  listInit (lbuf->txActiveList);
  listInit (lbuf->txWaitList);
  listInit (lbuf->rxIdleList);
  listInit (lbuf->rxActiveList);
  listInit (lbuf->rxWaitList);

  // insert frames on txIdleList
  for (i = 0; i < MAX_FRAME; i++)
    {
      lbuf->txFrameFile[i] = (struct frameSt *) malloc (sizeof (struct frameSt));
      lbuf->rxFrameFile[i] = (struct frameSt *) malloc (sizeof (struct frameSt));
      listNodeInsert (lbuf->txIdleList, lbuf->txIdleList->tail,
		      (struct nodeSt *) (lbuf->txFrameFile[i]));
      listNodeInsert (lbuf->rxIdleList, lbuf->rxIdleList->tail,
		      (struct nodeSt *) (lbuf->rxFrameFile[i]));
    }
}

void
bufferDeInit (linkBuf_t * lbuf)
{
  int i;

  lbuf->rxBufState = 0;
  lbuf->rxSuccessSeq = 0;
  lbuf->rxSuccessCount = 0;
  free (lbuf->txIdleList);
  free (lbuf->txActiveList);
  free (lbuf->txWaitList);
  free (lbuf->rxIdleList);
  free (lbuf->rxActiveList);
  free (lbuf->rxWaitList);
  for (i = 0; i < MAX_FRAME; i++)
    {
      free(lbuf->txFrameFile[i]);
      free(lbuf->rxFrameFile[i]);
    }
}

void
viewBuffList (struct listSt *viewList)
{
  struct frameSt *frame, *nFrame;

  frame = (struct frameSt *) listNextNodeView (viewList, NULL);
  while (frame != NULL)
    {
      slog (0, SLOG_INFO, "packet seq in list:%d\n", frame->seqNum);
      nFrame =
	(struct frameSt *) listNextNodeView (viewList,
					     (struct nodeSt *) frame);
      frame = nFrame;
    }
}

void
viewRxBuffLists (linkBuf_t * lbuf)
{
  slog (0, SLOG_INFO, "----------rxActivelist-------\n");
  viewBuffList (lbuf->rxActiveList);
  slog (0, SLOG_INFO, "----------rxWaitlist-------\n");
  viewBuffList (lbuf->rxWaitList);
}

void
writeToTxBuf (linkBuf_t * lbuf, struct frameSt *txframe)
{

  struct frameSt *frame;;

  slog (0, SLOG_INFO, "%s\n", __FUNCTION__);
  frame = (struct frameSt *) listNodeGet (lbuf->txIdleList);
  *frame = *txframe;
  listNodeInsert (lbuf->txActiveList, lbuf->txActiveList->tail, (struct nodeSt *) frame);
}

void
processCtrlFrame (linkParam_t * linfo, unsigned char *data, int len)
{
  int i;
  struct frameSt *frame;
  int recordLen = data[1];
  int retSeq, ackSeq;
  char tempbuf[100];
  linkBuf_t * lbuf;

  lbuf = &linfo->lbuf;
  frame = (struct frameSt *) listNodeGet (lbuf->rxIdleList);
  for (i = 0; i < 2 + (recordLen * 3); i++)
    sprintf (tempbuf, "%02X ", data[i]);
  slog (0, SLOG_DEBUG, "OP_RX: control packet :%d ", recordLen);
  for (i = 0; i < recordLen; i++)
    {
      if (data[2 + i * 2] & 0x80)
	{			// nack message
	  retSeq = ((data[3 + i * 2] & 0xFF) << 8) + data[4 + i * 2];
	  ackSeq = (retSeq - 1) & 0xFF;
	  slog (0, SLOG_DEBUG, "[%s] record %d NACK MSG for SEQ:%d\n",
		__FUNCTION__, i, retSeq);
	  retransmission (linfo, retSeq, linfo->txSockId);
	  //ackProcessing (lbuf,ackSeq);
	}
      else
	{			// ack message
	  ackSeq = ((data[3 + i * 2] & 0xFF) << 8) + data[4 + i * 2];
	  slog (0, SLOG_DEBUG, "[%s] record %d ACK MSG for SEQ:%d\n",
		__FUNCTION__, i, ackSeq);
	  ackProcessing (lbuf, ackSeq);
	}
    }

  listNodeInsert (lbuf->rxIdleList, lbuf->rxIdleList->tail, (struct nodeSt *) frame);
}

void
processDataFrame (linkParam_t * linfo, unsigned char *data, int len)
{

  int i;
  int pLen = 0, cnt = 0;
  int curSeq, expSeq;
  struct frameSt *frame, *prevFrame;
  int nackFlag = 0;
  int nackSeq = 0;
  int ret;
  char strflag[5];
  linkBuf_t * lbuf;

  lbuf = &linfo->lbuf;
  frame = (struct frameSt *) listNodeGet (lbuf->rxIdleList);

  for (i = 0, pLen = 0; i < len; i++, pLen++)
    frame->data[i] = data[pLen];
  frame->seqNum = ((data[1] & 0xFF) << 8) + data[2];
  frame->len = len;
  slog (0, SLOG_DEBUG, "OP_RX: data packet. Flags:%02x Seq no:%d\n ", data[0],
	frame->seqNum);

  if ((frame->data[0] & RETRANS_FRAME) == RETRANS_FRAME)
    {				// retransmitted packet
      ret = bufferActiveScan (lbuf, OP_RX, frame->seqNum, &prevFrame);
      if (ret == 0)
	{
	  listNodeInsert (lbuf->rxActiveList, (struct nodeSt *) prevFrame,
			  (struct nodeSt *) frame);
	}
      else if (frame->seqNum > lbuf->rxSuccessSeq)
	{
	  listNodeInsert (lbuf->rxActiveList, lbuf->rxActiveList->tail,
			  (struct nodeSt *) frame);
	}
      else
	{
	  slog (0, SLOG_DEBUG, "needless transmission seq = %d\n",
		frame->seqNum);
	  //exit (1);
	}
    }
  else
    {				// Regular Data packet

      if ((data[0] & 0x7F) == 0)
	sprintf (strflag, "%s\n", "I");
      if ((data[0] & 0x7F) == 1)
	sprintf (strflag, "%s\n", "E");
      if ((data[0] & 0x7F) == 2)
	sprintf (strflag, "%s\n", "S");
      if ((data[0] & 0x7F) == 3)
	sprintf (strflag, "%s\n", "S|E");

      curSeq = ((data[1] & 0xFF) << 8) + data[2];
      slog (0, SLOG_DEBUG, "curSeq = %d\n flag:%s", curSeq, strflag);
      if (lbuf->rxBufState)
	{
	  if (lbuf->rxActiveList->tail != NULL_NODE)
	    {
	      prevFrame = (struct frameSt *) (lbuf->rxActiveList->tail);
	      expSeq = (prevFrame->seqNum + 1) & 0xFFFF;
	    }
	  else
	    {
	      expSeq = (lbuf->rxSuccessSeq + 1) & 0xFFFF;
	    }

	  if (expSeq != curSeq)
	    {
	      slog (0, SLOG_DEBUG,
		    "------> frame missing !!, rxSuccess = %d, expSeq = %d, curSeq = %d\n",
		    lbuf->rxSuccessSeq, expSeq, curSeq);
	      nackFlag = 1;
	      nackSeq = expSeq;
	    }
	}
      else
	lbuf->rxBufState = 1;

      listNodeInsert (lbuf->rxActiveList, lbuf->rxActiveList->tail,
		      (struct nodeSt *) frame);

      if (nackFlag)
	{
	  while (cnt < (curSeq - expSeq))
	    {
	      regCtrlEvent (linfo, TYPE_NACK, nackSeq);
	      sendControl (linfo);
	      cnt++;
	      nackSeq++;
	    }
	}
    }
}

void
processRxFrame (linkParam_t * linfo, unsigned char *data, int len)
{

  if (data[0] & CTRL_FRAME)
    {				// control packet
      processCtrlFrame (linfo, data, len);
    }
  else
    {				// data packet
      processDataFrame (linfo, data, len);
    }
}

void
bufferWrite (linkBuf_t *lbuf,unsigned char *data, int len, int seq, int flag, 
             int op)
{

  int i;
  int pLen = 0;
  int curSeq, expSeq;
  struct frameSt *frame, *prevFrame;
  int nackFlag = 0;
  int nackSeq = 0;
  char tempbuf[100];

  slog (0, SLOG_INFO, "%s\n", __FUNCTION__);
  if (op == OP_TX)
    {
      slog (0, SLOG_DEBUG, "OP_TX : ");
      frame = (struct frameSt *) listNodeGet (lbuf->txIdleList);
      frame->data[pLen++] = (unsigned char) flag;
      frame->data[pLen++] =  (seq) & 0xFF;
      for (i = 0; i < len; i++, pLen++)
	     frame->data[pLen] = data[i];
      frame->seqNum = seq;
      frame->len = pLen;
      slog (0, SLOG_DEBUG, "buffer write len:%d, plen:%d\n", len, pLen);
      listNodeInsert (lbuf->txActiveList, lbuf->txActiveList->tail,
		      (struct nodeSt *) frame);
    }
}

int
bufferRead (linkBuf_t * lbuf, unsigned char *buf, int op, int txMsqId)
{

  int i, nodeNum;
  struct frameSt *frame;

  slog (0, SLOG_INFO, "%s :op  %d", __FUNCTION__, op);

  if (op == OP_TX)
    {
      slog (0, SLOG_DEBUG, "OP_TX : ");
      if (lbuf->txActiveList->nodeNum <= 0)
	return -1;
      frame = (struct frameSt *) listNodeGet (lbuf->txActiveList);
      for (i = 0; i < frame->len; i++)
	buf[i] = frame->data[i];
      slog (0, SLOG_DEBUG, "%02X %02X \n", buf[0], buf[1]);
      if (buf[0] & 0xC0)
	listNodeInsert (lbuf->txIdleList, lbuf->txWaitList->tail,
			(struct nodeSt *) frame);
      else
	listNodeInsert (lbuf->txWaitList, lbuf->txWaitList->tail,
			(struct nodeSt *) frame);
      bufferStatusReport(lbuf);
      return frame->len;
    }
  else if (op == OP_RX)
    {
      slog (0, SLOG_INFO, "[%s]OP_RX\n", __FUNCTION__);
      if (lbuf->rxActiveList->nodeNum <= 0)
	    return -1;
      frame = (struct frameSt *) listNodeGet (lbuf->rxActiveList);
      for (i = 0; i < frame->len; i++)
	    buf[i] = frame->data[i];
      //lbuf->rxSuccessSeq = ((frame->data[1] & 0xFF) << 8) + frame->data[2];
      lbuf->rxSuccessSeq = frame->seqNum;
      slog (0, SLOG_DEBUG, "update rxSuccessSeq into %d\n", lbuf->rxSuccessSeq);
      listNodeInsert (lbuf->rxIdleList, lbuf->rxIdleList->tail, (struct nodeSt *) frame);
      // to send ack message if exceeded interval
      lbuf->rxSuccessCount++;
      bufferStatusReport(lbuf);
      return frame->len;
    }
}

struct frameSt *
bufferWaitScan (linkBuf_t *lbuf, int op, int seq)
{

  struct frameSt *frame = NULL_FRAME;

  if (op == OP_TX)
    {
      if (lbuf->txWaitList->nodeNum == 0)
	return NULL_FRAME;
      frame = (struct frameSt *) lbuf->txWaitList->head;
    }
  else
    {
      if (lbuf->rxWaitList->nodeNum == 0)
	return NULL_FRAME;
      frame = (struct frameSt *) lbuf->rxWaitList->head;
    }

  while (frame != NULL_FRAME)
    {
      if (frame->seqNum == seq)
	return frame;
      else
	frame = (struct frameSt *) frame->node.nNode;
    }

  return NULL_FRAME;
}

int
bufferActiveScan (linkBuf_t *lbuf, int op, int seq, struct frameSt **prevFrame)
{

  struct frameSt *frame = NULL_FRAME, *pFrame;
  int prevSeq, nextSeq;

  if (op == OP_TX)
    return -1;
  if (lbuf->rxActiveList->nodeNum == 0)
    return -1;

  frame = (struct frameSt *) lbuf->rxActiveList->head;

  prevSeq = lbuf->rxSuccessSeq;
  nextSeq = frame->seqNum;

  while (frame != NULL_FRAME)
    {
      if (prevSeq < nextSeq)
	{
	  if (seq > prevSeq && seq < nextSeq)
	    {
	      *prevFrame = (struct frameSt *) frame->node.pNode;
	      return 0;
	    }
	}
      else if (nextSeq < prevSeq)
	{
	  if (seq > prevSeq && seq <= 0xFFFF)
	    {
	      *prevFrame = (struct frameSt *) frame->node.pNode;
	      return 0;
	    }
	  else if (seq < nextSeq && seq >= 0)
	    {
	      *prevFrame = (struct frameSt *) frame->node.pNode;
	      return 0;
	    }
	}
      frame = (struct frameSt *) frame->node.nNode;
      pFrame = (struct frameSt *) frame->node.pNode;
      prevSeq = pFrame->seqNum;
      nextSeq = frame->seqNum;
    }
  slog (0, SLOG_DEBUG, "[%s] prevSeq:%d,nextSeq%d", __FUNCTION__, prevSeq,
	nextSeq);
  return -1;
}

void
ackProcessing (linkBuf_t *lbuf, int seq)
{

  struct frameSt *frame;

  while (lbuf->txWaitList->nodeNum > 0)
    {
      frame = (struct frameSt *) lbuf->txWaitList->head;
      slog (0, SLOG_INFO, "free seq %d\n", frame->seqNum);
      frame = (struct frameSt *) listNodeGet (lbuf->txWaitList);
      listNodeInsert (lbuf->txIdleList, lbuf->txIdleList->tail, (struct nodeSt *) frame);
      if (frame->seqNum == seq)
	break;
    }
  bufferStatusReport(lbuf);
}

int
bufferState (linkBuf_t *lbuf, int op)
{

  int state = 0;
  int seq1 = 0, seq2 = 0;
  struct frameSt *frame1, *frame2;

  slog (0, SLOG_INFO, "%s : ", __FUNCTION__);
  if (op == OP_TX)
    {
      state = (lbuf->txActiveList->nodeNum != 0) ? 1 : 0;
      slog (0, SLOG_DEBUG, "TX : nodeNum = %d, state = %d\n",
	    lbuf->txActiveList->nodeNum, state);
    }
  else
    {
      if (lbuf->rxActiveList->nodeNum > 0)
	{
	  if (lbuf->rxBufState == 2)
	    {
	      frame1 = (struct frameSt *) (lbuf->rxActiveList->head);
	      seq1 = (lbuf->rxSuccessSeq + 1) & 0xFFFF;
	      seq2 = frame1->seqNum;
	      if (seq1 == seq2)
		{
		  state = 1;
		}
	      slog (0, SLOG_DEBUG,
		    "[%s] rxbufstate :2 seq1:%d seq:%d buffstate:%d\n",
		    __FUNCTION__, seq1, seq2, state);
	    }
	  else if (lbuf->rxBufState == 1)
	    {
	      state = 1;
	      lbuf->rxBufState = 2;
	    }
	}
      slog (0, SLOG_DEBUG,
	    "[%s]RX : nodeNum = %d, seq1=%d, seq2=%d, rxbufstate:%d state=%d\n",
	    __FUNCTION__, lbuf->rxActiveList->nodeNum, seq1, seq2, lbuf->rxBufState,
	    state);
    }
  return state;
}
