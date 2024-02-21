#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>


#define BUFFER_SIZE 2048 // Use a buffer large enough to send data efficiently
#define SIZE_OF_FILE 2097153 // Size of the file (2MB)
#define EXIT_MESSAGE "<exit>" // Exit massage

// Function to generate a random alphanumeric character only with letters and numbers
char random_alphanumeric() {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    const size_t charset_size = sizeof(charset) - 1;
    return charset[rand() % charset_size];
}

int main(int argc, char *argv[]) {

    //Create a file
    FILE *file = fopen("random_generated_file.txt", "w");
    if (file == NULL) {
        perror("Error opening file\n");
        exit(EXIT_FAILURE);;
    }

    // Seed the random number generator
    srand((unsigned int)time(NULL));

    // Generate random characters and write them to the file
    for (int i = 0; i < SIZE_OF_FILE - 2; i++) {
        fputc(random_alphanumeric(), file);
    }

    //the end of file
    fputc('!', file);
    fputc(' ', file);

    //We are supposed to get 5 arguments from the user
    if (argc != 5) {
        fprintf(stderr, "Supposed to get an argument this style: ./RUDP_Sender -ip IP -p PORT\n");
        exit(EXIT_FAILURE);
    }

    char *ip = NULL; //IP
    int port; // The port

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-ip") == 0) {
            ip = argv[i + 1];
        }

        else if (strcmp(argv[i], "-p") == 0) {
            port = atoi(argv[i + 1]);
        }
    }

    // Create socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) {
        perror("Error creating socket\n");
        exit(EXIT_FAILURE);
    }

//    // Set congestion control algorithm
//    if (setsockopt(sockfd, IPPROTO_TCP, TCP_CONGESTION, algo, strlen(algo)) < 0) {
//        perror("Error setting congestion control algorithm\n");
//        close(sockfd);
//        exit(EXIT_FAILURE);
//    }

    struct sockaddr_in dest_addr; // The address of the receiver
//    dest_addr.sin_family = AF_INET;
//    dest_addr.sin_port = htons(port);
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;// Using IPv4
    dest_addr.sin_port = htons(port);
    //inet_pton(AF_INET, ip, &dest_addr.sin_addr);

    if (inet_pton(AF_INET, ip, &dest_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address/ Address not supported \n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Set a large buffer or a file
    char buffer[BUFFER_SIZE];
    memset(buffer, 'A', BUFFER_SIZE);

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

    // Send the file until the user ask to stop
    do {

        total_sent = 0;

        //Read the file
        while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file_to_read)) > 0) {

            //Send the data
            bytes_sent = sendto(sockfd, buffer, bytes_read, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
            total_sent += bytes_sent;
            if (bytes_sent < 0) {
                perror("Error sending data\n");
                break;
            }
        }


        fseek(file_to_read, 0, SEEK_SET);

        //The byte sent
        printf("Total bytes sent: %zd\n", total_sent);

        //Asking the user if he wants to send again the data
        printf("Send again? (y/n): ");
        scanf("%s", &send_again);

    } while(send_again == 'y' || send_again == 'Y');

    // Send the exit message to the server
    if (sendto(sockfd, EXIT_MESSAGE, strlen(EXIT_MESSAGE), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        perror("Could not send a exit message\n");
        return EXIT_FAILURE;
    }

    else{
        printf("Sent exit massage\n");
    }

    //Close the socket
    close(sockfd);

    printf("Sender end\n");

    return 0;
}
