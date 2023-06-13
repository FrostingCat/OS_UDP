#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdio.h>      /* for printf() and fprintf() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <sys/socket.h> /* for socket(), bind(), and connect() */
#include <unistd.h>     /* for close() */

void DieWithError(char *errorMessage);
void handle(int *queue1, int *queue2, int servSock, struct sockaddr_in echoServAddr);
int CreateUDPServerSocket(unsigned short port);