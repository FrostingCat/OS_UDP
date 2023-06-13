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
#define SHM_NAME5 "/queue5"

int* mass1;
int* mass2;
int* logs;
int shm_fd_3;
int shm_fd_4;
int shm_fd_5;

#define RCVBUFSIZE 32

void DieWithError(char *errorMessage);

void HandleClients(struct sockaddr_in echoClntAddr, int clntSocket, int *queue1, int *queue2) {
    char echoBuffer[RCVBUFSIZE];
    int recvMsgSize;
    unsigned int cliAddrLen;
    int place1 = 0, place2 = 0;
    
    shm_fd_3 = shm_open(SHM_NAME3, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_3, 400);
    mass1 = mmap(NULL, 400, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_3, 0);
    
    shm_fd_4 = shm_open(SHM_NAME4, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_4, 400);
    mass2 = mmap(NULL, 400, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_4, 0);

    shm_fd_5 = shm_open(SHM_NAME5, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_5, 400);
    logs = mmap(NULL, 400, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_5, 0);

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
        int number = logs[0] + 1;
        logs[0] += 3;
        logs[number] = 0;
        logs[number + 1] = atoi(echoBuffer);
        logs[number + 2] = 0;
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

        number = logs[0] + 1;
        logs[0] += 3;
        logs[number] = 0;
        logs[number + 1] = atoi(echoBuffer);
        logs[number + 2] = random_cashier;
    }

    int number = logs[0] + 1;
    logs[0] += 3;
    logs[number] = 0;
    logs[number + 1] = 0;
    logs[number + 2] = 3;
    queue1[1] = 0;
    queue2[1] = 0;

    close(clntSocket);
    munmap(mass1, 400);
    close(shm_fd_3);
    shm_unlink(SHM_NAME3);
    
    munmap(mass2, 400);
    close(shm_fd_4);
    shm_unlink(SHM_NAME4);

    munmap(logs, 400);
    close(shm_fd_5);
    shm_unlink(SHM_NAME5);
}

void HandleCashier1(struct sockaddr_in echoClntAddr, int clntSocket, int *queue1) {
    char echoBuffer[RCVBUFSIZE];
    int recvMsgSize;
    char echoString[30]; 
    unsigned int echoStringLen;
    unsigned int cliAddrLen;
    int place = 0;

    shm_fd_3 = shm_open(SHM_NAME3, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_3, 400);
    mass1 = mmap(NULL, 400, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_3, 0);

    shm_fd_5 = shm_open(SHM_NAME5, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_5, 400);
    logs = mmap(NULL, 400, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_5, 0);

    while (1) {
        
        int n = logs[0] + 1;
        logs[0] += 3;
        logs[n] = 1;
        logs[n + 1] = 0;
        logs[n + 2] = 1;
        
        while (queue1[0] == 0) {
            if (queue1[1] == 0) {
                break;
            }
            sleep(1);
        }

        if (queue1[1] == 0 && queue1[0] == 0) {
            int number = 0;
            
            n = logs[0] + 1;
            logs[0] += 3;
            logs[n] = 1;
            logs[n + 1] = 0;
            logs[n + 2] = 0;
            
            sprintf(echoString, "%d", number);
            echoStringLen = strlen(echoString);
            if (sendto(clntSocket, echoString, echoStringLen, 0, (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != echoStringLen)
                DieWithError("sendto() sent a different number of bytes than expected");
            break;
        }

        int number = 1;
        sprintf(echoString, "%d", number);
        echoStringLen = strlen(echoString);
        
        // отправляем сообщение о начале обслуживания
        if (sendto(clntSocket, echoString, echoStringLen, 0, (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != echoStringLen)
            DieWithError("sendto() sent a different number of bytes than expected");

        n = logs[0] + 1;
        logs[0] += 3;
        logs[n] = 1;
        logs[n + 1] = mass1[place];
        logs[n + 2] = 2;

        cliAddrLen = sizeof(echoClntAddr);

        // получаем сообщение об окончании обслуживания
        if ((recvMsgSize = recvfrom(clntSocket, echoBuffer, RCVBUFSIZE, 0,
            (struct sockaddr *) &echoClntAddr, &cliAddrLen)) < 0)
            DieWithError("recvfrom() failed");

        queue1[0]--;
        place++;

        echoBuffer[recvMsgSize] = '\0';
    }

    logs[1] = 'f';

    close(clntSocket);
    munmap(mass1, 400);
    close(shm_fd_3);
    shm_unlink(SHM_NAME3);

    munmap(logs, 400);
    close(shm_fd_5);
    shm_unlink(SHM_NAME5);
}

void HandleCashier2(struct sockaddr_in echoClntAddr, int clntSocket, int *queue2) {
    char echoBuffer[RCVBUFSIZE];
    int recvMsgSize;
    char echoString[30]; 
    unsigned int echoStringLen;
    unsigned int cliAddrLen;
    int place = 0;

    shm_fd_4 = shm_open(SHM_NAME4, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_4, 400);
    mass2 = mmap(NULL, 400, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_4, 0);

    shm_fd_5 = shm_open(SHM_NAME5, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_5, 400);
    logs = mmap(NULL, 400, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_5, 0);

    while (1) {
        
        int n = logs[0] + 1;
        logs[0] += 3;
        logs[n] = 2;
        logs[n + 1] = 0;
        logs[n + 2] = 1;
        
        while (queue2[0] == 0) {
            if (queue2[1] == 0) {
                break;
            }
            sleep(1);
        }

        if (queue2[1] == 0 && queue2[0] == 0) {

            int n = logs[0] + 1;
            logs[0] += 3;
            logs[n] = 2;
            logs[n + 1] = 0;
            logs[n + 2] = 0;

            int number = 0;
            sprintf(echoString, "%d", number);
            echoStringLen = strlen(echoString);
            if (sendto(clntSocket, echoString, echoStringLen, 0, (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != echoStringLen)
                DieWithError("sendto() sent a different number of bytes than expected");
            break;
        }

        int number = 1;
        sprintf(echoString, "%d", number);
        echoStringLen = strlen(echoString);
        
        // отправляем сообщение о начале обслуживания
        if (sendto(clntSocket, echoString, echoStringLen, 0, (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr)) != echoStringLen)
            DieWithError("sendto() sent a different number of bytes than expected");

        n = logs[0] + 1;
        logs[0] += 3;
        logs[n] = 2;
        logs[n + 1] = mass2[place];
        logs[n + 2] = 2;

        cliAddrLen = sizeof(echoClntAddr);

        // получаем сообщение об окончании обслуживания
        if ((recvMsgSize = recvfrom(clntSocket, echoBuffer, RCVBUFSIZE, 0,
            (struct sockaddr *) &echoClntAddr, &cliAddrLen)) < 0)
            DieWithError("recvfrom() failed");

        queue2[0]--;
        place++;

        echoBuffer[recvMsgSize] = '\0';
    }

    logs[2] = 'f';

    close(clntSocket);
    munmap(mass2, 400);
    close(shm_fd_4);
    shm_unlink(SHM_NAME4);

    munmap(logs, 400);
    close(shm_fd_5);
    shm_unlink(SHM_NAME5);
}

void HandleListener(struct sockaddr_in echoClntAddr, int clntSocket) {
    char echoString[50]; 
    unsigned int echoStringLen;
    
    shm_fd_5 = shm_open(SHM_NAME5, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_5, 400);
    logs = mmap(NULL, 400, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_5, 0);

    logs[0] = 0;
    int place = 0;

    unsigned int cliAddrLen = sizeof(echoClntAddr);
    
    for (;;) {
        while (logs[0] == place) {
            sleep(1);
            if (logs[1] == 'f' && logs[2] == 'f') {
                sendto(clntSocket, "0", 1, 0, (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr));
                break;
            }
        }
        if (logs[1] == 'f' && logs[2] == 'f') {
                break;
            }
        while (place != logs[0]) {
            if (logs[place + 1] == 0) {
                if (logs[place + 2] == 0 && logs[place + 3] == 3) {
                    strcpy(echoString, "no clients anymore");
                    echoStringLen = strlen(echoString);
                    sendto(clntSocket, echoString, echoStringLen, 0, (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr));
                } else if (logs[place + 3] == 0) {
                    char cashNumber[3];
                    sprintf(cashNumber, "%d", logs[place + 2]);
                    strcpy(echoString, "customer ");
                    strcat(echoString, cashNumber);
                    strcat(echoString, " has arrived");
                    echoStringLen = strlen(echoString);
                    sendto(clntSocket, echoString, echoStringLen, 0, (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr));
                } else {
                    char cashNumber[3];
                    sprintf(cashNumber, "%d", logs[place + 2]);
                    char queueNumber[3];
                    sprintf(queueNumber, "%d", logs[place + 3]);
                    strcpy(echoString, "customer ");
                    strcat(echoString, cashNumber);
                    strcat(echoString, " goes to queue ");
                    strcat(echoString, queueNumber);
                    echoStringLen = strlen(echoString);
                    sendto(clntSocket, echoString, echoStringLen, 0, (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr));
                }  
            } else if (logs[place + 1] == 1) {
                if (logs[place + 2] == 0 && logs[place + 3] == 1) {
                    strcpy(echoString, "cashier1 is ready for a new customer");
                    echoStringLen = strlen(echoString);
                    sendto(clntSocket, echoString, echoStringLen, 0, (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr));
                } else if (logs[place + 3] == 0) {
                    strcpy(echoString, "cashier1 goes home");
                    echoStringLen = strlen(echoString);
                    sendto(clntSocket, echoString, echoStringLen, 0, (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr));
                } else {
                    char clientNumber[3];
                    sprintf(clientNumber, "%d", logs[place + 2]);
                    strcpy(echoString, "customer ");
                    strcat(echoString, clientNumber);
                    strcat(echoString, " is being served by cashier1");
                    echoStringLen = strlen(echoString);
                    sendto(clntSocket, echoString, echoStringLen, 0, (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr));
                }
            } else {
                if (logs[place + 2] == 0 && logs[place + 3] == 1) {
                    strcpy(echoString, "cashier2 is ready for a new customer");
                    echoStringLen = strlen(echoString);
                    sendto(clntSocket, echoString, echoStringLen, 0, (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr));
                } else if (logs[place + 3] == 0) {
                    strcpy(echoString, "cashier2 goes home");
                    echoStringLen = strlen(echoString);
                    sendto(clntSocket, echoString, echoStringLen, 0, (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr));
                } else {
                    char clientNumber[3];
                    sprintf(clientNumber, "%d", logs[place + 2]);
                    strcpy(echoString, "customer ");
                    strcat(echoString, clientNumber);
                    strcat(echoString, " is being served by cashier2");
                    echoStringLen = strlen(echoString);
                    sendto(clntSocket, echoString, echoStringLen, 0, (struct sockaddr *) &echoClntAddr, sizeof(echoClntAddr));
                }
            }
            place += 3;
            struct timespec tw = {0,300000000};
            struct timespec tr;
            nanosleep (&tw, &tr);
        }
    }

    close(clntSocket);

    munmap(logs, 400);
    close(shm_fd_5);
    shm_unlink(SHM_NAME5);
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
    } else if (s == '3') {
        HandleListener(echoServAddr, clientSocket);
    }
}