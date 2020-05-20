#include "thread_pool.h"

threadPool* createPool(size_t numberOfThreads) {
    printf("%ld\n", numberOfThreads);
    return NULL;
}

void destroyPool(threadPool* threadManager) {
    if (threadManager == NULL){
        return;
    }
    free(threadManager);
}