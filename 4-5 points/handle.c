#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <string.h>
#include <arpa/inet.h>

#define SHM_NAME3 "/queue3"
#define SHM_NAME4 "/queue4"

int* mass1;
int* mass2;
int shm_fd_3;
int shm_fd_4;

#define RCVBUFSIZE 32

void DieWithError(char *errorMessage);

void HandleClients(struct sockaddr_in echoClntAddr, int clntSocket, int *queue1, int *queue2) {
    char echoBuffer[RCVBUFSIZE];
    int recvMsgSize;
    unsigned int cliAddrLen;
    int place1 = 0, place2 = 0;
    
    shm_fd_3 = shm_open(SHM_NAME3, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_3, 4);
    mass1 = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_3, 0);
    
    shm_fd_4 = shm_open(SHM_NAME4, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_4, 4);
    mass2 = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_4, 0);

    queue1[0] = 0;
    queue2[0] = 0;

    cliAddrLen = sizeof(echoClntAddr);

    while (1) {
        struct sockaddr_in client;
        unsigned int clientLen = sizeof(client);
        if ((recvMsgSize = recvfrom(clntSocket, echoBuffer, RCVBUFSIZE, 0,
            (struct sockaddr *) &client, &clientLen)) < 0)
            DieWithError("recvfrom1() failed");
        echoBuffer[recvMsgSize] = '\0';
        if (echoBuffer[0] == '0') {
            break;
        }
        printf("customer %s has arrived\n", echoBuffer);
        int person = atoi(echoBuffer);

        srand(time(NULL));
        int random_cashier = rand() % 2;
        if (random_cashier == 0) {
            queue1[0]++;
            mass1[place1] = person;
            place1++;
        } else {
            queue2[0]++;
            mass2[place2] = person;
            place2++;
        }

        random_cashier++;
        printf("customer %s goes to queue %d\n", echoBuffer, random_cashier);
    }

    printf("no clients anymore\n");
    queue1[1] = 0;
    queue2[1] = 0;

    close(clntSocket);
    munmap(mass1, 4);
    close(shm_fd_3);
    shm_unlink(SHM_NAME3);
    munmap(mass2, 4);
    close(shm_fd_4);
    shm_unlink(SHM_NAME4);
}

void HandleCashier1(struct sockaddr_in echoClntAddr, int clntSocket, int *queue1) {
    char echoBuffer[RCVBUFSIZE];
    int recvMsgSize;
    char echoString[30]; 
    unsigned int echoStringLen;
    unsigned int cliAddrLen;
    int place = 0;

    shm_fd_3 = shm_open(SHM_NAME3, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_3, 4);
    mass1 = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_3, 0);

    while (1) {
        printf("cashier1 is ready\n");
        while (queue1[0] == 0) {
            if (queue1[1] == 0) {
                break;
            }
            sleep(1);
        }

        if (queue1[1] == 0 && queue1[0] == 0) {
            int number = 0;
            printf("cashier1 goes home\n");
            sprintf(echoString, "%d", number);
            echoStringLen = strlen(echoString);
            if (sendto(clntSocket, echoString, echoStringLen, 0, (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != echoStringLen)
                DieWithError("send() sent a different number of bytes than expected");
            break;
        }

        int number = 1;
        sprintf(echoString, "%d", number);
        echoStringLen = strlen(echoString);
        
        // отправляем сообщение о начале обслуживания
        if (sendto(clntSocket, echoString, echoStringLen, 0, (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != echoStringLen)
            DieWithError("send() sent a different number of bytes than expected");
        printf("customer %d is being served by cashier1\n", mass1[place]);

        cliAddrLen = sizeof(echoClntAddr);

        // получаем сообщение об окончании обслуживания
        if ((recvMsgSize = recvfrom(clntSocket, echoBuffer, RCVBUFSIZE, 0,
            (struct sockaddr *) &echoClntAddr, &cliAddrLen)) < 0)
            DieWithError("recvfrom() failed");

        queue1[0]--;
        place++;

        echoBuffer[recvMsgSize] = '\0';
    }

    close(clntSocket);
    munmap(mass1, 4);
    close(shm_fd_3);
    shm_unlink(SHM_NAME3);
}

void HandleCashier2(struct sockaddr_in echoClntAddr, int clntSocket, int *queue2) {
    char echoBuffer[RCVBUFSIZE];
    int recvMsgSize;
    char echoString[30]; 
    unsigned int echoStringLen;
    unsigned int cliAddrLen;
    int place = 0;

    shm_fd_4 = shm_open(SHM_NAME4, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_4, 4);
    mass2 = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_4, 0);

    while (1) {
        printf("cashier2 is ready\n");
        while (queue2[0] == 0) {
            if (queue2[1] == 0) {
                break;
            }
            sleep(1);
        }

        if (queue2[1] == 0 && queue2[0] == 0) {
            printf("cashier2 goes home\n");
            int number = 0;
            sprintf(echoString, "%d", number);
            echoStringLen = strlen(echoString);
            if (sendto(clntSocket, echoString, echoStringLen, 0, (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != echoStringLen)
                DieWithError("send() sent a different number of bytes than expected");
            break;
        }

        int number = 1;
        sprintf(echoString, "%d", number);
        echoStringLen = strlen(echoString);
        
        // отправляем сообщение о начале обслуживания
        if (sendto(clntSocket, echoString, echoStringLen, 0, (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != echoStringLen)
            DieWithError("send() sent a different number of bytes than expected");
        printf("customer %d is being served by cashier2\n", mass2[place]);

        cliAddrLen = sizeof(echoClntAddr);

        // получаем сообщение об окончании обслуживания
        if ((recvMsgSize = recvfrom(clntSocket, echoBuffer, RCVBUFSIZE, 0,
            (struct sockaddr *) &echoClntAddr, &cliAddrLen)) < 0)
            DieWithError("recvfrom() failed");

        queue2[0]--;
        place++;

        echoBuffer[recvMsgSize] = '\0';
    }

    close(clntSocket);
    munmap(mass2, 4);
    close(shm_fd_4);
    shm_unlink(SHM_NAME4);
}

void handle(int *queue1, int *queue2, int clientSocket, struct sockaddr_in echoServAddr) {
    socklen_t clientAddressLength = sizeof(echoServAddr);
    char buffer[10];

    int bytesRead = recvfrom(clientSocket, buffer, 9, 0, (struct sockaddr *)&echoServAddr, &clientAddressLength);
    buffer[bytesRead] = '\0';
    char s = buffer[0];
    
    if (s == '0') {
        HandleClients(echoServAddr, clientSocket, queue1, queue2);
    } else if (s == '1') {
        HandleCashier1(echoServAddr, clientSocket, queue1);
    } else if (s == '2') {
        HandleCashier2(echoServAddr, clientSocket, queue2);
    } 
}