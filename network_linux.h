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
} LinuxDomain;

typedef enum linux_socket_type {
    SOCKET_STREAM = SOCK_STREAM,
    SOCKET_DATAGRAM = SOCK_DGRAM,
    SOCKET_RECORD = SOCK_SEQPACKET
} LinuxSocketType;

static int32_t linux_create_socket(int32_t* socket_fd, LinuxDomain address_family,
                                   LinuxSocketType socket_type) {
    *socket_fd = socket(address_family, socket_type, 0);
    return *socket_fd > 0 ? 0 : -1;
}

static int linux_create_socket_address(struct sockaddr* out_socket_address,
                                       LinuxDomain address_family, const char* ip_address,
                                       uint16_t port) {
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

static int32_t linux_bind_socket(int32_t socket_fd, const struct sockaddr* socket_address,
                                 uint32_t socket_address_size) {
    return bind(socket_fd, socket_address, socket_address_size);
}

static int32_t linux_connect_socket(int32_t socket_fd, const struct sockaddr* socket_address,
                                    uint32_t socket_address_size) {
    return connect(socket_fd, socket_address, socket_address_size);
}

static int32_t linux_listen(int32_t socket_fd, int32_t backlog) {
    return listen(socket_fd, backlog);
}

static int32_t linux_accept(int32_t socket_fd, struct sockaddr* client_socket_address,
                            uint32_t* socket_address_size) {
    return accept(socket_fd, client_socket_address, socket_address_size);
}

static int64_t linux_receive(int32_t socket_fd, void* receive_buffer,
                             uint64_t receive_buffer_lenght, int32_t flags) {
    return recv(socket_fd, receive_buffer, receive_buffer_lenght, flags);
}

static void linux_get_client_address_and_port(LinuxDomain address_family,
                                              const struct sockaddr* client_socket_address,
                                              char* client_ip_address,
                                              uint32_t client_ip_address_length,
                                              uint16_t* client_port) {
    inet_ntop(address_family, &(((struct sockaddr_in*)client_socket_address)->sin_addr),
              client_ip_address, client_ip_address_length);
    *client_port = ntohs(((struct sockaddr_in*)&client_socket_address)->sin_port);
}
