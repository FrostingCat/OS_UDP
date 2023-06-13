#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <stdlib.h>
#include <string.h> 
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <sys/fcntl.h>

#define RCVBUFSIZE 50

void DieWithError(char *errorMessage);

int main(int argc, char *argv[]) {
    
    int sock;
    struct sockaddr_in echoServAddr;
    unsigned short echoServPort;
    char *servIP;
    char echoString[50];
    char echoBuffer[RCVBUFSIZE];
    unsigned int echoStringLen;
    int bytesRcvd, totalBytesRcvd;
    int place = 0;

    if (argc != 3) {
       fprintf(stderr, "Usage: %s <Server IP> <Echo Port>\n",
               argv[0]);
       exit(1);
    }

    servIP = argv[1];
    echoServPort = atoi(argv[2]);

    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    memset(&echoServAddr, 0, sizeof(echoServAddr));
    echoServAddr.sin_family      = AF_INET;
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);
    echoServAddr.sin_port        = htons(echoServPort + 3);

    int number = 3;
    sprintf(echoString, "%d", number);
    echoStringLen = strlen(echoString);

    sendto(sock, echoString, echoStringLen, 0, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr));

    sleep(1);

    unsigned int cliAddrLen;
    cliAddrLen = sizeof(echoServAddr);
    if ((bytesRcvd = recvfrom(sock, echoBuffer, RCVBUFSIZE - 1, 0,
            (struct sockaddr *) &echoServAddr, &cliAddrLen)) < 0)
        DieWithError("recv() failed or connection closed prematurely");
    echoBuffer[bytesRcvd] = '\0';

    while (echoBuffer[0] != '0') {
        printf("%s\n", echoBuffer);
        if ((bytesRcvd = recvfrom(sock, echoBuffer, RCVBUFSIZE - 1, 0,
            (struct sockaddr *) &echoServAddr, &cliAddrLen)) < 0)
        DieWithError("recv() failed or connection closed prematurely");
    echoBuffer[bytesRcvd] = '\0';
    }

    close(sock);
    exit(0);
}