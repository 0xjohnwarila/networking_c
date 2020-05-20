/*
This program takes one argument, the length the keygen should be. Then
generates a random string of chars that is that length. Then outputs the
string to stdout with a newline at the end.
*/
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void keyGen(int length, char* key) {
    char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
    while (length-- > 0) {
        // Get a random number that will be our index. (0-26)
        int index = (double) rand() / RAND_MAX * (sizeof charset - 1);
        // Set the char to the charset, and increment the pointer to point to the next char
        *key++ = charset[index];
    }
    // Set the final char to a newline
    *key = '\n';
}

int main(int argc, char* argv[]) {
    // Error handling for incorrect number of args
    if (argc != 2) {
        printf("%s\n", "Usage: keygen [length]");
        exit(1);
    }
    // Init time to NULL
    srand(time(NULL));
    char* key;
    int length = 0;
    // Get the length from the command line args
    length = atoi(argv[1]);

    // Error handling for length of 0 or string put in argv[1]
    if (length == 0) {
        printf("%s\n", "Usage: keygen [length]");
        exit(1);
    }
    
    // Get the memory we need
    key = (char*)malloc(sizeof(char) * length);
    // Set to all '\0'
    memset(key, '\0', length);
    // Generate the random key
    keyGen(length, key);
    // Print the key
    printf("%s", key);
    return 0;
}