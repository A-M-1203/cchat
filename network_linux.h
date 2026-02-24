#include <arpa/inet.h>
#include <string.h>
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
    char ip_address[CCN_IP6_ADDRESS_LENGTH + 1];
} ccn_socket;

static ccn_socket ccn_create_socket(domain address_family, socket_type type) {
    ccn_socket socket_;
    socket_.fd = socket(address_family, type, 0);
    socket_.address_family = address_family;
    socket_.type = type;
    socket_.ip_address[0] = 0;
    socket_.ip_address[CCN_IP6_ADDRESS_LENGTH] = 0;
    socket_.port = 0;

    return socket_;
}

static int32_t __ccn_get_socket_address_and_port(ccn_socket* socket_) {
    uint32_t socket_address_size = 0;
    if (socket_->address_family == CCN_AF_IP4) {
        struct sockaddr_in socket_address_ipv4 = {0};
        socket_address_size = sizeof(struct sockaddr_in);
        getsockname(socket_->fd, (struct sockaddr*)&socket_address_ipv4, &socket_address_size);
        inet_ntop(CCN_AF_IP4, &socket_address_ipv4.sin_addr, socket_->ip_address,
                  CCN_IP4_ADDRESS_LENGTH);
        socket_->port = ntohs(socket_address_ipv4.sin_port);
    } else if (socket_->address_family == CCN_AF_IP6) {
        struct sockaddr_in6 socket_address_ipv6 = {0};
        socket_address_size = sizeof(struct sockaddr_in6);
        getsockname(socket_->fd, (struct sockaddr*)&socket_address_ipv6, &socket_address_size);
        inet_ntop(CCN_AF_IP6, &socket_address_ipv6.sin6_addr, socket_->ip_address,
                  CCN_IP6_ADDRESS_LENGTH);
        socket_->port = ntohs(socket_address_ipv6.sin6_port);
    } else {
        return -1;
    }

    return 0;
}

static int32_t __ccn_get_sockaddr(struct sockaddr* socket_address, uint32_t* socket_address_size,
                                  domain address_family, const char* ip_address, uint16_t port) {
    if (address_family == CCN_AF_IP4) {
        struct sockaddr_in* socket_address_ipv4 = (struct sockaddr_in*)socket_address;
        socket_address_ipv4->sin_family = address_family;
        if (inet_pton(address_family, ip_address, &socket_address_ipv4->sin_addr) <= 0) {
            return -1;
        }
        socket_address_ipv4->sin_port = htons(port);
        *socket_address_size = sizeof(struct sockaddr_in);
    } else if (address_family == CCN_AF_IP6) {
        struct sockaddr_in6* socket_address_ipv6 = (struct sockaddr_in6*)socket_address;
        socket_address_ipv6->sin6_family = address_family;
        if (inet_pton(address_family, ip_address, &socket_address_ipv6->sin6_addr) <= 0) {
            return -1;
        }
        socket_address_ipv6->sin6_port = htons(port);
        *socket_address_size = sizeof(struct sockaddr_in6);
    } else {
        return -1;
    }

    return 0;
}

static int32_t __ccn_set_client_address_and_port(ccn_socket* client_socket,
                                                 const struct sockaddr* client_socket_address) {
    if (client_socket->address_family == CCN_AF_IP4) {
        inet_ntop(CCN_AF_IP4, &(((struct sockaddr_in*)client_socket_address)->sin_addr),
                  client_socket->ip_address, CCN_IP4_ADDRESS_LENGTH);
        client_socket->port = ntohs(((struct sockaddr_in*)client_socket_address)->sin_port);
    } else if (client_socket->address_family == CCN_AF_IP6) {
        inet_ntop(CCN_AF_IP6, &(((struct sockaddr_in6*)client_socket_address)->sin6_addr),
                  client_socket->ip_address, CCN_IP6_ADDRESS_LENGTH);
        client_socket->port = ntohs(((struct sockaddr_in6*)client_socket_address)->sin6_port);
    } else
        return -1;

    return 0;
}

static int32_t ccn_bind_socket(ccn_socket* socket_, const char* ip_address, uint16_t port) {
    struct sockaddr socket_address = {0};
    uint32_t socket_address_size = 0;
    int32_t result = __ccn_get_sockaddr(&socket_address, &socket_address_size,
                                        socket_->address_family, ip_address, port);
    result = bind(socket_->fd, &socket_address, socket_address_size);
    if (result == -1) {
        return -1;
    }

    strncpy(socket_->ip_address, ip_address, sizeof(socket_->ip_address) - 1);
    socket_->port = port;
    return 0;
}

static ccn_socket ccn_create_server_socket(domain address_family, socket_type type,
                                           const char* server_ip_address, uint16_t server_port) {
    ccn_socket server_socket = ccn_create_socket(address_family, type);
    ccn_bind_socket(&server_socket, server_ip_address, server_port);
    return server_socket;
}

static int32_t ccn_connect(ccn_socket* socket_, const char* ip_address, uint16_t port) {
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

static int32_t ccn_listen(ccn_socket* listening_socket, int32_t backlog) {
    return listen(listening_socket->fd, backlog);
}

static ccn_socket ccn_accept_connection(ccn_socket* listening_socket) {
    struct sockaddr client_socket_address = {0};
    uint32_t client_socket_address_size = sizeof(struct sockaddr_in);
    if (listening_socket->address_family == CCN_AF_IP6)
        client_socket_address_size = sizeof(struct sockaddr_in6);
    int32_t client_socket_fd =
        accept(listening_socket->fd, &client_socket_address, &client_socket_address_size);
    ccn_socket client_socket = {.fd = client_socket_fd,
                                .address_family = listening_socket->address_family,
                                .type = listening_socket->type,
                                .port = 0};
    client_socket.ip_address[0] = 0;
    client_socket.ip_address[CCN_IP6_ADDRESS_LENGTH] = 0;
    int32_t result = __ccn_set_client_address_and_port(&client_socket, &client_socket_address);
    return client_socket;
}

static int32_t ccn_close_socket(ccn_socket* socket_) { return close(socket_->fd); }

// --------------------------------------------------------------------------------------------------

static int64_t send_data(int32_t socket_fd, void* send_buffer, uint64_t send_buffer_length,
                         int32_t flags) {
    return send(socket_fd, send_buffer, send_buffer_length, flags);
}

static int64_t receive_data(int32_t socket_fd, void* receive_buffer, uint64_t receive_buffer_lenght,
                            int32_t flags) {
    return recv(socket_fd, receive_buffer, receive_buffer_lenght, flags);
}
