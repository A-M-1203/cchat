#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <string.h>

#define SERVER_PORT "27015"
#define BUFFER_LENGTH 1024

int main() {
    struct WSAData winsock;
    int result = WSAStartup(MAKEWORD(2, 2), &winsock);
    if (result != 0){
        printf("Winsock startup failed: %d\n", result);
        return 1;
    }

    struct addrinfo *host_info = NULL, hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    result = getaddrinfo(NULL, SERVER_PORT, &hints, &host_info);
    if (result != 0) {
        printf("Failed to resolve server address and port: %d\n", result);
        WSACleanup();
        return 1;
    }

    SOCKET listen_socket = INVALID_SOCKET;
    listen_socket = socket(host_info->ai_family, host_info->ai_socktype, host_info->ai_protocol);
    if (listen_socket == INVALID_SOCKET) {
        printf("Failed to create listen socket: %d\n", WSAGetLastError());
        freeaddrinfo(host_info);
        WSACleanup();
        return 1;
    }

    result = bind(listen_socket, host_info->ai_addr, (int)host_info->ai_addrlen);
    if (result == SOCKET_ERROR) {
        printf("Failed to bind listen socket to an IP address and port: %d\n", WSAGetLastError());
        freeaddrinfo(host_info);
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(host_info);

    if(listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        printf("Failed to listen on listen socket: %d\n", WSAGetLastError());
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    printf("Listening for connection on port %s\n", SERVER_PORT);

    SOCKET client_socket = INVALID_SOCKET;
    client_socket = accept(listen_socket, NULL, NULL);
    if (client_socket == INVALID_SOCKET) {
        printf("Failed to accept client connection: %d\n", WSAGetLastError());
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    closesocket(listen_socket);

    char receive_buffer[BUFFER_LENGTH];
    int send_result;
    do {
        result = recv(client_socket, receive_buffer, BUFFER_LENGTH, 0);
        if (result > 0) {
            printf("Received %d bytes\n", result);
            printf("%s\n", receive_buffer);
            send_result = send(client_socket, receive_buffer, result, 0);
            if (send_result == SOCKET_ERROR) {
                printf("Failed to send data to the client: %d\n", WSAGetLastError());
                closesocket(client_socket);
                WSACleanup();
                return 1;
            }

            printf("Sent %d bytes\n", send_result);
        } else if (result == 0){
            printf("Closing connection...\n");
        } else {
            printf("Failed to receive data from the client: %d\n", WSAGetLastError());
            closesocket(client_socket);
            WSACleanup();
            return 1;
        }
    } while (result > 0);

    result = shutdown(client_socket, SD_SEND);
    if (result == SOCKET_ERROR) {
        printf("Failed to close client socket for sending data: %d\n", WSAGetLastError());
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    closesocket(client_socket);
    WSACleanup();

    return 0;
}
