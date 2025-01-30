/*
* Auth: Jake Henderson
* Date: 12-01-24 (Due: 12-08-24)
* Course: CSCI-3550 (Sec: 850)
* Desc: Project 1, Server
*/

#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h> /* For file operations */

#define BUF_SIZE (10 * 1024 * 1024) /* 10MiB */

/* Function Prototypes */
void SIGINT_handler(int sig);
void cleanup(void);

/* Global Variables */
int listen_sockfd = -1;    /* Socket [file] descriptor */
char *inbuf = NULL;        /* For storing incoming data from 'recv()' */

/********************************SIGINT**********************************/
/* SIGINT handler for the server */
void SIGINT_handler(int sig) {
    fprintf(stderr, "server: Server interrupted. Shutting down.\n");
    cleanup();
    exit(EXIT_FAILURE);
}

/********************************CLEANUP**********************************/
void cleanup(void) {
    if (inbuf != NULL) {
        free(inbuf);
        inbuf = NULL;
    }
    if (listen_sockfd > -1) {
        close(listen_sockfd);
        listen_sockfd = -1;
    }
    printf("server: Cleanup complete.\n");
}

/********************************MAIN***********************************/
int main(int argc, char *argv[]) {
    struct sockaddr_in sa, cl_sa; /* Server and client socket addresses */
    socklen_t cl_sa_size = sizeof(cl_sa);
    int cl_sockfd;               /* Socket descriptor for accepted client */
    int bytes_read;              /* Track bytes read */
    int file_fd;                 /* File descriptor for saving received data */
    unsigned short int port;
    char filename[64];           /* For storing the generated filename */
    static int file_count = 1;   /* Track the number of files received */

    /* Ensure the correct number of arguments */
    if (argc != 2) {
        fprintf(stderr, "server: ERROR: Usage: %s <port>\n\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Convert port number */
    port = (unsigned short int)atoi(argv[1]);

    /* Check for privileged ports */
    if (port <= 1023) {
        fprintf(stderr, "server: ERROR: Port number is privileged.\n\n");
        exit(EXIT_FAILURE);
    }

    /* Set up SIGINT handler */
    signal(SIGINT, SIGINT_handler);

    /* Allocate memory for the buffer */
    inbuf = (char *)malloc(BUF_SIZE);
    if (inbuf == NULL) {
        fprintf(stderr, "server: ERROR: Failed to allocate memory.\n\n");
        exit(EXIT_FAILURE);
    }

    /* Create socket */
    listen_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sockfd < 0) {
        fprintf(stderr, "server: ERROR: Failed to create socket.\n\n");
        cleanup();
        exit(EXIT_FAILURE);
    }

    /* Prepare socket address */
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port = htons(port);

    /* Bind socket */
    if (bind(listen_sockfd, (struct sockaddr *)&sa, sizeof(sa)) != 0) {
        fprintf(stderr, "server: ERROR: Failed to bind socket.\n\n");
        cleanup();
        exit(EXIT_FAILURE);
    }

    /* Listen on socket */
    if (listen(listen_sockfd, 32) != 0) {
        fprintf(stderr, "server: ERROR: listen(): Failed.\n\n");
        cleanup();
        exit(EXIT_FAILURE);
    }

    printf("Server listening on 127.0.0.1:%d...\n", port);

    /* Accept connections in a loop */
    while (1) {
        cl_sockfd = accept(listen_sockfd, (struct sockaddr *)&cl_sa, &cl_sa_size);
        if (cl_sockfd < 0) {
            fprintf(stderr, "server: ERROR: While attempting to accept a connection.\n\n");
            continue;
        }

        printf("server: Connection accepted!\n");
        printf("server: Receiving file...\n");

        /* Receive data from client */
        bytes_read = recv(cl_sockfd, (void *)inbuf, BUF_SIZE, 0);
        if (bytes_read > 0) {
            /* Generate a filename for saving */
            snprintf(filename, sizeof(filename), "file-%02d.dat", file_count++);

            /* Open file for writing */
            file_fd = open(filename, O_CREAT | O_WRONLY, 0644);
            if (file_fd < 0) {
                fprintf(stderr, "server: ERROR: Unable to create: %s.\n\n", filename);
                close(cl_sockfd);
                continue;
            }

            /* Write data to the file */
            if (write(file_fd, inbuf, bytes_read) != bytes_read) {
                fprintf(stderr, "server: ERROR: Unable to write: %s.\n\n", filename);
                close(file_fd);
                close(cl_sockfd);
                continue;
            }

            printf("server: Saving file: \"%s\" ...\n", filename);
            printf("server: Finished. Closing connection.\n\n");

            close(file_fd);
        } else if (bytes_read == 0) {
            printf("server: Client disconnected.\n");
        } else {
            fprintf(stderr, "server: ERROR: Reading from socket.\n\n");
        }

        close(cl_sockfd);
    }

    cleanup();
    return 0;
}




