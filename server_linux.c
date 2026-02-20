#include "common.h"

#include <stdio.h>

int main() {
    int32_t socket_fd;
    int result = create_socket(&socket_fd, AF_IP4, SOCKET_STREAM);
    if (result == -1) {
        printf("Failed to create socket.\n");
        return 1;
    }

    printf("Successfully created socket.\n");

    const char* ip_address = "127.0.0.1";
    struct sockaddr socket_address;
    result = create_socket_address(&socket_address, AF_IP4, ip_address, 2000);

    result = bind(socket_fd, (const struct sockaddr*)&socket_address, sizeof(struct sockaddr_in));
    if (result == 0) {
        printf("Socket successfull bound to %s:%hu\n", ip_address,
               ntohs(((struct sockaddr_in*)&socket_address)->sin_port));
    } else {
        printf("Failed to bind socket to %s:%hu\n", ip_address,
               ntohs(((struct sockaddr_in*)&socket_address)->sin_port));
    }
    return 0;
}
