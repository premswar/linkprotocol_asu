#ifndef __LINK_H__
#define __LINK_H__

#include "list.h"
#include <pthread.h>
#include "timer.h"
#include "common.h"

#define START_FRAME 2
#define END_FRAME   1
#define INT_FRAME   0

#define TYPE_NACK 0x8000
#define TYPE_ACK  0x0000

struct frameSt
{
  struct nodeSt node;
  unsigned char data[100];
  unsigned int seqNum;
  int len;
};
typedef struct linkBuf_s
{
  
  unsigned char rxBufState;
  /*TODO IDEALLY These 2 params should be in link param structures
   *But as link code and buffercode has been so tightly coupled, 
   *It is more efficient to keep in link buf sturcture. In future 
   *If there is chance to clean up this code decouple two layers
   *Remove the below 2 variables and keep them linkparam struct  */
  unsigned int rxSuccessCount;
  unsigned int rxSuccessSeq;
  
  struct listSt *txIdleList;
  struct listSt *txActiveList;
  struct listSt *txWaitList;
  struct frameSt *txFrameFile[MAX_FRAME];
  struct listSt *rxIdleList;
  struct listSt *rxActiveList;
  struct listSt *rxWaitList;
  struct frameSt *rxFrameFile[MAX_FRAME];
} linkBuf_t;

typedef struct ctrlEvntSt
{
  unsigned char event[3];
} ctrlEvntSt_t;

typedef struct linkParam_s
{
  int txSockId;
  int rxServSockId;
  char idString[100];
  char linkTxName[25];
  char linkRxName[25];
  unsigned int seqNum ;
  ctrlEvntSt_t ctrlEv[MAX_EVENT];
  int evCount;
  linkBuf_t lbuf;
  sem_t recvSema;
  sem_t recvDoneSema;
  char *sendBuf;
  char *recvBuf;
  int recvBufLen; 
  pthread_t linkRxThrdId;
  ltimer_t reassembleTimer;
  ltimer_t nackTimer;
} linkParam_t;

#define NULL_FRAME ((struct frameSt *)-1)

//void ctrlEventInit ();
void regCtrlEvent (linkParam_t *linfo, int type, int seq);
void sendControl (linkParam_t * linfo);
void retransmission (linkParam_t * linfo, int seq, int msqId);
void linkInit (linkParam_t * lparam);
void linkDeInit (linkParam_t * lparam);
void linkSend (linkParam_t * linfo, unsigned char *buf, int len);
void linkRecv (linkParam_t * linfo, unsigned char *buf, int *len);
void linkTx (linkParam_t * linfo, unsigned char *buf, int len);
void linkRx (linkParam_t * linfo, unsigned char *buf, int len);
void updateRecvr(linkParam_t * linfo, unsigned char *buf, int len);

#endif
