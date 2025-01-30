/*
 * Auth: Jake Henderson
 * Date: 12-01-24 (Due: 12-08-24)
 * Course: CSCI-3550 (Sec: 850)
 * Desc: Project 1, Client
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define BUF_SIZE (10*1024*1024) /* 10MiB */

/* Function Prototypes */
void cleanup(char *inbuf, char *outbuf, int sockfd);

/* Buffer pointers */
char *inbuf = NULL;  /* For storing incoming data from 'recv()' */
char *outbuf = NULL; /* For storing outgoing data using 'send()' */

int sockfd = -1; /* Socket [file] descriptor */

/********************************CLEANUP**********************************/

void cleanup(char *inbuf, char *outbuf, int sockfd) {
    if (inbuf != NULL) {
        free(inbuf);
        inbuf = NULL;
    }
    if (outbuf != NULL) {
        free(outbuf);
        outbuf = NULL;
    }
    if (sockfd > -1) {
        close(sockfd);
        sockfd = -1;
    }
}

int main(int argc, char *argv[]) {
    struct sockaddr_in server_addr; /* Move declarations to the top of the block */
    int bytes_sent;
    int i; /* Declare loop variable at the top for C90 compliance */
    int port = atoi(argv[2]);

    if(argc < 4) {
        fprintf(stderr, "Usage: %s <server_ip> <port> <file1> [file2] ...\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("client: Starting client...\n");

    /* Allocate memory for buffers */
    inbuf = (char *)malloc(BUF_SIZE);
    outbuf = (char *)malloc(BUF_SIZE);

    if (!inbuf || !outbuf) {
        fprintf(stderr, "client: ERROR: Failed to allocate memory.\n\n");
        cleanup(inbuf, outbuf, -1);
        exit(EXIT_FAILURE);
    }
    printf("client: Memory allocation successful.\n");

    if (port <= 0) {
        fprintf(stderr, "client: ERROR: Invalid port number.\n\n");
        cleanup(inbuf, outbuf, -1);
        exit(EXIT_FAILURE);
    }

    /* Iterate over files in the argument list starting from argv[2] */
    for (i = 3; i < argc; i++) {
        FILE *file;
        int file_size;

        printf("client: Preparing to open file: \"%s\"\n", argv[i]);
        file = fopen(argv[i], "rb");
        if (!file) {
            fprintf(stderr, "client: ERROR: Failed to open: \"%s\"\n\n", argv[i]);
            continue;
        }

        /* Read file content */
        file_size = fread(inbuf, 1, BUF_SIZE, file);
        fclose(file);
        if (file_size <= 0) {
            fprintf(stderr, "client: ERROR: Unable to read: \"%s\"\n\n", argv[i]);
            continue;
        }

        printf("client: File \"%s\" read successfully. Size: %d bytes.\n", argv[i], file_size);

        /* Prepare socket connection */
        printf("client: Address prepared. Connecting to %s:%s...\n", argv[1], argv[2]);
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            fprintf(stderr, "client: ERROR: Failed to create socket.\n\n");
            cleanup(inbuf, outbuf, sockfd);
            exit(EXIT_FAILURE);
        }

        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons((unsigned short int)atoi(argv[2]));
        if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
            fprintf(stderr, "client: ERROR: Invalid address/Address not supported.\n\n");
            cleanup(inbuf, outbuf, sockfd);
            exit(EXIT_FAILURE);
        }

        if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            fprintf(stderr, "client: ERROR: Failed to connect to the server.\n\n");
            close(sockfd);
            continue;
        }

        printf("client: Connected to the server successfully.\n");

        /* Send file content */
        printf("client: Sending file content: \"%s\"...\n", argv[i]);
        bytes_sent = send(sockfd, inbuf, file_size, 0);
        if (bytes_sent < file_size) {
            fprintf(stderr, "client: ERROR: While sending data.\n\n");
        }

        close(sockfd);
    }

    printf("client: File transfer(s) complete.\n");
    printf("client: Goodbye!\n");

    cleanup(inbuf, outbuf, -1);
    return 0;
}



