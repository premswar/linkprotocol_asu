#include <stdio.h>
#include <stdlib.h>
#include "link.h"
#include<fcntl.h>
#include<string.h>
#include <errno.h>

//#define RFILE_N   "prem.jpg"
//#define WFILE_N   "prem_w.jpg"
#define RFILE_N   "output_4.mp4"
#define WFILE_N   "output_4W.mp4"

void printstatus(int crnt, int total);

void
appRx(linkParam_t *rx_machine)
{

  int wfile, len,offset,tlen =0,i = 0;
  unsigned short pLen;
  unsigned char *buf, *bufv;
 
   printf("%s()\n", __FUNCTION__);
   buf = (char *)malloc(2048*sizeof(unsigned char));
   bufv = (char *)malloc(2048*400*sizeof(unsigned char));
   while(1)
   {
     linkRecv(rx_machine, buf, &len);
     if(i == 0)
       printf("Reciving video file\n");
     if(len == 2048){
       offset = buf[0]*0x100 + buf[1];
       if(offset == i){
        //printf("RCVD PART:%d\n",i);
        memcpy(&bufv[i*2046], &buf[2], len-2);
        if(i%10 == 0)
         printstatus(i, 281); 
        i++;
        tlen = tlen+(len-2);
       }
       //else
        //printf("DUP COPY:%d [%d]\n",i,offset);
     continue;
   }
   //printf("RCVD PART:%d\n",i);
   memcpy(&bufv[i*2046], &buf[2], len-2);
   tlen = tlen+(len-2);
   printf("==========> Recieved %d bytes <===========\n", tlen);
   remove(WFILE_N);    
    wfile= open (WFILE_N, O_RDWR | O_CREAT , 0600);
    if(wfile <0){
         perror("open err");
         return;
      }
      if(write(wfile, (bufv), tlen) < 0)
         perror("write err");
     printf("finished  writing %d bytes\n", tlen);
     close(wfile);
   }
}

void
appTx (linkParam_t * tx_machine, int len)
{

    int rfile,i,offset;
    int ct = 0,tam,parts;
    unsigned char byte[1];
    unsigned char *imbuff, *part;
    

    rfile= open (RFILE_N, O_RDWR , 0600);
     printf("opened file\n");
     imbuff = (char *)malloc(2048*400*sizeof(unsigned char));
     part = (char *)malloc(2048*sizeof(unsigned char));//max pkt size
     printf("malloc success\n");
     while( tam=read(rfile, &byte, 1)>0)
     {
        //printf("read :%d\n", ct);
            memcpy(&imbuff[ct],&byte, sizeof(unsigned char));
            ct++;
     }
     if(tam < 0)
         perror("read err");
     //printf("finished reading %d bytes\n", ct);
     
     if(ct > 2048) // max packet size
     parts = ct/2046;
     printf("finished reading %d bytes\n", ct);
     printf("Sending video file in %d parts\n",parts);
     for(i = 0; i < parts; i++){
        offset = i;
        part[0] = (offset >> 8) & 0xFF;
        part[1] = (offset) & 0xFF;
        memcpy(part+2,&imbuff[i*2046],2046);
        linkSend(tx_machine, part, 2048);
        if(i%10 == 0)
         printstatus(i, parts); 
        usleep(90000);
     }
     offset = i;
     part[0] = (offset >> 8) & 0xFF;
     part[1] = (offset) & 0xFF;
     memcpy(part+2,&imbuff[i*2046],ct-(parts*2046));
     linkSend(tx_machine, part, ct-(parts*2046)+2);
     printf("============> Sent %d bytes <============\n", ct);
     //close(rfile);
     free(imbuff);
     return;
}
void printstatus(int crnt, int total)
{
    int progress = 0, factr;
    int barWidth = 70, pos, i;
    factr = total/10;
    progress = crnt/factr;
    if (progress <= 10.0) {
        printf( "[");
        pos = 0.1*barWidth * progress;
        for (i = 0; i < barWidth; ++i) {
            if (i < pos) printf("=");
            else if (i == pos) printf( ">");
            else printf(" ");
        }
        printf("] %d%% \r",(int)(progress * 10));
        fflush(stdout);
        //sleep(1);
        //progress += 0.16; // for demonstration only
    }   
    //printf("\n");
}

