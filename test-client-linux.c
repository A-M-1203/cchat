#include "network_linux.h"

#include <stdio.h>
#define BUFFER_LENGTH 4096
int main() {
    ccn_socket client_socket = ccn_create_socket(CCN_AF_IP4, CCN_STREAM_SOCKET);

    printf("Successfully created client socket %d\n", client_socket.fd);

    const char* server_ip_address = "127.0.0.2";
    uint16_t server_port = 3000;
    ccn_connect(&client_socket, server_ip_address, server_port);

    printf("Client: %s:%hu\n", client_socket.ip_address, client_socket.port);
    printf("Successfully connected to server %s:%hu\n", server_ip_address, server_port);

    close(client_socket.fd);

    return 0;
}
