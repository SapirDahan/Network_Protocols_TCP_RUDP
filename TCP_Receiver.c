#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/tcp.h>

#define BUFFER_SIZE 2048
#define EXIT_MESSAGE "exit"

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Usage: ./TCP_Receiver -p PORT -algo ALGO\n");
        exit(EXIT_FAILURE);
    }

    int port;
    char *algo = NULL;

    // Parse command line arguments
    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-p") == 0) {
            port = atoi(argv[i + 1]);
        } else if (strcmp(argv[i], "-algo") == 0) {
            algo = argv[i + 1];
        }
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in my_addr;
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    listen(sockfd, 10);

    int new_sockfd = accept(sockfd, NULL, NULL);
    if (new_sockfd < 0) {
        perror("Accept failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Set congestion control algorithm
    if (setsockopt(new_sockfd, IPPROTO_TCP, TCP_CONGESTION, algo, strlen(algo)) < 0) {
        perror("Error setting congestion control algorithm");
        close(new_sockfd);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    ssize_t total_received = 0;
    int run_number = 1;

    // Receive data until the sender closes the connection
    while (1) {
        bytes_received = recv(new_sockfd, buffer, BUFFER_SIZE, 0);

        // Print run statists after a complete file received
        if (bytes_received == 0 && total_received > 0) {
            printf("Received Bytes: %zd\n", total_received);
            printf("-Run #%d Data:", run_number);
            run_number += 1;
            total_received = 0;
        }

        // Check for the exit message
        if (strncmp(buffer, EXIT_MESSAGE, strlen(EXIT_MESSAGE)) == 0) {
            printf("Exit message received. Closing connection.\n");
            break; // Exit the loop to close the connection
        }

        total_received += bytes_received;
    }

    if (bytes_received < 0) {
        perror("Receive failed");
    }

    else {
        printf("Total bytes received: %zd\n", total_received);
    }

    close(new_sockfd);
    close(sockfd);
    return 0;
}
