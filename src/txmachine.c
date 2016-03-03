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


linkParam_t tx_machine;

int txMachine(void) {

   int i;
 
   sprintf(tx_machine.idString, "txMachine");
   sprintf(tx_machine.linkTxName, TX_MACHINE_TX);
   sprintf(tx_machine.linkRxName, TX_MACHINE_RX);
   linkInit(&tx_machine);
   appTx(&tx_machine, i);
   usleep(10000);
   return 0;
}

static void 
catch_interrupt(int signal) 
{
    int s;
    printf ("User Interrupt signal caught Terminating TxMachine\n");
    // cancel the link recv thread
    s = pthread_cancel(tx_machine.linkRxThrdId);
    if (s != 0)
        perror("pthread_cancel");
    linkDeInit (&tx_machine);
    exit (1); 
}

int main() {

  if (signal(SIGINT, catch_interrupt) == SIG_ERR)
     printf("\ncan't catch SIGINT\n");
   txMachine();
   pthread_join(tx_machine.linkRxThrdId, NULL);
   //pthread_exit(NULL);
   return 0;
}
