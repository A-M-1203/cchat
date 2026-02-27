#ifndef CCN_H
#define CCN_H

#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef CCN_BUFFER_CAPACITY
#define CCN_BUFFER_CAPACITY 256
#endif

#ifndef CCN_LISTEN_QUEUE_CAPACITY
#define CCN_LISTEN_QUEUE_CAPACITY 100
#endif

#define CCN_AF_UNSPECIFIED AF_UNSPEC
#define CCN_AF_IP4 AF_INET
#define CCN_AF_IP6 AF_INET6

#define CCN_STREAM_SOCKET SOCK_STREAM
#define CCN_DATAGRAM_SOCKET SOCK_DGRAM
#define CCN_RECORD_SOCKET SOCK_SEQPACKET

#define CCN_IP4_ADDRESS_LENGTH INET_ADDRSTRLEN
#define CCN_IP6_ADDRESS_LENGTH INET6_ADDRSTRLEN

typedef int32_t domain;
typedef int32_t socket_type;
typedef struct sockaddr socket_address_info;
typedef struct sockaddr_in socket_address_info_ip4;
typedef struct sockaddr_in6 socket_address_info_ip6;

typedef struct ccn_client_linux_socket {
    uint64_t buffer_size;
    int32_t fd;
    domain address_family;
    socket_type type;
    uint16_t port;
    char ip_address[CCN_IP6_ADDRESS_LENGTH + 1];
    int8_t buffer[CCN_BUFFER_CAPACITY];
} ccn_client_socket;

typedef struct ccn_server_linux_socket {
    int32_t fd;
    domain address_family;
    socket_type type;
    uint16_t port;
    char ip_address[CCN_IP6_ADDRESS_LENGTH + 1];
} ccn_server_socket;

// functions that are called only in client application (start with ccn_client_)
static ccn_client_socket* ccn_client_open_socket(domain address_family, socket_type type,
                                                 int32_t fd);
static int32_t ccn_client_connect(ccn_client_socket* client_socket, const char* server_ip_address,
                                  uint16_t server_port);

// functions that are called only in server application (start with ccn_server_)
static ccn_server_socket* ccn_server_open_socket(domain address_family, socket_type type);
static int32_t ccn_server_bind_socket(ccn_server_socket* server_socket, const char* ip_address,
                                      uint16_t port);
static ccn_server_socket* ccn_server_open_and_bind_socket(domain address_family, socket_type type,
                                                          const char* server_ip_address,
                                                          uint16_t server_port);
static int32_t ccn_server_listen(ccn_server_socket* server_socket, int32_t backlog);
static ccn_client_socket* ccn_server_accept_connection(ccn_server_socket* server_socket);
static int32_t ccn_server_close_socket(ccn_server_socket* server_socket);

// functions that are called in both client and server applications (start with ccn_)
static int64_t ccn_send_buffer(ccn_client_socket* client_socket, int32_t flags);
static int64_t ccn_send_data(ccn_client_socket* client_socket, const void* data,
                             uint64_t data_length, int32_t flags);
static int64_t ccn_receive_buffer(ccn_client_socket* client_socket, int32_t flags);
static int64_t ccn_receive_data(ccn_client_socket* client_socket, void* data,
                                uint64_t receive_count, int32_t flags);
static int32_t ccn_close_client_socket(ccn_client_socket* client_socket);

// functions that are only called by this library (start with __ccn_)
static uint32_t __ccn_ip_address_length(const char* ip_address);
static void __ccn_server_copy_ip_address(ccn_server_socket* server_socket,
                                         const char* server_ip_address,
                                         uint8_t server_ip_address_length);
static int32_t __ccn_client_get_socket_address_and_port(ccn_client_socket* client_socket);
static int32_t __ccn_get_socket_address_info(socket_address_info* socket_address,
                                             uint32_t* socket_address_size, domain address_family,
                                             const char* ip_address, uint16_t port);
static int32_t
__ccn_server_set_client_address_and_port(ccn_client_socket* client_socket,
                                         const socket_address_info* client_socket_address_info);

ccn_client_socket* ccn_client_open_socket(domain address_family, socket_type type, int32_t fd) {
    int32_t client_socket_fd;
    if (fd > 0)
        client_socket_fd = fd;
    else
        client_socket_fd = socket(address_family, type, 0);
    if (client_socket_fd == -1) return NULL;
    ccn_client_socket* client_socket = (ccn_client_socket*)calloc(1, sizeof(ccn_client_socket));
    client_socket->fd = client_socket_fd;
    client_socket->address_family = address_family;
    client_socket->type = type;
    return client_socket;
}

int32_t ccn_client_connect(ccn_client_socket* client_socket, const char* server_ip_address,
                           uint16_t server_port) {
    socket_address_info server_socket_address_info = {0};
    uint32_t server_socket_address_info_size = 0;
    int32_t result = __ccn_get_socket_address_info(
        &server_socket_address_info, &server_socket_address_info_size,
        client_socket->address_family, server_ip_address, server_port);
    if (result == -1) return -1;

    result =
        connect(client_socket->fd, &server_socket_address_info, server_socket_address_info_size);
    if (result == -1) return -1;

    result = __ccn_client_get_socket_address_and_port(client_socket);
    if (result == -1) return -1;

    return 0;
}

ccn_server_socket* ccn_server_open_socket(domain address_family, socket_type type) {
    int32_t server_socket_fd = socket(address_family, type, 0);
    if (server_socket_fd == -1) return NULL;
    ccn_server_socket* client_socket = (ccn_server_socket*)calloc(1, sizeof(ccn_server_socket));
    client_socket->fd = server_socket_fd;
    client_socket->address_family = address_family;
    client_socket->type = type;
    return client_socket;
}

int32_t ccn_server_bind_socket(ccn_server_socket* server_socket, const char* server_ip_address,
                               uint16_t server_port) {
    socket_address_info server_socket_address_info = {0};
    uint32_t server_socket_address_info_size = 0;
    int32_t result = __ccn_get_socket_address_info(
        &server_socket_address_info, &server_socket_address_info_size,
        server_socket->address_family, server_ip_address, server_port);
    if (result == -1) return -1;
    result = bind(server_socket->fd, &server_socket_address_info, server_socket_address_info_size);
    if (result == -1) return -1;

    uint8_t server_ip_address_length = __ccn_ip_address_length(server_ip_address);
    __ccn_server_copy_ip_address(server_socket, server_ip_address, server_ip_address_length);
    server_socket->port = server_port;
    return 0;
}

ccn_server_socket* ccn_server_open_and_bind_socket(domain address_family, socket_type type,
                                                   const char* server_ip_address,
                                                   uint16_t server_port) {
    int32_t server_socket_fd = socket(address_family, type, 0);
    if (server_socket_fd == -1) return NULL;
    ccn_server_socket* server_socket = (ccn_server_socket*)calloc(1, sizeof(ccn_server_socket));
    server_socket->fd = server_socket_fd;
    server_socket->address_family = address_family;
    server_socket->type = type;
    int32_t result = ccn_server_bind_socket(server_socket, server_ip_address, server_port);
    if (result == -1) return NULL;
    int32_t option = 1;
    result = setsockopt(server_socket->fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    if (result == -1) return NULL;
    return server_socket;
}

int32_t ccn_server_listen(ccn_server_socket* server_socket, int32_t backlog) {
    return listen(server_socket->fd, backlog);
}

ccn_client_socket* ccn_server_accept_connection(ccn_server_socket* server_socket) {
    socket_address_info client_socket_address_info = {0};
    uint32_t client_socket_address_info_size = sizeof(socket_address_info_ip4);
    if (server_socket->address_family == CCN_AF_IP6)
        client_socket_address_info_size = sizeof(socket_address_info_ip6);
    int32_t client_socket_fd =
        accept(server_socket->fd, &client_socket_address_info, &client_socket_address_info_size);
    if (client_socket_fd == -1) return NULL;
    ccn_client_socket* client_socket = ccn_client_open_socket(
        server_socket->address_family, server_socket->type, client_socket_fd);
    int32_t result =
        __ccn_server_set_client_address_and_port(client_socket, &client_socket_address_info);
    if (result == -1) return NULL;
    return client_socket;
}

int32_t ccn_server_close_socket(ccn_server_socket* server_socket) {
    int32_t result = close(server_socket->fd);
    if (result == -1) return -1;
    free(server_socket);
    return 0;
}

int64_t ccn_send_buffer(ccn_client_socket* client_socket, int32_t flags) {
    return send(client_socket->fd, client_socket->buffer, client_socket->buffer_size, flags);
}

int64_t ccn_send_data(ccn_client_socket* client_socket, const void* data, uint64_t data_length,
                      int32_t flags) {
    return send(client_socket->fd, data, data_length, flags);
}

int64_t ccn_receive_buffer(ccn_client_socket* client_socket, int32_t flags) {
    int64_t bytes_received =
        recv(client_socket->fd, client_socket->buffer, CCN_BUFFER_CAPACITY, flags);
    if (bytes_received == -1) return -1;
    client_socket->buffer_size = bytes_received;
    if (bytes_received < CCN_BUFFER_CAPACITY) client_socket->buffer[bytes_received] = 0;
    return bytes_received;
}

int64_t ccn_receive_data(ccn_client_socket* client_socket, void* data, uint64_t receive_count,
                         int32_t flags) {
    return recv(client_socket->fd, data, receive_count, flags);
}

int32_t ccn_close_client_socket(ccn_client_socket* client_socket) {
    int32_t result = close(client_socket->fd);
    if (result == -1) return -1;
    free(client_socket);
    return 0;
}

uint32_t __ccn_ip_address_length(const char* ip_address) {
    uint32_t length = 0;
    while (*ip_address) {
        length++;
        ip_address++;
    }
    return length;
}

void __ccn_server_copy_ip_address(ccn_server_socket* server_socket, const char* ip_address,
                                  uint8_t ip_address_length) {
    char* ip_address_ptr = server_socket->ip_address;
    for (; ip_address_length > 0; ip_address_length--, ip_address_ptr++, ip_address++)
        *ip_address_ptr = *ip_address;
}

int32_t __ccn_client_get_socket_address_and_port(ccn_client_socket* client_socket) {
    uint32_t client_socket_address_size;
    int32_t result;
    const char* ntop_result;
    if (client_socket->address_family == CCN_AF_IP4) {
        socket_address_info_ip4 client_socket_address_ipv4 = {0};
        client_socket_address_size = sizeof(socket_address_info_ip4);
        result = getsockname(client_socket->fd, (socket_address_info*)&client_socket_address_ipv4,
                             &client_socket_address_size);
        if (result == -1) return -1;
        ntop_result = inet_ntop(CCN_AF_IP4, &client_socket_address_ipv4.sin_addr,
                                client_socket->ip_address, CCN_IP4_ADDRESS_LENGTH);
        if (ntop_result == NULL) return -1;
        client_socket->port = ntohs(client_socket_address_ipv4.sin_port);
    } else if (client_socket->address_family == CCN_AF_IP6) {
        socket_address_info_ip6 client_socket_address_ipv6 = {0};
        client_socket_address_size = sizeof(socket_address_info_ip6);
        result = getsockname(client_socket->fd, (socket_address_info*)&client_socket_address_ipv6,
                             &client_socket_address_size);
        if (result == -1) return -1;
        ntop_result = inet_ntop(CCN_AF_IP6, &client_socket_address_ipv6.sin6_addr,
                                client_socket->ip_address, CCN_IP6_ADDRESS_LENGTH);
        if (ntop_result == NULL) return -1;
        client_socket->port = ntohs(client_socket_address_ipv6.sin6_port);
    } else {
        return -1;
    }

    return 0;
}

int32_t __ccn_get_socket_address_info(socket_address_info* socket_address,
                                      uint32_t* socket_address_size, domain address_family,
                                      const char* ip_address, uint16_t port) {
    if (address_family == CCN_AF_IP4) {
        socket_address_info_ip4* socket_address_ipv4 = (socket_address_info_ip4*)socket_address;
        socket_address_ipv4->sin_family = address_family;
        if (inet_pton(address_family, ip_address, &socket_address_ipv4->sin_addr) <= 0) return -1;
        socket_address_ipv4->sin_port = htons(port);
        *socket_address_size = sizeof(socket_address_info_ip4);
    } else if (address_family == CCN_AF_IP6) {
        socket_address_info_ip6* socket_address_ipv6 = (socket_address_info_ip6*)socket_address;
        socket_address_ipv6->sin6_family = address_family;
        if (inet_pton(address_family, ip_address, &socket_address_ipv6->sin6_addr) <= 0) return -1;
        socket_address_ipv6->sin6_port = htons(port);
        *socket_address_size = sizeof(socket_address_info_ip6);
    } else
        return -1;

    return 0;
}

int32_t
__ccn_server_set_client_address_and_port(ccn_client_socket* client_socket,
                                         const socket_address_info* client_socket_address_info) {
    const char* ntop_result;
    if (client_socket->address_family == CCN_AF_IP4) {
        ntop_result = inet_ntop(CCN_AF_IP4,
                                &(((socket_address_info_ip4*)client_socket_address_info)->sin_addr),
                                client_socket->ip_address, CCN_IP4_ADDRESS_LENGTH);
        if (ntop_result == NULL) return -1;
        client_socket->port =
            ntohs(((socket_address_info_ip4*)client_socket_address_info)->sin_port);
    } else if (client_socket->address_family == CCN_AF_IP6) {
        ntop_result = inet_ntop(
            CCN_AF_IP6, &(((socket_address_info_ip6*)client_socket_address_info)->sin6_addr),
            client_socket->ip_address, CCN_IP6_ADDRESS_LENGTH);
        if (ntop_result == NULL) return -1;
        client_socket->port =
            ntohs(((socket_address_info_ip6*)client_socket_address_info)->sin6_port);
    } else
        return -1;

    return 0;
}

#endif // CCN_H
