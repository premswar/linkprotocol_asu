#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<string.h>
#include <errno.h>

void main(){

    int rfile,wfile;
    int ct = 0,i=0,tam;
    unsigned char byte[1];
    unsigned char *imbuff;
    rfile= open ("prem.jpg", O_RDWR , 0600);
     
     imbuff = (char *)malloc(2048*sizeof(unsigned char));
     while( tam=read(rfile, &byte, 1)>0)
     {
            memcpy(&imbuff[ct],&byte, sizeof(unsigned char));
            ct++;
     }
     printf("finished reading %d bytes\n", ct);
     close(rfile);
    remove("prem_w.jpg");    
    wfile= open ("prem_w.jpg", O_RDWR | O_CREAT , 0600);
    if(wfile <0){
         perror("open err");
         return;
      }
      if(write(wfile, imbuff, ct) < 0)
         perror("write err");
    /* while( i < ct)
     {
         if(write(wfile, &imbuff[i], 1) < 0)
            perror("write err");
         i++;
     }*/
     
     printf("finished  writing %d bytes\n", ct);
     close(wfile);
     return;
}
