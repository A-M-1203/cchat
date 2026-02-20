#ifdef win
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <stdio.h>
#include <string.h>

#define SERVER_PORT "27015"
#define BUFFER_LENGTH 1024

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s <server_name>", argv[0]);
        return 0;
    }

    struct WSAData winsock;
    int result = WSAStartup(MAKEWORD(2, 2), &winsock);
    if (result != 0) {
        printf("Winsock startup failed: %d\n", result);
        return 1;
    }

    struct addrinfo *server_info = NULL, hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    result = getaddrinfo(argv[1], SERVER_PORT, &hints, &server_info);
    if (result != 0) {
        printf("Failed to resolve server address and port: %d\n", result);
        WSACleanup();
        return 1;
    }

    SOCKET client_socket = INVALID_SOCKET;
    client_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    if (client_socket == INVALID_SOCKET) {
        printf("Failed to create socket: %d\n", WSAGetLastError());
        freeaddrinfo(server_info);
        WSACleanup();
        return 1;
    }

    result = connect(client_socket, server_info->ai_addr, (int)server_info->ai_addrlen);
    if (result != 0) {
        printf("Failed to connect to server socket: %d\n", result);
        closesocket(client_socket);
        client_socket = INVALID_SOCKET;
    }

    freeaddrinfo(server_info);
    if (client_socket == INVALID_SOCKET) {
        printf("Failed to connect to server\n");
        WSACleanup();
        return 1;
    }

    const char* send_buffer = "test message";
    char receive_buffer[BUFFER_LENGTH];

    result = send(client_socket, send_buffer, (int)(strlen(send_buffer) + 1), 0);
    if (result == SOCKET_ERROR) {
        printf("Failed to send data to the server: %d\n", WSAGetLastError());
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    printf("Bytes sent to the server: %d\n", result);

    result = shutdown(client_socket, SD_SEND);
    if (result == SOCKET_ERROR) {
        printf("Failed to close client socket for sending data: %d\n", WSAGetLastError());
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    do {
        result = recv(client_socket, receive_buffer, BUFFER_LENGTH, 0);
        if (result > 0) {
            printf("Bytes received from the server: %d\n", result);
            printf("%s\n", receive_buffer);
        } else if (result == 0) {
            printf("Server closed the connection\n");
        } else {
            printf("Failed to receive data from the server: %d\n", WSAGetLastError());
        }
    } while (result > 0);

    closesocket(client_socket);
    WSACleanup();
    return 0;
}
#endif
