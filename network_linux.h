#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

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

typedef struct ccn_linux_socket {
    int32_t fd;
    domain address_family;
    socket_type type;
    uint16_t port;
    char ip_address[CCN_IP6_ADDRESS_LENGTH + 2];
    uint64_t buffer_size;
    uint64_t buffer_capacity;
    int8_t buffer[];
} ccn_socket;

static ccn_socket* ccn_open_socket(domain address_family, socket_type type,
                                   uint64_t buffer_capacity);
static int32_t ccn_bind_socket(ccn_socket* socket_, const char* ip_address, uint16_t port);
static ccn_socket* ccn_open_server_socket(domain address_family, socket_type type,
                                          uint64_t client_buffer_capacity,
                                          const char* server_ip_address, uint16_t server_port);
static int32_t ccn_connect(ccn_socket* socket_, const char* ip_address, uint16_t port);
static int32_t ccn_listen(ccn_socket* listening_socket, int32_t backlog);
static ccn_socket* ccn_accept_connection(ccn_socket* listening_socket);
static int64_t ccn_send_buffer(ccn_socket* send_socket, int32_t flags);
static int64_t ccn_send_data(ccn_socket* send_socket, const void* data, uint64_t data_length,
                             int32_t flags);
static int64_t ccn_receive_buffer(ccn_socket* receive_socket, int32_t flags);
static int64_t ccn_receive_data(ccn_socket* receive_socket, void* data, uint64_t data_length,
                                int32_t flags);
static int32_t ccn_close_socket(ccn_socket* socket_);

static uint32_t __ccn_ip_address_length(const char* ip_address);
static void __ccn_copy_ip_address(ccn_socket* socket_, const char* ip_address,
                                  uint8_t ip_address_length);
static int32_t __ccn_get_socket_address_and_port(ccn_socket* socket_);
static int32_t __ccn_get_sockaddr(struct sockaddr* socket_address, uint32_t* socket_address_size,
                                  domain address_family, const char* ip_address, uint16_t port);
static int32_t __ccn_set_client_address_and_port(ccn_socket* client_socket,
                                                 const struct sockaddr* client_socket_address);

ccn_socket* ccn_open_socket(domain address_family, socket_type type, uint64_t buffer_capacity) {
    int32_t new_socket_fd = socket(address_family, type, 0);
    if (new_socket_fd == -1) return NULL;
    ccn_socket* new_socket = (ccn_socket*)calloc(1, sizeof(ccn_socket) + buffer_capacity);
    new_socket->fd = new_socket_fd;
    new_socket->address_family = address_family;
    new_socket->type = type;
    new_socket->buffer_capacity = buffer_capacity;
    return new_socket;
}

int32_t ccn_bind_socket(ccn_socket* socket_, const char* ip_address, uint16_t port) {
    struct sockaddr socket_address = {0};
    uint32_t socket_address_size = 0;
    int32_t result = __ccn_get_sockaddr(&socket_address, &socket_address_size,
                                        socket_->address_family, ip_address, port);
    if (result == -1) return -1;
    result = bind(socket_->fd, &socket_address, socket_address_size);
    if (result == -1) return -1;

    __ccn_copy_ip_address(socket_, ip_address, __ccn_ip_address_length(ip_address));
    socket_->port = port;
    return 0;
}

ccn_socket* ccn_open_server_socket(domain address_family, socket_type type,
                                   uint64_t client_buffer_capacity, const char* server_ip_address,
                                   uint16_t server_port) {
    int32_t server_socket_fd = socket(address_family, type, 0);
    if (server_socket_fd == -1) return NULL;
    ccn_socket* server_socket = (ccn_socket*)calloc(1, sizeof(ccn_socket));
    server_socket->fd = server_socket_fd;
    server_socket->address_family = address_family;
    server_socket->type = type;
    server_socket->buffer_capacity = client_buffer_capacity;
    int32_t result = ccn_bind_socket(server_socket, server_ip_address, server_port);
    if (result == -1) return NULL;
    int32_t option = 1;
    result = setsockopt(server_socket->fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    if (result == -1) return NULL;
    return server_socket;
}

int32_t ccn_connect(ccn_socket* socket_, const char* ip_address, uint16_t port) {
    struct sockaddr socket_address = {0};
    uint32_t socket_address_size = 0;
    int32_t result = __ccn_get_sockaddr(&socket_address, &socket_address_size,
                                        socket_->address_family, ip_address, port);
    if (result == -1) return -1;

    result = connect(socket_->fd, &socket_address, socket_address_size);
    if (result == -1) return -1;

    result = __ccn_get_socket_address_and_port(socket_);
    if (result == -1) return -1;

    return 0;
}

int32_t ccn_listen(ccn_socket* listening_socket, int32_t backlog) {
    return listen(listening_socket->fd, backlog);
}

ccn_socket* ccn_accept_connection(ccn_socket* listening_socket) {
    struct sockaddr client_socket_address = {0};
    uint32_t client_socket_address_size = sizeof(struct sockaddr_in);
    if (listening_socket->address_family == CCN_AF_IP6)
        client_socket_address_size = sizeof(struct sockaddr_in6);
    int32_t client_socket_fd =
        accept(listening_socket->fd, &client_socket_address, &client_socket_address_size);
    if (client_socket_fd == -1) return NULL;
    ccn_socket* client_socket =
        (ccn_socket*)calloc(1, sizeof(ccn_socket) + listening_socket->buffer_capacity);
    client_socket->fd = client_socket_fd;
    client_socket->address_family = listening_socket->address_family;
    client_socket->type = listening_socket->type;
    client_socket->buffer_capacity = listening_socket->buffer_capacity;
    int32_t result = __ccn_set_client_address_and_port(client_socket, &client_socket_address);
    if (result == -1) return NULL;
    return client_socket;
}

int64_t ccn_send_buffer(ccn_socket* send_socket, int32_t flags) {
    return send(send_socket->fd, send_socket->buffer, send_socket->buffer_size, flags);
}

int64_t ccn_send_data(ccn_socket* send_socket, const void* data, uint64_t data_length,
                      int32_t flags) {
    return send(send_socket->fd, data, data_length, flags);
}

int64_t ccn_receive_buffer(ccn_socket* receive_socket, int32_t flags) {
    int64_t bytes_received =
        recv(receive_socket->fd, receive_socket->buffer, receive_socket->buffer_capacity, flags);
    if (bytes_received == -1) return -1;
    receive_socket->buffer_size = bytes_received;
    receive_socket->buffer[bytes_received] = 0;
    return bytes_received;
}

int64_t ccn_receive_data(ccn_socket* receive_socket, void* data, uint64_t read_count,
                         int32_t flags) {
    return recv(receive_socket->fd, data, read_count, flags);
}

int32_t ccn_close_socket(ccn_socket* socket_) {
    int32_t result = close(socket_->fd);
    if (result == -1) return -1;
    free(socket_);
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

void __ccn_copy_ip_address(ccn_socket* socket_, const char* ip_address, uint8_t ip_address_length) {
    char* ip_address_ptr = socket_->ip_address;
    for (; ip_address_length > 0; ip_address_length--, ip_address_ptr++, ip_address++)
        *ip_address_ptr = *ip_address;
}

int32_t __ccn_get_socket_address_and_port(ccn_socket* socket_) {
    uint32_t socket_address_size = 0;
    int32_t result;
    const char* ntop_result;
    if (socket_->address_family == CCN_AF_IP4) {
        struct sockaddr_in socket_address_ipv4 = {0};
        socket_address_size = sizeof(struct sockaddr_in);
        result =
            getsockname(socket_->fd, (struct sockaddr*)&socket_address_ipv4, &socket_address_size);
        if (result == -1) return -1;
        ntop_result = inet_ntop(CCN_AF_IP4, &socket_address_ipv4.sin_addr, socket_->ip_address,
                                CCN_IP4_ADDRESS_LENGTH);
        if (ntop_result == NULL) return -1;
        socket_->port = ntohs(socket_address_ipv4.sin_port);
    } else if (socket_->address_family == CCN_AF_IP6) {
        struct sockaddr_in6 socket_address_ipv6 = {0};
        socket_address_size = sizeof(struct sockaddr_in6);
        result =
            getsockname(socket_->fd, (struct sockaddr*)&socket_address_ipv6, &socket_address_size);
        if (result == -1) return -1;
        ntop_result = inet_ntop(CCN_AF_IP6, &socket_address_ipv6.sin6_addr, socket_->ip_address,
                                CCN_IP6_ADDRESS_LENGTH);
        if (ntop_result == NULL) return -1;
        socket_->port = ntohs(socket_address_ipv6.sin6_port);
    } else {
        return -1;
    }

    return 0;
}

int32_t __ccn_get_sockaddr(struct sockaddr* socket_address, uint32_t* socket_address_size,
                           domain address_family, const char* ip_address, uint16_t port) {
    if (address_family == CCN_AF_IP4) {
        struct sockaddr_in* socket_address_ipv4 = (struct sockaddr_in*)socket_address;
        socket_address_ipv4->sin_family = address_family;
        if (inet_pton(address_family, ip_address, &socket_address_ipv4->sin_addr) <= 0) return -1;
        socket_address_ipv4->sin_port = htons(port);
        *socket_address_size = sizeof(struct sockaddr_in);
    } else if (address_family == CCN_AF_IP6) {
        struct sockaddr_in6* socket_address_ipv6 = (struct sockaddr_in6*)socket_address;
        socket_address_ipv6->sin6_family = address_family;
        if (inet_pton(address_family, ip_address, &socket_address_ipv6->sin6_addr) <= 0) return -1;
        socket_address_ipv6->sin6_port = htons(port);
        *socket_address_size = sizeof(struct sockaddr_in6);
    } else
        return -1;

    return 0;
}

int32_t __ccn_set_client_address_and_port(ccn_socket* client_socket,
                                          const struct sockaddr* client_socket_address) {
    const char* ntop_result;
    if (client_socket->address_family == CCN_AF_IP4) {
        ntop_result =
            inet_ntop(CCN_AF_IP4, &(((struct sockaddr_in*)client_socket_address)->sin_addr),
                      client_socket->ip_address, CCN_IP4_ADDRESS_LENGTH);
        if (ntop_result == NULL) return -1;
        client_socket->port = ntohs(((struct sockaddr_in*)client_socket_address)->sin_port);
    } else if (client_socket->address_family == CCN_AF_IP6) {
        ntop_result =
            inet_ntop(CCN_AF_IP6, &(((struct sockaddr_in6*)client_socket_address)->sin6_addr),
                      client_socket->ip_address, CCN_IP6_ADDRESS_LENGTH);
        if (ntop_result == NULL) return -1;
        client_socket->port = ntohs(((struct sockaddr_in6*)client_socket_address)->sin6_port);
    } else
        return -1;

    return 0;
}
