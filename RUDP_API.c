#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdint.h>
#include <string.h>


int rudp_socket(int domain, int type, int protocol){
    int sockfd = socket(domain, type, protocol);
    if (sockfd == -1) {
        perror("socket");
    }
    return sockfd;
}

char* int_to_2_chars(int the_int){
    char binaryString[17];
    int i;
    for (i = 15; i >= 0; --i) {
        binaryString[15 - i] = ((the_int & (1 << i)) ? '1' : '0');
    }
    binaryString[16] = '\0';

    char byte0[9];
    char byte1[9];

    strncpy(byte0, binaryString, 8);
    byte0[8] = '\0';
    strncpy(byte1, binaryString + 8, 9);

    static char chars[2];

    chars[0] = (char)strtol(byte0, NULL, 2);
    chars[1] = (char)strtol(byte1, NULL, 2);

    return chars;
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

    char* bytes01 = int_to_2_chars((int)len);
    char* bytes23 = int_to_2_chars((int) calculate_checksum((void*)buf, len));

    int bytes_sent = sendto(sockfd, buf, len, flags, dest_addr, addrlen);
    if (bytes_sent == -1) {
        perror("rudp_send");
    }
    return bytes_sent;
}

int rudp_recv(int sockfd, void *buf, size_t len, int flags,
             struct sockaddr *src_addr, socklen_t *addrlen) {
    int bytes_received = recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
    if (bytes_received == -1) {
        perror("rudp_recv");
    }
    return bytes_received;
}

int hand_shake_send(char * buffer, int sockfd, const struct sockaddr_in recv_addr, int BUFFER_SIZE){
    int client_seq = 0;
    int server_seq = 0;

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
        return 1;
    }
    else{
        return 0;
    }

}

int hand_shake_recv(char * buffer, int sockfd, const struct sockaddr_in sender_addr, int BUFFER_SIZE){
    int client_seq = 0;
    int server_seq = 0;

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
        return 1;
    }
    else{
       return 0;
   }
}

void rudp_close(int sockfd){
    close(sockfd);
}



