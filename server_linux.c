#include "network_linux.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#define BUFFER_LENGTH 4096
#define LISTEN_QUEUE_LENGTH 10
int main() {
    int32_t server_socket_fd;
    int32_t result = linux_create_socket(&server_socket_fd, AF_IP4, SOCKET_STREAM);
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

    result = linux_bind_socket(server_socket_fd, (const struct sockaddr*)&socket_address,
                               sizeof(struct sockaddr_in));
    if (result < 0) {
        printf("Failed to bind socket to %s:%hu\n", ip_address,
               ntohs(((struct sockaddr_in*)&socket_address)->sin_port));
        return -3;
    }

    printf("Socket successfully binded to %s:%hu\n", ip_address,
           ntohs(((struct sockaddr_in*)&socket_address)->sin_port));

    result = linux_listen(server_socket_fd, LISTEN_QUEUE_LENGTH);
    if (result < 0) {
        printf("Failed to listen on %s:%hu\n", ip_address,
               ntohs(((struct sockaddr_in*)&socket_address)->sin_port));
        return -4;
    }

    printf("Listening on %s:%hu\n", ip_address,
           ntohs(((struct sockaddr_in*)&socket_address)->sin_port));

    struct sockaddr_in client_socket_address;
    uint32_t client_socket_address_size = sizeof(struct sockaddr_in);
    int32_t client_socket_fd = linux_accept(
        server_socket_fd, (struct sockaddr*)&client_socket_address, &client_socket_address_size);
    if (client_socket_fd < 0) {
        printf("Failed to connect to client.\n");
        return -5;
    }

    char client_ip_address[INET_ADDRSTRLEN] = {0};
    uint16_t client_port;

    linux_get_client_address_and_port(AF_IP4, (const struct sockaddr*)&client_socket_address,
                                      client_ip_address, sizeof(client_ip_address), &client_port);

    printf("Successfully connected to client %s:%hu\n", client_ip_address, client_port);

    char receive_buffer[BUFFER_LENGTH] = {0};
    int64_t message_length = linux_receive(client_socket_fd, receive_buffer, BUFFER_LENGTH, 0);
    printf("Message length: %ld bytes\n", message_length);
    printf("Response: %s\n", receive_buffer);

    close(client_socket_fd);
    close(server_socket_fd);
    return 0;
}
