#include <arpa/inet.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define SHM_NAME3 "/queue3"

int *mass1;
int shm_fd_3;

#define RCVBUFSIZE 32

void DieWithError(char *errorMessage);

int main(int argc, char *argv[]) {

    int sock;
    struct sockaddr_in echoServAddr;
    unsigned short echoServPort;
    char *servIP;
    char echoString[30];
    char numberStr[3];
    char echoBuffer[RCVBUFSIZE];
    unsigned int echoStringLen;
    int bytesRcvd, totalBytesRcvd;
    struct sockaddr_in echoClntAddr;
    unsigned int cliAddrLen;
    int place = 0;
    
    shm_fd_3 = shm_open(SHM_NAME3, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_3, 4);
    mass1 = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_3, 0);
    
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Server IP> <Echo Port>\n", argv[0]);
        exit(1);
    }
    
    servIP = argv[1];
    echoServPort = atoi(argv[2]);
    
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");
    
    memset(&echoServAddr, 0, sizeof(echoServAddr));
    echoServAddr.sin_family      = AF_INET;
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);
    echoServAddr.sin_port        = htons(echoServPort + 1);
    
    strcpy(echoString, "1");
    echoStringLen = strlen(echoString);
    
    // отправляем сообщение, чтобы сервер понимал, что кассир1 стучится на сервер
    if (sendto(sock, echoString, echoStringLen, 0, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) != echoStringLen)
        DieWithError("send() sent a different number of bytes than expected");
    printf("i am cashier1 and i am ready\n");
    
    cliAddrLen = sizeof(echoServAddr);
    // получаем сообщение, что подошел клиент
    if ((bytesRcvd = recvfrom(sock, echoBuffer, RCVBUFSIZE - 1, 0,
                            (struct sockaddr *)&echoServAddr, &cliAddrLen)) < 0)
    DieWithError("recv() failed or connection closed prematurely");
    echoBuffer[bytesRcvd] = '\0';
    
    while (echoBuffer[0] != '0') { // если клиенты еще есть
        printf("cashier1 is serving a client number %d\n", mass1[place]);
        place++;
        
        srand(time(NULL));
        sleep(rand() % 3 + 2);
        
        // сообщаем о готовности принять следующего клиента
        if (sendto(sock, echoString, echoStringLen, 0,
                   (struct sockaddr *)&echoServAddr,
                   sizeof(echoServAddr)) != echoStringLen)
          DieWithError("send() sent a different number of bytes than expected");
        printf("cashier1 is ready for a new client\n");
        
        // получаем информацию о следующем клиенте
        bytesRcvd = recvfrom(sock, echoBuffer, RCVBUFSIZE - 1, 0, (struct sockaddr *)&echoServAddr, &cliAddrLen);
        echoBuffer[bytesRcvd] = '\0';
    }
    printf("cashier1 is leaving\n");
    
    close(sock);
    munmap(mass1, 4);
    close(shm_fd_3);
    shm_unlink(SHM_NAME3);
    exit(0);
}