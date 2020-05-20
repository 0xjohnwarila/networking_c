#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#include "decrypt_pool.h"

#define NUMBER_OF_THREADS 5

int PORT;

/*
Function: startup
Input: int argc, char* argv
Output: threadPool_type*

Sets the globals and does input error checking.
Creates the thread manager and returns it.
*/
threadPool* startup(int argc, char** argv) {
    // Check if the correct number of args are given
    if (argc != 2) {
        error("Usage: opt_enc_d [PORT]");
        exit(1);
    }
    // Convert argv[1] to a number
    char* endptr;
    PORT = strtol(argv[1], &endptr, 10);

    // Check for a string input
    if (*endptr != '\0') {
        error("ERROR port must be a number");
        exit(1);
    }

    threadPool* threadManager;

    threadManager = createPool(NUMBER_OF_THREADS);

    return threadManager;
}

/*
Function: serve
Input: int portNumber
Output: void

Listens on the port and adds work to the thread pool.
*/
void serve(int portNumber, threadPool* threadManager) {
    int listenSocketFD, establishedConnectionFD;
    socklen_t sizeOfClientInfo;
    struct sockaddr_in serverAddress, clientAddress;

    // Clear out address struct
    memset((char *)&serverAddress, '\0', sizeof(serverAddress));

    // Create network-capable socket
    serverAddress.sin_family = AF_INET;
    // Store port number
    serverAddress.sin_port = htons(portNumber);
    // Allow any address
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Create the socket
    listenSocketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocketFD < 0) {
        error("ERROR opening socket");
        exit(1);
    }

    // Connect socket to port
    if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        error("ERROR on binding");
        exit(1);
    }

    // Listen on the socket, accepting up to 10 connections
    listen(listenSocketFD, 10);

    sizeOfClientInfo = sizeof(clientAddress);

    while (1) {
        establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
        if (establishedConnectionFD < 0) {
            error("ERROR on accept");
        }
        // Add work to the list
        addWork(establishedConnectionFD, threadManager);
    }
    
    close(listenSocketFD);
    destroyPool(threadManager);
}

int main(int argc, char** argv) {
    // Set globals and check for errors
    threadPool* threadManager = startup(argc, argv);
    // Listen on the port, and add work to the threadmanager
    serve(PORT, threadManager);
    return 0;
}