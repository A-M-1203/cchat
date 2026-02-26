#include "network_linux.h"

#include <stdio.h>
#define BUFFER_LENGTH 4096
#define LISTEN_QUEUE_LENGTH 10
int main() {
    const char* server_ip_address = "127.0.0.2";
    uint16_t server_port = 3003;
    ccn_socket* server_socket = ccn_open_server_socket(CCN_AF_IP4, CCN_STREAM_SOCKET, BUFFER_LENGTH,
                                                       server_ip_address, server_port);
    if (server_socket == NULL) {
        printf("Failed to open and bind server socket\n");
        return 1;
    }

    printf("Successfully opened and binded server socket %d to address %s:%hu\n", server_socket->fd,
           server_socket->ip_address, server_socket->port);

    int32_t result = ccn_listen(server_socket, LISTEN_QUEUE_LENGTH);
    if (result == -1) {
        printf("Server socket %d failed listening on %s:%hu\n", server_socket->fd,
               server_socket->ip_address, server_socket->port);
        return 1;
    }

    printf("Server socket %d listening on %s:%hu\n", server_socket->fd, server_socket->ip_address,
           server_socket->port);

    ccn_socket* client_socket = ccn_accept_connection(server_socket);
    if (client_socket == NULL) {
        printf("Failed to accept connection\n");
        return 1;
    }

    printf("Client socket %d with address %s:%hu successfully connected\n", client_socket->fd,
           client_socket->ip_address, client_socket->port);

    int64_t bytes_received = ccn_receive_buffer(client_socket, 0);
    if (bytes_received == -1) {
        printf("Failed to receive data\n");
        return 1;
    }

    printf("Received %ld bytes\n", client_socket->buffer_size);
    printf("Data: %s\n", client_socket->buffer);

    result = ccn_close_socket(client_socket);
    if (result == -1) {
        printf("Failed to close socket %d\n", client_socket->fd);
        return 1;
    }
    client_socket = NULL;

    result = ccn_close_socket(server_socket);
    if (result == -1) {
        printf("Failed to close socket %d\n", server_socket->fd);
        return 1;
    }
    server_socket = NULL;

    return 0;
}
