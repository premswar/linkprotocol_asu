#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>

#include "buffer.h"
#include "app.h"
#include "phy.h"

#define BUFSIZE 50

char idString[100];

struct my_msg_st {
   long int my_msg_type;
   char text[BUFSIZ];
};


linkParam_t tx_machine;
linkParam_t rx_machine;

int trMsqId;
int rtMsqId;

int txMachine(void) {

   int i;
 
   sprintf(tx_machine.idString, "txMachine");
   sprintf(tx_machine.linkTxName, TX_MACHINE_TX);
   sprintf(tx_machine.linkRxName, TX_MACHINE_RX);

   linkInit(&tx_machine);
   for(i=94; i<97; i++) {
      printf("==============> msg_len [%d]:\n", i);
      appTx(i, tx_machine.txSockId);
      //appTx(i, trMsqId);
      usleep(10000);
   }
   phyRx(tx_machine.rxServSockId, tx_machine.txSockId);

   return 0;
}

int rxMachine(void) {

   sprintf(rx_machine.idString, "rxMachine");
   
   sprintf(rx_machine.linkTxName, RX_MACHINE_TX);
   sprintf(rx_machine.linkRxName, RX_MACHINE_RX);
   linkInit(&rx_machine);
   //linkInit();
   //phyRx(trMsqId, rtMsqId);
   phyRx(rx_machine.rxServSockId, rx_machine.txSockId);

   return 0;
}

int main() {

   pid_t pid;
   struct msgSt rxMsg;
   int len;

   /*trMsqId = msgget((key_t)1234, 0666 | IPC_CREAT);
   if (trMsqId == -1) {
      perror("msgget failed with error");
      exit(EXIT_FAILURE);
   }

   rtMsqId = msgget((key_t)5678, 0666 | IPC_CREAT);
   if (rtMsqId == -1) {
      perror("msgget failed with error");
      exit(EXIT_FAILURE);
   }

   // purge msg queue between txMachine and rxMachine
   do {
      len = msgrcv(trMsqId, (void *)&rxMsg, sizeof(struct msgSt), 0, IPC_NOWAIT);
   } while(len != -1);
   do {
      len = msgrcv(rtMsqId, (void *)&rxMsg, sizeof(struct msgSt), 0, IPC_NOWAIT);
   } while(len != -1);*/


   pid = fork();
   if(pid == -1) { 
      printf("fork error !!\n");
      exit(1);
   }
   else if(pid == 0) { // child
      txMachine();
   }
   else { // parents
      rxMachine();
   }
}
