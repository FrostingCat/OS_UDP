#include "server.h"  /* TCP echo server includes */
#include <sys/wait.h>       /* for waitpid() */
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <string.h>
#include <pthread.h>

#define RCVBUFSIZE 32
#define MAX_PROCESSES 4

#define SHM_NAME1 "/queue1"
#define SHM_NAME2 "/queue2"

int* queue1;
int* queue2;
int shm_fd_1;
int shm_fd_2;

void my_handler(int nsig) {
    munmap(queue1, 400);
    close(shm_fd_1);
    shm_unlink(SHM_NAME1);

    munmap(queue2, 400);
    close(shm_fd_2);
    shm_unlink(SHM_NAME2);

    exit(0);
}

int main(int argc, char *argv[]) {

    signal(SIGINT, my_handler);

    int clntSock;
    unsigned short echoServPort;
    pid_t processID;
    unsigned int childProcCount = 0;
    struct ThreadArgs *threadArgs;

    if (argc != 2)  {
        fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]);
        exit(1);
    }

    shm_fd_1 = shm_open(SHM_NAME1, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_1, 400);
    queue1 = mmap(NULL, 400, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_1, 0);

    shm_fd_2 = shm_open(SHM_NAME2, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_2, 400);
    queue2 = mmap(NULL, 400, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_2, 0);

    queue1[1] = 1;
    queue2[1] = 1;
    queue1[0] = 0;
    queue2[0] = 0;

    echoServPort = atoi(argv[1]);

    struct sockaddr_in echoServAddr;

    for (int i = 0; i < MAX_PROCESSES; ++i) {        
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(1);
        } else if (pid == 0) {
            int servSock;
            servSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            
            echoServAddr.sin_family = AF_INET;
            echoServAddr.sin_addr.s_addr = INADDR_ANY;
            echoServAddr.sin_port = htons(echoServPort + i);
            if (bind(servSock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
                DieWithError("bind() failed");
            handle(queue1, queue2, servSock, echoServAddr);
            close(servSock);
            exit(0);
        }
    }

    // Родительский процесс ожидает завершения дочерних процессов
    for (int i = 0; i < MAX_PROCESSES; i++) {
        wait(NULL);
    }

    munmap(queue1, 400);
    close(shm_fd_1);
    shm_unlink(SHM_NAME1);

    munmap(queue2, 400);
    close(shm_fd_2);
    shm_unlink(SHM_NAME2);
}