#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <stdlib.h>
#include <string.h> 
#include <unistd.h>
#include <time.h>

void DieWithError(char *errorMessage);  /* Error handling function */

int main(int argc, char *argv[]) {
    int sock;                        /* Socket descriptor */
    struct sockaddr_in echoServAddr; /* Echo server address */
    unsigned short echoServPort;     /* Echo server port */
    char *servIP;                    /* Server IP address (dotted quad) */
    char echoString[30];
    char numberStr[3];
    unsigned int echoStringLen;
    struct sockaddr_in echoClntAddr;
    int bytesRcvd, totalBytesRcvd;

    if (argc != 4) {
       fprintf(stderr, "Usage: %s <Server IP> <Customers number> <Echo Port>\n",
               argv[0]);
       exit(1);
    }

    servIP = argv[1];
    char *p;
    long customers_kol = strtol(argv[2], &p, 10); /* Second arg: cust_num */
    echoServPort = atoi(argv[3]);

    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    memset(&echoServAddr, 0, sizeof(echoServAddr));
    echoServAddr.sin_family      = AF_INET;
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);
    echoServAddr.sin_port        = htons(echoServPort);

    strcpy(echoString, "0");
    strcat(echoString, numberStr);
    echoStringLen = strlen(echoString);

    for (int i = 0; i <= customers_kol; ++i) {
        sprintf(echoString, "%d", i);

        if (i != 0) {
            printf("customer %d has arrived\n", i);
        }
        echoStringLen = strlen(echoString);

        sendto(sock, echoString, echoStringLen, 0, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr));

        srand(time(NULL) * (i + 2));
        sleep(rand() % 3 + 1);
    }

    sendto(sock, "0", 1, 0, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr));

    close(sock);
    exit(0);
}