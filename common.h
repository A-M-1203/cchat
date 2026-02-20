#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>

#define BUFFER_LENGTH 4096

typedef enum domain {
    AF_UNSPECIFIED = AF_UNSPEC,
    AF_IP4 = AF_INET,
    AF_IP6 = AF_INET6
} Domain;

typedef enum socket_type {
    SOCKET_STREAM = SOCK_STREAM,
    SOCKET_DATAGRAM = SOCK_DGRAM,
    SOCKET_RECORD = SOCK_SEQPACKET
} SocketType;

static int32_t create_socket(int32_t* socket_fd, Domain address_family, SocketType socket_type) {
    *socket_fd = socket(address_family, socket_type, 0);
    return *socket_fd > 0 ? 1 : -1;
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

    return 1;
}

static int32_t connect_socket(int32_t socket_fd, const struct sockaddr* socket_address,
                              uint32_t socket_address_size) {
    return connect(socket_fd, socket_address, socket_address_size);
}
