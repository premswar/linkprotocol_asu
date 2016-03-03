#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "common.h"
#include "link.h"

void bufferInit(linkBuf_t * lbuf);
void bufferDeInit(linkBuf_t * lbuf); 
int bufferState(linkBuf_t *lbuf, int op);
void viewRxBuffLists(linkBuf_t * lbuf);
void writeToTxBuf(linkBuf_t * lbuf, struct frameSt *txframe);
void bufferWrite(linkBuf_t *lbuf, unsigned char *data, int len, int seq, int flag, int op);
int bufferRead(linkBuf_t * lbuf, unsigned char *buf, int op, int msqId);
struct frameSt *bufferWaitScan(linkBuf_t *lbuf, int op, int seq);
int bufferActiveScan(linkBuf_t *lbuf, int op, int seq, struct frameSt **prevFrame);
void ackProcessing(linkBuf_t *lbuf, int seq);

#endif
