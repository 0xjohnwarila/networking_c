#include "decrypt_pool.h"

#define MAX_PACKET_SIZE 10000

void error(const char *msg) {
    perror(msg);
} // Error function used for reporting issues

int modulo(int a, int b) {
  const int result = a % b;
  return result >= 0 ? result : result + b;
}

void decrypt(char* ciphertext, char* key, char* plain) {
    //int* numericalText = malloc(strlen(ciphertext));
    int numericalText[512];
    //int* numericalKey = malloc(strlen(key)*sizeof(int));
    int numericalKey[80000];
    //int* numericalCipher = malloc(strlen(ciphertext));
    int numericalCipher[512];

    for (int i = 0; i < strlen(ciphertext); i++) {
        if (ciphertext[i] == ' ') {
            numericalText[i] = 27;
        }
        else {
            numericalText[i] = ciphertext[i] - 64;
        }
    }
    for (int i = 0; i < strlen(key); i++) {
        if (key[i] == ' ') {
            numericalKey[i] = 27;
        }
        else {
            numericalKey[i] = key[i] - 64;
        }
    }
    for (int i = 0; i < strlen(ciphertext); i++) {
        numericalCipher[i] = modulo((numericalText[i] - numericalKey[i]), 27) - 1;
    }
    for (int i = 0; i < strlen(ciphertext); i++) {
        if (numericalCipher[i] == 0) {
            plain[i] = ' ';
        }
        else {
            char tmp = numericalCipher[i];
            plain[i] = tmp + '@';
        }
    }
}

void doWork(int connection) {
    int charsRead;
    char buffer[MAX_PACKET_SIZE];

    memset(buffer, '\0', MAX_PACKET_SIZE);
    charsRead = recv(connection, buffer, MAX_PACKET_SIZE, 0);
    if (charsRead < 0) {
        error("ERROR reading from socket");
    }
    // We are connected to the wrong client
    if (strcmp(buffer, "otp_dec") != 0) {
        charsRead = send(connection, "error", 5, 0);
        close(connection);
        return;
    }
    charsRead = send(connection, "otp_dec", 7, 0);
    if (charsRead < 0) {
        error("ERROR writing to socket");
    }

    // Accept incomming plain text
    memset(buffer, '\0', MAX_PACKET_SIZE);
    charsRead = recv(connection, buffer, MAX_PACKET_SIZE, 0);
    if (charsRead < 0) {
        error("ERROR reading from socket");
    }
    

    char* ciphertext = (char*)malloc(sizeof(char)*strlen(buffer));
    memcpy(ciphertext, buffer, strlen(buffer));

    // Accept incomming number of packets
    memset(buffer, '\0', MAX_PACKET_SIZE);
    charsRead = recv(connection, buffer, MAX_PACKET_SIZE, 0);
    if (charsRead < 0) {
        error("ERROR reading from socket");
    }

    charsRead = send(connection, buffer, sizeof(buffer), 0);
    if (charsRead < 0) {
        error("ERROR writing to socket");
    }



    int numberOfPackets = atoi(buffer);
    //char key[100000];
    
    char* key = malloc((MAX_PACKET_SIZE* (numberOfPackets+5)) + 1);

    char packets[10][MAX_PACKET_SIZE];
    // Accept incomming key
    for (int i = 0; i < numberOfPackets; i++) {
        memset(buffer, '\0', MAX_PACKET_SIZE);
        charsRead = recv(connection, buffer, MAX_PACKET_SIZE, 0);
        if (charsRead < 0) {
            error("ERROR reading from socket");
        }
        strcpy(packets[i], buffer);
    }

    sprintf(key, "%s%s%s%s%s%s%s%s%s%s", packets[0], packets[1], packets[2], packets[3], packets[4], packets[5], packets[6], packets[7], packets[8], packets[9]);
    // Decrypt and send to client
    char plain[512];
    decrypt(ciphertext, key, plain);
    plain[sizeof(char)*strlen(ciphertext) + 1] = '\0';
    // send to client
    memset(buffer, '\0', MAX_PACKET_SIZE);
    memcpy(buffer, plain, strlen(plain));

    charsRead = send(connection, buffer, strlen(buffer), 0);
    if (charsRead < 0) {
        error("ERROR writing to socket");
    }
    free(key);
    free(ciphertext);
    close(connection);
}


/* BELOW IS THREADING STUFF */


// Get some work from the manager
work_t* getWork(threadPool* threadManager) {
    work_t* work;
    // Check for null manager
    if (threadManager == NULL) 
        return NULL;
    
    // Get first work and check null
    work = threadManager->firstWork;
    if (work == NULL)
        return NULL;

    // Update the linked list
    if (work->next == NULL) {
        threadManager->firstWork = NULL;
        threadManager->lastWork = NULL;
    } else {
        threadManager->firstWork = work->next;
    }

    return work;
}

// Destroy the work
void destroyWork(work_t* work) {
    if (work == NULL) {
        return;
    }
    free(work);
}

// Worker locks the mutex and tries to get work
void* worker(void* arg) {
    threadPool* threadManager = arg;
    work_t* work;
    while (1) {
        // lock work mutex
        pthread_mutex_lock(&(threadManager->workMutex));

        // If no work, wait for work
        if (threadManager->firstWork == NULL)
            pthread_cond_wait(&(threadManager->workCond), &(threadManager->workMutex));
        
        // Get some work
        work = getWork(threadManager);
        // Unlock
        pthread_mutex_unlock(&(threadManager->workMutex));

        if (work != NULL) {
            // Do the work
            doWork(work->connection);
            destroyWork(work);
        }
    }
}

// Constructor for the pool
threadPool* createPool(size_t numberOfThreads) {
    threadPool* threadManager;
    pthread_t thread;

    // default threads
    if (numberOfThreads == 0) {
        numberOfThreads = 4;
    }

    // allocate manager
    threadManager = calloc(1, sizeof(*threadManager));
    threadManager->numberOfThreads = numberOfThreads;

    // init mutex and cond
    pthread_mutex_init(&(threadManager->workMutex), NULL);
    pthread_cond_init(&(threadManager->workCond), NULL);

    // init linked list
    threadManager->firstWork = NULL;
    threadManager->lastWork = NULL;

    // make threads
    for (int i = 0; i < numberOfThreads; i++) {
        pthread_create(&thread, NULL, worker, threadManager);
        pthread_detach(thread);
    }
    return threadManager;
}

// Destroy pool
void destroyPool(threadPool* threadManager) {
    if (threadManager == NULL){
        return;
    }
    free(threadManager);
}

// work constructor
work_t* createWork(int connection) {
    work_t* work;

    work = malloc(sizeof(*work));
    work->connection = connection;
    work->next = NULL;
    return work;
}

// add work to linked list
void addWork(int newWork, threadPool* threadManager) {
    work_t* work;

    if (threadManager == NULL) {
        return;
    }

    // create a new work
    work = createWork(newWork);
    if (work == NULL) {
        return;
    }

    // lock and add to list
    pthread_mutex_lock(&(threadManager->workMutex));
    if (threadManager->firstWork == NULL) {
        threadManager->firstWork = work;
        threadManager->lastWork = work;
    } else {
        threadManager->lastWork->next = work;
        threadManager->lastWork = work;
    }
    // let others know there is work
    pthread_cond_broadcast(&(threadManager->workCond));
    pthread_mutex_unlock(&(threadManager->workMutex));
}