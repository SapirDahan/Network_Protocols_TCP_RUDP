#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <time.h>
#include <errno.h>


#define BUFFER_SIZE 2048 // Use a buffer large enough to send data efficiently
// #define SIZE_OF_FILE 2097153 // Size of the file (2MB)
#define SIZE_OF_FILE 32769 // Size of the file (32K)
#define EXIT_MESSAGE "<exit>"

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
    for (int i = 0; i < SIZE_OF_FILE - 2; i++) {
        fputc(random_alphanumeric(), file);
    }
    fputc('!', file);
    fputc('_', file);

    if (argc != 7) {
        fprintf(stderr, "Usage: ./TCP_Sender -ip IP -p PORT -algo ALGO\n");
        exit(EXIT_FAILURE);
    }

    char *ip = NULL;
    int port;
    char *algo = NULL;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
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

    // Open file for reading
    FILE *file_to_read = fopen("random_generated_file.txt", "rb");
    if (file_to_read == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    // Send data in chunks until you reach the desired file size
    ssize_t total_sent;
    ssize_t bytes_sent;
    ssize_t bytes_read;
    char send_again;

    do {

        total_sent = 0;

        while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file_to_read)) > 0) {
            bytes_sent = send(sockfd, buffer, bytes_read, 0);
            total_sent += bytes_sent;
            if (bytes_sent < 0) {
                perror("Error sending data");
                break;
            }
        }

        //rewind(file_to_read);
        fseek(file_to_read, 0, SEEK_SET);
        printf("Total bytes sent: %zd\n", total_sent);
        printf("Send again? (y/n): ");
        scanf("%s", &send_again);

    } while(send_again == 'y' || send_again == 'Y');

    // Send the exit message to the server
    if (send(sockfd, EXIT_MESSAGE, strlen(EXIT_MESSAGE), 0) < 0) {
        perror("Could not send a exit message");
    }

    close(sockfd);
    return 0;
}
