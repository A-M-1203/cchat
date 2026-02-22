#include "network_linux.h"

#include <stdio.h>
#define BUFFER_LENGTH 4096
int main() {
    int32_t socket_fd;
    int32_t result = linux_create_socket(&socket_fd, AF_IP4, SOCKET_STREAM);
    if (result < 0) {
        printf("Failed to create socket.\n");
        return -1;
    }

    printf("Successfully created socket.\n");

    const char* ip_address = "127.0.0.2";
    struct sockaddr socket_address;
    result = linux_create_socket_address(&socket_address, AF_IP4, ip_address, 2001);
    if (result < 0) {
        printf("Failed to create socket address.\n");
        return -2;
    }

    result = linux_connect_socket(socket_fd, (const struct sockaddr*)&socket_address,
                                  sizeof(struct sockaddr_in));
    if (result < 0) {
        printf("Failed to establish connection to %s:%hu.\n", ip_address,
               ntohs(((struct sockaddr_in*)&socket_address)->sin_port));
        return -3;
    }

    printf("Successfully established connection to %s:%hu\n", ip_address,
           ntohs(((struct sockaddr_in*)&socket_address)->sin_port));

    char send_buffer[BUFFER_LENGTH] = {0};
    fgets(send_buffer, BUFFER_LENGTH, stdin);
    size_t message_length = strlen(send_buffer);
    send(socket_fd, send_buffer, message_length, 0);

    char receive_buffer[BUFFER_LENGTH] = {0};
    // recv(socket_fd, receive_buffer, BUFFER_LENGTH, 0);
    // printf("Response: %s\n", receive_buffer);

    close(socket_fd);
    return 0;
}
