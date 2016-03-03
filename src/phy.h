#ifndef __PHY_H__
#define __PHY_H__

void SendToChannel (unsigned char *buf, int len, int sockId);
void phyTx(linkParam_t *linfo);
void phyReTx(linkParam_t *linfo, unsigned char *buf, int len) ;
void phyRx(linkParam_t *linfo) ;

#endif
