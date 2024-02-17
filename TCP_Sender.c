#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <time.h>


#define BUFFER_SIZE 2048 // Use a buffer large enough to send data efficiently
#define SIZE_OF_FILE 2097152 // Size of the file (2MB)

// Function to generate a random alphanumeric character
char random_alphanumeric() {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    const size_t charset_size = sizeof(charset) - 1;
    return charset[rand() % charset_size];
}

int main(int argc, char *argv[]) {

    FILE *file = fopen("random_generated_file.txt", "w");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    // Seed the random number generator
    srand((unsigned int)time(NULL));

    // Generate random characters and write them to the file
    for (int i = 0; i < SIZE_OF_FILE; i++) {
        fputc(random_alphanumeric(), file);
    }

    if (argc != 7) {
        fprintf(stderr, "Usage: ./TCP_Sender -ip IP -p PORT -algo ALGO\n");
        exit(EXIT_FAILURE);
    }

    char *ip = NULL;
    int port;
    char *algo = NULL;

    // Parse command line arguments
    for (int i = 1; i < argc; i += 2) {
        if (strcmp(argv[i], "-ip") == 0) {
            ip = argv[i + 1];
        } else if (strcmp(argv[i], "-p") == 0) {
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

    // Set congestion control algorithm
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_CONGESTION, algo, strlen(algo)) < 0) {
        perror("Error setting congestion control algorithm");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &dest_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address/ Address not supported \n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        perror("Connection Failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Send a large buffer or a file
    char buffer[BUFFER_SIZE];
    memset(buffer, 'A', BUFFER_SIZE); // Dummy data for example purposes

    // Send data in chunks until you reach the desired file size
    ssize_t total_sent = 0;
    ssize_t bytes_sent;
    size_t file_size = 2 * 1024 * 1024; // 2MB file size as required

    while (total_sent < file_size) {
        bytes_sent = send(sockfd, buffer, BUFFER_SIZE, 0);
        if (bytes_sent < 0) {
            perror("Error sending data");
            break;
        }
        total_sent += bytes_sent;
    }

    printf("Total bytes sent: %zd\n", total_sent);

    close(sockfd);
    return 0;
}
