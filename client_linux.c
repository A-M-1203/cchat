#ifdef Linux
#include "network_linux.h"

#include <stdint.h>
#include <stdio.h>
#define BUFFER_LENGTH 4096
int main2() {
    int32_t socket_fd;
    int32_t result = create_socket(&socket_fd, AF_IP4, SOCKET_STREAM);
    if (result < 0) {
        printf("Failed to create socket.\n");
        return -1;
    }

    printf("Successfully created socket.\n");

    const char* ip_address = "127.0.0.2";
    struct sockaddr socket_address;
    result = create_socket_address(&socket_address, AF_IP4, ip_address, 2001);
    if (result < 0) {
        printf("Failed to create socket address.\n");
        return -2;
    }

    result = connect_socket(socket_fd, (const struct sockaddr*)&socket_address,
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
    int64_t message_size = send_data(socket_fd, send_buffer, message_length, 0);
    printf("Sent %ld bytes\n", message_size);

    char receive_buffer[BUFFER_LENGTH] = {0};
    message_size = receive_data(socket_fd, receive_buffer, BUFFER_LENGTH, 0);
    printf("Received %ld bytes\n", message_size);
    printf("Data: %s\n", receive_buffer);

    close(socket_fd);
    return 0;
}
#endif
