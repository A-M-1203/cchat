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
    int32_t result = create_socket(&server_socket_fd, AF_IP4, SOCKET_STREAM);
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

    result = bind_socket(server_socket_fd, (const struct sockaddr*)&socket_address,
                         sizeof(struct sockaddr_in));
    if (result < 0) {
        printf("Failed to bind socket to %s:%hu\n", ip_address,
               ntohs(((struct sockaddr_in*)&socket_address)->sin_port));
        return -3;
    }

    printf("Socket successfully binded to %s:%hu\n", ip_address,
           ntohs(((struct sockaddr_in*)&socket_address)->sin_port));

    result = listen_on(server_socket_fd, LISTEN_QUEUE_LENGTH);
    if (result < 0) {
        printf("Failed to listen on %s:%hu\n", ip_address,
               ntohs(((struct sockaddr_in*)&socket_address)->sin_port));
        return -4;
    }

    printf("Listening on %s:%hu\n", ip_address,
           ntohs(((struct sockaddr_in*)&socket_address)->sin_port));

    struct sockaddr_in client_socket_address;
    uint32_t client_socket_address_size = sizeof(struct sockaddr_in);
    int32_t client_socket_fd = accept_connection(
        server_socket_fd, (struct sockaddr*)&client_socket_address, &client_socket_address_size);
    if (client_socket_fd < 0) {
        printf("Failed to connect to client.\n");
        return -5;
    }

    char client_ip_address[INET_ADDRSTRLEN] = {0};
    uint16_t client_port;

    get_client_address_and_port(AF_IP4, (const struct sockaddr*)&client_socket_address,
                                client_ip_address, sizeof(client_ip_address), &client_port);

    printf("Successfully connected to client %s:%hu\n", client_ip_address, client_port);

    char receive_buffer[BUFFER_LENGTH] = {0};
    int64_t message_size = receive_data(client_socket_fd, receive_buffer, BUFFER_LENGTH, 0);
    printf("Received %ld bytes\n", message_size);
    printf("Data: %s\n", receive_buffer);

    size_t message_length = strlen(receive_buffer);
    message_size = send_data(client_socket_fd, receive_buffer, message_length, 0);
    printf("Sent %ld bytes\n", message_size);

    close(client_socket_fd);
    close(server_socket_fd);
    return 0;
}
