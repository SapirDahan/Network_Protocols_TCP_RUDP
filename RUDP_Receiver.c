#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>

int rudp_socket(int domain, int type, int protocol);
ssize_t rudp_send(int sockfd, char* buffer, ssize_t bytes_read, int flag, const struct sockaddr *addr, socklen_t addr_len);
int rudp_recv(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
int hand_shake_recv(char * buffer, int sockfd, const struct sockaddr_in sender_addr, int BUFFER_SIZE);
void rudp_close(int sockfd);

#define BUFFER_SIZE 2048 // The size of the buffer
#define EXIT_MESSAGE "<exit>" // The exit massage
#define PACKET_RECEIVED "<PACKET RECEIVED>"
#define PROBABILITY_LOSS 0.5

// Function to calculate the difference in time between start and end time
double time_diff(struct timeval start, struct timeval end) {
    return (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
}

int main(int argc, char *argv[]) {

    //We are supposed to get 3 arguments from the user
    if (argc != 3) {
        fprintf(stderr, "Supposed to get an argument this style: ./RUDP_Receiver -p PORT\n");
        exit(EXIT_FAILURE); // There is a problem with the input
    }

    int port; // The port

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0) {
            port = atoi(argv[i + 1]);
        }
    }

    // Opening socket using IPV4
    printf("Starting Receiver...\n");
    int sockfd = rudp_socket(AF_INET, SOCK_DGRAM, 0);

    //Problem opening the socket
    if (sockfd < 0) {
        perror("Error creating socket\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in my_addr, client_addr; //The receiver and client address

    memset(&my_addr, 0, sizeof(my_addr));

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    //Bind the socket
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
        perror("Bind failed\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE]; // An array with the buffer size

    // Three-Way-Handshake
    while(hand_shake_recv(buffer, sockfd, client_addr, BUFFER_SIZE) == 0){}

    ssize_t bytes_received;
    ssize_t total_received = 0;

    long all_run_receive = 0;

    int run_number = 1;

    struct timeval start, end;
    double time;
    double total_time = 0;


    socklen_t addr_size = sizeof(client_addr);

    char *isEOF = NULL;

    printf("----------------------------------\n");
    printf("-         * Statistics *         -\n");
    // Receive data until the sender closes the connection
    while (1) {

        // Receive the data
        bytes_received = rudp_recv(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &addr_size);

        // Send packet received ack with probability
        //sleep(2); // test feature: delay for triggering a timeout
        if(bytes_received > 0 && (double)rand() / (double)RAND_MAX > PROBABILITY_LOSS){
            rudp_send(sockfd, PACKET_RECEIVED, strlen(PACKET_RECEIVED), 0, (struct sockaddr *)&client_addr, addr_size);

            //Symbol end of file
            isEOF = strchr(buffer, '!');
        }

        else{
            bytes_received = 0;
            isEOF = NULL;
        }


        //Start to count for start
        if(total_received == 0 && bytes_received > 0){
            gettimeofday(&start, NULL);
        }

        //Count only if not exit massage
        if (strncmp(buffer, EXIT_MESSAGE, strlen(EXIT_MESSAGE)) != 0 && bytes_received > 0) {
            total_received += bytes_received;
            all_run_receive += bytes_received;
        }

        //If we did not finish yet
        if ((isEOF != NULL && total_received > 0) && strncmp(buffer, EXIT_MESSAGE, strlen(EXIT_MESSAGE)) != 0) {
            gettimeofday(&end, NULL);
            time = time_diff(start, end);
            total_time += time;

            printf("- Run #%d Data: Bytes Received: %d Bytes; Time: %.2f ms; Speed: %.2f MB/s\n", run_number, (int)total_received, time, (double)total_received/(time*1000));

            run_number += 1;

            total_received = 0;
            bytes_received = 0;
        }

        // Check for the exit message
        if (strncmp(buffer, EXIT_MESSAGE, strlen(EXIT_MESSAGE)) == 0) {
            printf("\nExit message received. Closing connection.\n\n");
            break; // Exit the loop to close the connection
        }

        bytes_received = 0;
    }


    if (bytes_received < 0) {
        perror("Receive failed");
    }

        //Print averages
    else {
        printf("- Average time: %.2f ms\n", total_time/(run_number - 1));
        printf("- Average bandwidth: %.2f MB/s\n", (double)all_run_receive/(total_time*1000));
        printf("----------------------------------\n");
        printf("\nReceiver ended\n");
    }


    //Close socket
    rudp_close(sockfd);

    return 0;
}
