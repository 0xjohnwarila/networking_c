#pragma once

#include <stdio.h>
#include <stdlib.h>

struct pool {
        size_t numberOfThreads;
        FILE* firstWork;
        FILE* lastWork;
};

typedef struct pool threadPool;

threadPool* createPool(size_t numberOfThreads);
void destroyPool(threadPool* threadManager);
void addConnection();
