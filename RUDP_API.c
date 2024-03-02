#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>

#define BUFFER_SIZE1 2048
#define TIMEOUT_SEC 0
#define TIMEOUT_US 1000


int rudp_socket(int domain, int type, int protocol){
    int sockfd = socket(domain, type, protocol);
    if (sockfd == -1) {
        perror("socket");
    }
    return sockfd;
}


// Function to convert an integer to a 16-bit binary represented by a string of size 2
char* int_to_2_char_string(unsigned int value) {
    if (value > 65535) {
        fprintf(stderr, "Value out of range for 16-bit representation\n");
        return NULL;
    }

    // Allocate memory for the result
    char* result = malloc(3); // 2 characters + null terminator
    if (!result) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    // Store the higher and lower 8 bits in two separate chars
    result[0] = (char)((value >> 8) & 0xFF); // Higher 8 bits
    result[1] = (char)(value & 0xFF);        // Lower 8 bits
    result[2] = '\0';                        // Null-terminate the string

    return result;
}

unsigned short int calculate_checksum(void *data, unsigned int bytes) {
    unsigned short int *data_pointer = (unsigned short int *)data;
    unsigned int total_sum = 0;
    // Main summing loop
    while (bytes > 1) {
        total_sum += *data_pointer++;
        bytes -= 2;
    }
    // Add left-over byte, if any
    if (bytes > 0)
        total_sum += *((unsigned char *)data_pointer);
    // Fold 32-bit sum to 16 bits
    while (total_sum >> 16)
        total_sum = (total_sum & 0xFFFF) + (total_sum >> 16);
    return (~((unsigned short int)total_sum));
}


int rudp_send(int sockfd, const char *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {

    // Creating the header
    char new_buffer[BUFFER_SIZE1+4]; // Make room for the header
    char* bytes01 = int_to_2_char_string((int)len); // generate length in 16 bits
    char* bytes23 = int_to_2_char_string((int) calculate_checksum((void*)buf, (int)len)); // generate checksum in 16 bits
    char header[5] = "0123"; // header placeholder
    strcpy(new_buffer, header);
    strcat(new_buffer, buf);

    // Placing length and checksum at the header
    new_buffer[0] = bytes01[0];
    new_buffer[1] = bytes01[1];
    new_buffer[2] = bytes23[0];
    new_buffer[3] = bytes23[1];
    free(bytes01);
    free(bytes23);

    // Sending the data and the header
    int bytes_sent = sendto(sockfd, new_buffer, len+4, flags, dest_addr, addrlen);
    if (bytes_sent == -1) {
        perror("rudp_send");
    }

    // Bytes without the header
    return bytes_sent - 4;
}

int rudp_recv(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    char buf1[BUFFER_SIZE1+4]={0};
    int bytes_received = recvfrom(sockfd, buf1, len+4, flags, src_addr, addrlen);
    if (bytes_received == -1) {
        perror("rudp_recv");
    }

    // Parse Length and Checksum from the header
    char header_length[2+1];
    char header_checksum[2+1];
    header_length[0] = buf1[0];
    header_length[1] = buf1[1];
    header_length[2] = '\0';
    header_checksum[0] = buf1[2];
    header_checksum[1] = buf1[3];
    header_checksum[2] = '\0';
    strncpy(buf, buf1 + 4, BUFFER_SIZE1); // Remove header from packet

    // Check if the packet is data or command
    char first_char_in_packet = (char)buf1[4];
    int command = 0;
    if (first_char_in_packet == '<') {
        command = 1;
    }


    // Check packet integrity
    char* recv_len = int_to_2_char_string((int)len);
    char* header_len = header_length;
    int length_ok = (((recv_len[0] == header_len[0]) && (recv_len[1] == header_len[1])) || (command == 1)) ? 1 : 0;

    char* calculated_sum = int_to_2_char_string((int)calculate_checksum((void*)buf, len));
    char* header_sum = header_checksum;
    int checksum_ok = ((calculated_sum[0] == header_sum[0]) && (calculated_sum[1] == header_sum[1])) ? 1 : 0;

    if(length_ok*checksum_ok == 1 || bytes_received == 4){
        return bytes_received - 4;
    }

    else{
        return -1;
    }
}

int ack_recv(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    char buf1[36]={0};

    // Set timeout for the socket
    struct timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = TIMEOUT_US;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Error setting socket timeout");
        exit(EXIT_FAILURE);
    }

    recvfrom(sockfd, buf1, len+4, flags, src_addr, addrlen);
    strncpy(buf, buf1 + 4, 32); // Remove header from packet

    return errno;
}


int hand_shake_send(char * buffer, int sockfd, const struct sockaddr_in recv_addr, int BUFFER_SIZE){
    int client_seq = 0;
    int server_seq = 0;

    printf("Three-Way Handshake has started.\n");

    // Generate initial client sequence number
    srand(time(NULL));
    client_seq = rand();

    // Step 1: Send SYN with sequence number
    sprintf(buffer, "<SYN %d>", client_seq);
    rudp_send(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&recv_addr, sizeof(recv_addr));
    printf("Sender sent: %s\n", buffer);

    // Step 2: Receive SYN-ACK and parse server sequence number
    socklen_t addr_size = sizeof(recv_addr);
    rudp_recv(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&recv_addr, &addr_size);

    char* end_of_type = strchr(buffer, '>');
    char type[100] = {0};
    strncpy(type, buffer, end_of_type - buffer + 1);
    printf("Sender received: %s\n", type);
    int client_seq_recv = 0;
    sscanf(buffer, "<SYN-ACK %d, %d>", &server_seq, &client_seq_recv);

    // Step 3: Send ACK with the next expected server sequence number
    sprintf(buffer, "<ACK %d>", server_seq + 1);
    rudp_send(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&recv_addr, sizeof(recv_addr));
    printf("Sender sent: %s\n", buffer);

    if(client_seq_recv == client_seq + 1){
        printf("Three-Way Handshake is completed successfully.\n");
        return 1;
    }
    else{
        printf("Three-Way Handshake has failed.\n");
        return 0;
    }

}

int hand_shake_recv(char * buffer, int sockfd, const struct sockaddr_in sender_addr, int BUFFER_SIZE){
    int client_seq = 0;
    int server_seq = 0;

    printf("Three-Way Handshake has started.\n");

    // Generate initial server sequence number
    srand(time(NULL));
    server_seq = rand();

    // Step 1: Receive SYN and parse client sequence number
    socklen_t addr_size = sizeof(sender_addr);
    rudp_recv(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&sender_addr, &addr_size);
    printf("Receiver received: %s\n", buffer);
    sscanf(buffer, "<SYN %d>", &client_seq);

    // Step 2: Send SYN-ACK with server sequence number and client ack number
    sprintf(buffer, "<SYN-ACK %d, %d>", server_seq, client_seq + 1);
    rudp_send(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&sender_addr, addr_size);
    printf("Receiver sent: %s\n", buffer);

    // Step 3: Receive ACK and verify client ack number
    rudp_recv(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&sender_addr, &addr_size);
    char* end_of_type = strchr(buffer, '>');
    char type[100] = {0};
    strncpy(type, buffer, end_of_type - buffer + 1);
    printf("Receiver received: %s\n", type);

    int server_seq_recv = 0;
    sscanf(type, "<ACK %d>", &server_seq_recv);

    if(server_seq + 1 == server_seq_recv){
        printf("Three-Way Handshake is completed successfully.\n");
        return 1;
    }
    else{
        printf("Three-Way Handshake has failed.\n");
        return 0;
   }
}

void rudp_close(int sockfd){
    close(sockfd);
}



