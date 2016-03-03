#include <stdio.h>
#include <stdlib.h>
#include "link.h"

extern char idString[];
unsigned char packet[2048];

void appRx(unsigned char *buf, int len) {

   int i;
   unsigned short pLen;
   int diff = 0;

//printf("%s()\n", __FUNCTION__);

   pLen = buf[0]*0x100 + buf[1];;
   if(pLen > len) return;

   for(i=2; i<pLen; i++) {
      if(buf[i] != (i&0xFF)) diff = 1;
   }

   if(diff) {
      printf("%s : RX error in the packet with %d bytes\n", idString, pLen);
      for(i=0; i<pLen; i++) {
         printf("%d %03d %03d ", i, buf[i], i);
         printf("%s", (buf[i] == (i&0xFF)) ? "\n" : "ERR\n");
      }
      printf("\n");
      exit(1);
   }
   else printf("=============> Success when len = %d\n\n", pLen);

//bufferStatusReport();

}

void appTx(linkParam_t *tx_machine, int len) {

   int i;

   //printf("%s\n", __FUNCTION__);
   packet[0] = (len >> 8) & 0xFF;
   packet[1] = (len) & 0xFF;

   for(i=2; i<len; i++) packet[i] = (unsigned char)i;
   printf("\n appTx-firstbyte:%02x lastbyte:%02x\n",packet[0],packet[len-1]); 
   linkTx(tx_machine, packet, len);
}
