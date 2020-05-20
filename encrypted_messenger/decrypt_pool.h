#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

// Work object, linked list of file connections
struct work {
        int connection;
        struct work* next;
};

typedef struct work work_t;

// Pool object, manages the work and workers
struct pool {
        size_t numberOfThreads;
        work_t* firstWork;
        work_t* lastWork;
        pthread_mutex_t workMutex;
        pthread_cond_t workCond;
};

typedef struct pool threadPool;

void error(const char *msg);

threadPool* createPool(size_t numberOfThreads);

void destroyPool(threadPool* threadManager);
void addWork(int newConnection, threadPool* threadManager);
void addConnection();
