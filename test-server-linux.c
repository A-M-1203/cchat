#include "network_linux.h"

#include <stdio.h>
int main() {
    const char* server_ip_address = "127.0.0.2";
    uint16_t server_port = 3003;
    ccn_server_socket* server_socket = ccn_server_open_and_bind_socket(
        CCN_AF_IP4, CCN_STREAM_SOCKET, server_ip_address, server_port);
    if (server_socket == NULL) {
        printf("Failed to open and bind server socket\n");
        return 1;
    }

    printf("Successfully opened and binded server socket %d to address %s:%hu\n", server_socket->fd,
           server_socket->ip_address, server_socket->port);

    int32_t result = ccn_server_listen(server_socket, CCN_LISTEN_QUEUE_CAPACITY);
    if (result == -1) {
        printf("Server socket %d failed listening on %s:%hu\n", server_socket->fd,
               server_socket->ip_address, server_socket->port);
        return 1;
    }

    printf("Server socket %d listening on %s:%hu\n", server_socket->fd, server_socket->ip_address,
           server_socket->port);

    ccn_client_socket* client_socket = ccn_server_accept_connection(server_socket);
    if (client_socket == NULL) {
        printf("Failed to accept connection\n");
        return 1;
    }

    printf("Client %s:%hu connected\n", client_socket->ip_address, client_socket->port);

    int64_t bytes_received = ccn_receive_buffer(client_socket, 0);
    if (bytes_received == -1) {
        printf("Failed to receive data\n");
        return 1;
    }

    printf("Received %ld bytes\n", client_socket->buffer_size);
    printf("Data: %s\n", client_socket->buffer);

    result = ccn_close_client_socket(client_socket);
    if (result == -1) {
        printf("Failed to close socket %d\n", client_socket->fd);
        return 1;
    }
    client_socket = NULL;

    result = ccn_server_close_socket(server_socket);
    if (result == -1) {
        printf("Failed to close socket %d\n", server_socket->fd);
        return 1;
    }
    server_socket = NULL;

    return 0;
}
