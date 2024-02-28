#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

int rudp_socket(int domain, int type, int protocol);
ssize_t rudp_send(int sockfd, char* buffer, ssize_t bytes_read, int flag, const struct sockaddr *addr, socklen_t addr_len);
int rudp_recv(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
int hand_shake_send(char * buffer, int sockfd, const struct sockaddr_in recv_addr, int BUFFER_SIZE);
void rudp_close(int sockfd);
int ack_recv(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);

#define BUFFER_SIZE 2048 // Use a buffer large enough to send data efficiently
#define SIZE_OF_FILE 2097152 // Size of the file (2MB)
//#define SIZE_OF_FILE 8192 // experimental size
#define EXIT_MESSAGE "<exit>" // Exit massage
#define PACKET_RECEIVED "<PACKET RECEIVED>"

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
    srand((unsigned int) time(NULL));

    // Generate random characters and write them to the file
    for (int i = 0; i < SIZE_OF_FILE - 1; i++) {
        fputc(random_alphanumeric(), file);
    }

    //the end of file
    fputc('!', file);
    fclose(file);

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
        } else if (strcmp(argv[i], "-p") == 0) {
            port = atoi(argv[i + 1]);
        }
    }

    // Create socket
    int sockfd = rudp_socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd < 0) {
        perror("Error creating socket\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in dest_addr; // The address of the receiver
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;// Using IPv4
    dest_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &dest_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address/ Address not supported \n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    memset(buffer, 'A', BUFFER_SIZE);
    char ack_buf[36];
    memset(ack_buf, 'B', 36);


    // Three-Way-Handshake
    while (hand_shake_send(buffer, sockfd, dest_addr, BUFFER_SIZE) == 0) {}

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
    char send_again; // Send the file again
    int resend_packet = 1;

    // Send the file until the user ask to stop
    do {

        total_sent = 0;

        //Read the file
        while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file_to_read)) > 0) {

            do {
                //Send the data
                bytes_sent = rudp_send(sockfd, buffer, bytes_read, 0, (struct sockaddr *) &dest_addr,
                                       sizeof(dest_addr));
                if (bytes_sent < 0) {
                    perror("Error sending data\n");
                    break;
                }

                //Receive ack from receiver
                int error_number = ack_recv(sockfd, ack_buf, 36, 0, (struct sockaddr *) &dest_addr,
                                            (socklen_t *) sizeof(dest_addr));

                if (!(error_number != 11 && strcmp(ack_buf, PACKET_RECEIVED) == 0)) {
                    resend_packet = 1;
                } else {
                    resend_packet = 0;
                    total_sent += bytes_sent;
                }
            } while (resend_packet == 1);
        }
        fseek(file_to_read, 0, SEEK_SET); //Reset pointer

        //The byte sent
        printf("Total bytes sent: %zd\n", total_sent);

        //Asking the user if he wants to send again the data
        printf("Send again? (y/n): ");
        scanf("%s", &send_again);

    }while (send_again == 'y' || send_again == 'Y');


    // Send the exit message to the server
    if (rudp_send(sockfd, EXIT_MESSAGE, strlen(EXIT_MESSAGE), 0, (struct sockaddr *) &dest_addr,
                  sizeof(dest_addr)) < 0) {
        perror("Could not send a exit message\n");
        return EXIT_FAILURE;
    } else {
        printf("Sent exit massage\n");
    }

    //Close the socket
    rudp_close(sockfd);

    //Close the file
    fclose(file);

    printf("Sender end\n");

    return 0;
}