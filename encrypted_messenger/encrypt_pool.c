#include "encrypt_pool.h"

#define MAX_PACKET_SIZE 10000

void error(const char *msg) {
    perror(msg);
} // Error function used for reporting issues

void encrypt(char* plaintext, char* key, char* cipher) {
    //int* numericalText = malloc(strlen(plaintext));
    int numericalText[512];
    //int* numericalKey = malloc(strlen(key)*sizeof(int));
    int numericalKey[80000];
    //int* numericalCipher = malloc(strlen(plaintext));
    int numericalCipher[512];

    // Conver the plain text to numbers
    for (int i = 0; i < strlen(plaintext); i++) {
        if (plaintext[i] == ' ') {
            numericalText[i] = 27;
        }
        else {
            numericalText[i] = plaintext[i] - 64;
        }
    }

    // Convert the key to numbers
    for (int i = 0; i < strlen(key); i++) {
        if (key[i] == ' ') {
            numericalKey[i] = 27;
        }
        else {
            numericalKey[i] = key[i] - 64;
        }
    }
    // encrypt
    for (int i = 0; i < strlen(plaintext); i++) {
        numericalCipher[i] = (numericalText[i] + numericalKey[i]) % 27;
    }

    // reconvert to text
    for (int i = 0; i < strlen(plaintext); i++) {
        if (numericalCipher[i] == 26) {
            cipher[i] = ' ';
        }
        else if (numericalCipher[i] > 26) {
            numericalCipher[i] = numericalCipher[i] - 26;
        }
        else {
            char tmp = numericalCipher[i];
            cipher[i] = tmp + 'A';
        }
    }
}

// do work function, handles connections and encryptions
void doWork(int connection) {
    int charsRead;
    char buffer[MAX_PACKET_SIZE];

    // Read in the opener to make sure that we are connected to the correct client
    memset(buffer, '\0', MAX_PACKET_SIZE);
    charsRead = recv(connection, buffer, MAX_PACKET_SIZE, 0);
    if (charsRead < 0) {
        error("ERROR reading from socket");
    }
    // We are connected to the wrong client
    if (strcmp(buffer, "otp_enc") != 0) {
        charsRead = send(connection, "error", 5, 0);
        close(connection);
        return;
    }
    charsRead = send(connection, "otp_enc", 7, 0);
    if (charsRead < 0) {
        error("ERROR writing to socket");
    }

    // Accept incomming plain text
    memset(buffer, '\0', MAX_PACKET_SIZE);
    charsRead = recv(connection, buffer, MAX_PACKET_SIZE, 0);
    if (charsRead < 0) {
        error("ERROR reading from socket");
    }
    
    // save the plain text
    char* plaintext = (char*)malloc(sizeof(char)*strlen(buffer));
    memcpy(plaintext, buffer, strlen(buffer));

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
        //strcat(key, buffer);
        //sprintf(key, "%s%s", key, buffer);
    }

    // This is a hack to avoid a strange malloc issue
    sprintf(key, "%s%s%s%s%s%s%s%s%s%s", packets[0], packets[1], packets[2], packets[3], packets[4], packets[5], packets[6], packets[7], packets[8], packets[9]);
    

    // Encrypt and send to client
    char cipher[512];
    encrypt(plaintext, key, cipher);
    cipher[sizeof(char)*strlen(plaintext) + 1] = '\0';
    // send to client
    memset(buffer, '\0', MAX_PACKET_SIZE);
    memcpy(buffer, cipher, strlen(cipher));

    // send to client
    charsRead = send(connection, buffer, strlen(buffer), 0);
    if (charsRead < 0) {
        error("ERROR writing to socket");
    }

    // Clean up
    free(key);
    free(plaintext);
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