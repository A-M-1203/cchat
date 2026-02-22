#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

typedef enum linux_domain {
    AF_UNSPECIFIED = AF_UNSPEC,
    AF_IP4 = AF_INET,
    AF_IP6 = AF_INET6
} Domain;

typedef enum linux_socket_type {
    SOCKET_STREAM = SOCK_STREAM,
    SOCKET_DATAGRAM = SOCK_DGRAM,
    SOCKET_RECORD = SOCK_SEQPACKET
} SocketType;

static int32_t create_socket(int32_t* socket_fd, Domain address_family, SocketType socket_type) {
    *socket_fd = socket(address_family, socket_type, 0);
    return *socket_fd > 0 ? 0 : -1;
}

static int create_socket_address(struct sockaddr* out_socket_address, Domain address_family,
                                 const char* ip_address, uint16_t port) {
    if (address_family == AF_IP4) {
        memset(out_socket_address, 0, sizeof(struct sockaddr_in));
        struct sockaddr_in* socket_address = (struct sockaddr_in*)out_socket_address;
        socket_address->sin_family = address_family;
        if (inet_pton(address_family, ip_address, &socket_address->sin_addr) <= 0) {
            return -1;
        }
        socket_address->sin_port = htons(port);
    } else if (address_family == AF_IP6) {
        memset(out_socket_address, 0, sizeof(struct sockaddr_in6));
        struct sockaddr_in6* socket_address = (struct sockaddr_in6*)out_socket_address;
        socket_address->sin6_family = address_family;
        if (inet_pton(address_family, ip_address, &socket_address->sin6_addr) <= 0) {
            return -1;
        }
        socket_address->sin6_port = htons(port);
    } else {
        return -2;
    }

    return 0;
}

static int32_t bind_socket(int32_t socket_fd, const struct sockaddr* socket_address,
                           uint32_t socket_address_size) {
    return bind(socket_fd, socket_address, socket_address_size);
}

static int32_t connect_socket(int32_t socket_fd, const struct sockaddr* socket_address,
                              uint32_t socket_address_size) {
    return connect(socket_fd, socket_address, socket_address_size);
}

static int32_t listen_on(int32_t socket_fd, int32_t backlog) { return listen(socket_fd, backlog); }

static int32_t accept_connection(int32_t socket_fd, struct sockaddr* client_socket_address,
                                 uint32_t* socket_address_size) {
    return accept(socket_fd, client_socket_address, socket_address_size);
}

static int64_t send_data(int32_t socket_fd, void* send_buffer, uint64_t send_buffer_length,
                         int32_t flags) {
    return send(socket_fd, send_buffer, send_buffer_length, flags);
}

static int64_t receive_data(int32_t socket_fd, void* receive_buffer, uint64_t receive_buffer_lenght,
                            int32_t flags) {
    return recv(socket_fd, receive_buffer, receive_buffer_lenght, flags);
}

static void get_client_address_and_port(Domain address_family,
                                        const struct sockaddr* client_socket_address,
                                        char* client_ip_address, uint32_t client_ip_address_length,
                                        uint16_t* client_port) {
    inet_ntop(address_family, &(((struct sockaddr_in*)client_socket_address)->sin_addr),
              client_ip_address, client_ip_address_length);
    *client_port = ntohs(((struct sockaddr_in*)&client_socket_address)->sin_port);
}
