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


linkParam_t rx_machine;

int rxMachine(void) {

   sprintf(rx_machine.idString, "rxMachine");
   sprintf(rx_machine.linkTxName, RX_MACHINE_TX);
   sprintf(rx_machine.linkRxName, RX_MACHINE_RX);
   linkInit(&rx_machine);
   appRx(&rx_machine);
   return 0;
}

static void 
catch_interrupt(int signal) 
{
    int s;
    printf ("User Interrupt signal caught Terminating RxMachine\n");
    // cancel the link recv thread
    s = pthread_cancel(rx_machine.linkRxThrdId);
    if (s != 0)
        perror("pthread_cancel");
    linkDeInit (&rx_machine);
    exit (1); 
}

int main() {

  if (signal(SIGINT, catch_interrupt) == SIG_ERR)
     printf("\ncan't catch SIGINT\n");
   rxMachine();
   //pthread_exit(NULL);
   pthread_join(rx_machine.linkRxThrdId, NULL);
   return 0;
}
