#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/un.h>
#define CHANNEL_RX  "ch_rx"
#define CHANNEL_TS  "ch_ts"

int main(void)
{
    int s, t, len, nbytes;
    struct sockaddr_un remote,saddr_chts;
    char str[100];

    if ((s = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
   saddr_chts.sun_family = AF_UNIX;
   strcpy(saddr_chts.sun_path, CHANNEL_TS);
   unlink(saddr_chts.sun_path);
   len = strlen(saddr_chts.sun_path) + sizeof(saddr_chts.sun_family);
   if (bind(s,(struct sockaddr *) &saddr_chts, len) == -1){
      perror("ch_ts_sock_bind");
      exit(-1);
   }

    printf("Trying to connect...\n");

    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, CHANNEL_RX);
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    /*if (connect(s, (struct sockaddr *)&remote, len) == -1) {
        perror("connect");
        exit(1);
    }*/

    //printf("Connected.\n");

    while(printf("> "), fgets(str, 100, stdin), !feof(stdin)) {
        if ((nbytes = sendto (s,str,strlen (str) + 1, 0,
                   (struct sockaddr *) & remote, len))<0)
    {
      perror ("sendto (client)");
      exit (1);
    }
    /*send(s, str, strlen(str), 0) == -1) {
            perror("send");
            fflush(stdout);
            exit(1);
        }

        if ((t=recv(s, str, 100, 0)) > 0) {
            str[t] = '\0';
            printf("echo> %s", str);
        } else {
            if (t < 0) perror("recv");
            else printf("Server closed connection\n");
            exit(1);
            fflush(stdout);
        }*/
    }

    close(s);
    fflush(stdout);

    return 0;
}
