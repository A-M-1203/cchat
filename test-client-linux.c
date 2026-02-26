#include "network_linux.h"

#include <stdio.h>
#include <string.h>
#define BUFFER_LENGTH 4096
int main() {
    ccn_socket* client_socket = ccn_open_socket(CCN_AF_IP4, CCN_STREAM_SOCKET, BUFFER_LENGTH);
    if (client_socket == NULL) {
        printf("Failed to open socket\n");
        return 1;
    }

    printf("Successfully opened client socket %d\n", client_socket->fd);

    const char* server_ip_address = "127.0.0.2";
    uint16_t server_port = 3003;
    int32_t result = ccn_connect(client_socket, server_ip_address, server_port);
    if (result == -1) {
        printf("Failed to connect to server %s:%hu\n", server_ip_address, server_port);
        return 1;
    }

    printf("Client: %s:%hu\n", client_socket->ip_address, client_socket->port);
    printf("Successfully connected to server %s:%hu\n", server_ip_address, server_port);

    char* message = fgets((char*)client_socket->buffer, client_socket->buffer_capacity, stdin);
    uint64_t message_length = strlen(message);
    int64_t bytes_sent = ccn_send_data(client_socket, message, message_length, 0);
    if (bytes_sent == -1) {
        printf("Failed to send data to: %s:%hu\n", server_ip_address, server_port);
        return 1;
    }

    printf("Successfully sent %ld bytes to: %s:%hu\n", bytes_sent, server_ip_address, server_port);
    result = ccn_close_socket(client_socket);
    if (result == -1) {
        printf("Failed to close socket %d\n", client_socket->fd);
        return 1;
    }
    client_socket = NULL;
    return 0;
}
