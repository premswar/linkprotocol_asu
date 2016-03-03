#ifndef __COMMON_H__
#define __COMMON_H__
#include <sys/socket.h>
#include <sys/un.h>
#include "slog.h"
#include <semaphore.h>  /* Semaphore */

#define DATA_FRAME 0x00
#define CTRL_FRAME 0x80
#define FIRST_FRAME 0x00
#define RETRANS_FRAME 0x40
#define OP_TX	0
#define OP_RX	1
#define OP_CTRL	2
#define TX_MACHINE_TX  "/tmp/tx_m_tx"
#define TX_MACHINE_RX  "/tmp/tx_m_rx"
#define RX_MACHINE_TX  "/tmp/rx_m_tx"
#define RX_MACHINE_RX  "/tmp/rx_m_rx"
#define CHANNEL_RX  "/tmp/ch_rx"
#define CHANNEL_TX  "/tmp/ch_tx"
#define MAX_MSG_SIZE 256
#define MAX_FRAME 2000
#define MAX_EVENT 100
#define ACK_INTERVAL 20
#define MAX_PKT_SIZE 2048

struct msgSt {
   long int msgType;
   char msg[4096];
};

extern int socket_create (const char *filename);
#endif
