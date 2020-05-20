#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdbool.h>

#define MAX_PACKET_SIZE 10000

void error(const char *msg) { perror(msg); exit(0); } // Error function used for reporting issues

bool checkConnection(int socketFD) {
    char buffer[512];
    int charsWritten, charsRead;

    memset(buffer, '\0', sizeof(buffer));
    // Set the buffer to show that we are otp_enc
    strcpy(buffer, "otp_dec");

    charsWritten = send(socketFD, buffer, strlen(buffer), 0);
    if (charsWritten < 0) {
        error("ERROR writing to socket");
    }
    if (charsWritten < strlen(buffer)) {
        printf("Not all data written to socket\n");
    }

    // Clear out the buffer again for reuse
	memset(buffer, '\0', sizeof(buffer));
	// Read data from the socket, leaving \0 at end
	charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0);
	if (charsRead < 0) 
		error("CLIENT: ERROR reading from socket");
    if (strcmp(buffer, "otp_dec") != 0) {
        // We are not connected to the correct daemon
        return false;
    }

    // We are connected to the correct daemon
    return true;
}

void sendPlainText(char* fileName, int socketFD) {
    char buffer[512];
    int charsWritten;

    memset(buffer, '\0', sizeof(buffer));

    FILE* fp;

    if ((fp = fopen(fileName, "r")) == NULL) {
        error("Could not open cipher text file");
    }

    fgets(buffer, sizeof(buffer) - 1, fp);
    buffer[strcspn(buffer, "\n")] = '\0';


    charsWritten = send(socketFD, buffer, strlen(buffer), 0);
	if (charsWritten < 0) 
		error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(buffer)) 
		printf("CLIENT: WARNING: Not all data written to socket!\n");
    fclose(fp);
}

void sendKey(char* fileName, int socketFD) {
    int charsWritten, charsRead;
    char buffer[MAX_PACKET_SIZE];

    FILE* fp;
    int fileSize;

    if ((fp = fopen(fileName, "r")) == NULL) {
        error("Could not open key file");
    }

    fseek(fp, 0L, SEEK_END);
    fileSize = ftell(fp) + 1;
    rewind(fp);

    char* key = malloc(sizeof(char)*fileSize);
    fgets(key, fileSize, fp);
    key[strcspn(key, "\n")] = '\0';

    
    int numberOfPackets = fileSize / MAX_PACKET_SIZE;
    numberOfPackets += 1;

    memset(buffer, '\0', sizeof(buffer));
    // Set the buffer to send the number of packets
    sprintf(buffer, "%d", numberOfPackets);

    charsWritten = send(socketFD, buffer, strlen(buffer), 0);
	if (charsWritten < 0) 
		error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(buffer)) 
		printf("CLIENT: WARNING: Not all data written to socket!\n");

    // Clear out the buffer again for reuse
	memset(buffer, '\0', sizeof(buffer));
	// Read data from the socket, leaving \0 at end
	charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0);
	if (charsRead < 0) 
		error("CLIENT: ERROR reading from socket");

    for (int i = 0; i < numberOfPackets; i++) {
        memset(buffer, '\0', sizeof(buffer));
        memcpy(buffer, &key[i*(MAX_PACKET_SIZE-1)], MAX_PACKET_SIZE-1);
        buffer[MAX_PACKET_SIZE-1] = '\0';

        charsWritten = send(socketFD, buffer, MAX_PACKET_SIZE, 0);
        if (charsWritten < 0) 
            error("CLIENT: ERROR writing to socket");
        if (charsWritten < strlen(buffer)) 
            printf("CLIENT: WARNING: Not all data written to socket!\n");
    }
    fclose(fp);

    memset(buffer, '\0', sizeof(buffer));
	// Read data from the socket, leaving \0 at end
	charsRead = recv(socketFD, buffer, MAX_PACKET_SIZE - 1, MSG_WAITALL);
	if (charsRead < 0) 
		error("CLIENT: ERROR reading from socket");
    
    printf("%s\n", buffer+1);
}

int main(int argc, char *argv[]) {
    int socketFD, portNumber;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
    int plaintextsize = 0;
    
    // Check usage & args
	if (argc < 4) {
        fprintf(stderr,"USAGE: %s ciphertext key port\n", argv[0]);
        exit(0);
    }

    // Check that the key is not too short
    FILE* fp;
    int fileSize;

    if ((fp = fopen(argv[1], "r")) == NULL) {
        error("Could not open key file");
    }

    fseek(fp, 0L, SEEK_END);
    fileSize = ftell(fp) + 1;
    rewind(fp);

    plaintextsize = fileSize;
    fclose(fp);

    if ((fp = fopen(argv[2], "r")) == NULL) {
        error("Could not open key file");
    }

    fseek(fp, 0L, SEEK_END);
    fileSize = ftell(fp) + 1;
    rewind(fp);

    if (fileSize < plaintextsize) {
        error("key is too short");
    }

	// Set up the server address struct

	// Clear out the address struct
	memset((char*)&serverAddress, '\0', sizeof(serverAddress));
	// Get the port number, convert to an integer from a string
	portNumber = atoi(argv[3]);
	// Create a network-capable socket
	serverAddress.sin_family = AF_INET;
	// Store the port number
	serverAddress.sin_port = htons(portNumber);
	// Convert the machine name into a special form of address
	serverHostInfo = gethostbyname("localhost");
	if (serverHostInfo == NULL) {
		fprintf(stderr, "CLIENT: ERROR, no such host\n"); 
		exit(0);
	}

	// Copy in the address
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length);

	// Set up the socket

	// Create the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0);
	if (socketFD < 0) 
		error("CLIENT: ERROR opening socket");
	
	// Connect to server

	// Connect socket to address
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
		error("CLIENT: ERROR connecting");

    // Make sure we are talking to the right daemon
    if (!checkConnection(socketFD)) {
        error("Connection could not be verified");
    }

    // Send the plain text
    sendPlainText(argv[1], socketFD);

    // Send the key and recv cipher
    sendKey(argv[2], socketFD);

    /*
	// Get input message from user
	printf("CLIENT: Enter text to send to the server, and then hit enter: ");
	// Clear out the buffer array
	memset(buffer, '\0', sizeof(buffer));
	// Get input from the user, trunc to buffer - 1 chars, leaving \0
	fgets(buffer, sizeof(buffer) - 1, stdin);
	// Remove the trailing \n that fgets adds
	buffer[strcspn(buffer, "\n")] = '\0';

	// Send message to server

	// Write to the server
	charsWritten = send(socketFD, buffer, strlen(buffer), 0);
	if (charsWritten < 0) 
		error("CLIENT: ERROR writing to socket");
	if (charsWritten < strlen(buffer)) 
		printf("CLIENT: WARNING: Not all data written to socket!\n");

	// Get return message from server

	
    */
	// Close the socket
	close(socketFD);
	return 0;
}
