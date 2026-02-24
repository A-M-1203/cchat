#include "network_linux.h"

#include <stdio.h>
#define BUFFER_LENGTH 4096
#define LISTEN_QUEUE_LENGTH 10
int main() {
    const char* server_ip_address = "127.0.0.2";
    uint16_t server_port = 3000;
    ccn_socket server_socket =
        ccn_create_server_socket(CCN_AF_IP4, CCN_STREAM_SOCKET, server_ip_address, server_port);

    printf("Successfully created and binded server socket %d to address %s:%hu\n", server_socket.fd,
           server_socket.ip_address, server_socket.port);

    int32_t result = ccn_listen(&server_socket, LISTEN_QUEUE_LENGTH);
    if (result == -1) {
        printf("Server socket %d failed listening on %s:%hu\n", server_socket.fd,
               server_socket.ip_address, server_socket.port);
    }

    printf("Server socket %d listening on %s:%hu\n", server_socket.fd, server_socket.ip_address,
           server_socket.port);

    ccn_socket client_socket = ccn_accept_connection(&server_socket);
    printf("Client socket %d with address %s:%hu successfully connected\n", client_socket.fd,
           client_socket.ip_address, client_socket.port);

    close(server_socket.fd);

    return 0;
}
