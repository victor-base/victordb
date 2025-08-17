#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * @brief Receives exactly `len` bytes from a file descriptor.
 *
 * This function attempts to read exactly `len` bytes from the given file descriptor `fd`,
 * storing the result in the buffer `buf`. It uses `MSG_WAITALL` to block until all bytes
 * are received, unless an error occurs or the connection is closed prematurely.
 *
 * @param fd File descriptor to read from.
 * @param buf Pointer to the buffer where received data will be stored.
 * @param len Number of bytes to read.
 * @return 0 on success, -1 on error or premature connection closure.
 */
int recv_all(int fd, void *buf, size_t len) {
    uint8_t *p = (uint8_t *)buf;
    while (len > 0) {
        ssize_t r = recv(fd, p, len, MSG_WAITALL);
        if (r <= 0) return -1;
        p += r;
        len -= r;
    }
    return 0;
}

/**
 * @brief Sends exactly `len` bytes to a file descriptor.
 *
 * This function ensures that all bytes in the buffer `buf` are sent over the
 * file descriptor `fd`. It uses `MSG_NOSIGNAL` to avoid generating a SIGPIPE
 * if the connection is broken.
 *
 * @param fd File descriptor to write to.
 * @param buf Pointer to the buffer containing data to send.
 * @param len Number of bytes to send.
 * @return 0 on success, -1 on error.
 */
int send_all(int fd, const void *buf, size_t len) {
    const uint8_t *p = (const uint8_t *)buf;
    while (len > 0) {
        ssize_t w = send(fd, p, len, MSG_NOSIGNAL);
        if (w <= 0) return -1;
        p += w;
        len -= w;
    }
    return 0;
}

/**
 * @brief Creates a UNIX domain socket server at the specified path.
 *
 * This function creates a local stream socket (`AF_UNIX`, `SOCK_STREAM`),
 * binds it to the given file path, and sets it to listen for incoming connections.
 *
 * @param path Filesystem path where the socket will be created (e.g., "/tmp/mysocket").
 * @return A listening socket file descriptor on success, -1 on failure.
 */
int unix_server(const char *path) {
    struct sockaddr_un addr;
    int sd;

    memset(&addr, 0, sizeof(struct sockaddr_un));
#ifdef __APPLE__   /* o __FreeBSD__, etc. */
    addr.sun_len = sizeof(struct sockaddr_un);
#endif
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, path);

    sd = socket(PF_LOCAL, SOCK_STREAM, 0);
    if (sd < 0)
        return -1;

    unlink(path);

    if (bind(sd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) != 0) {
        close(sd);
        return -1;
    }
    if (listen(sd, 5) != 0) {
        close(sd);
        return -1;
    }
    return sd;
}


/**
 * @brief Connects to a UNIX domain socket server at the specified path.
 *
 * This function creates a local stream socket and connects it to the
 * server socket located at the given file system path.
 *
 * @param path Filesystem path of the server socket to connect to.
 * @return A connected socket file descriptor on success, -1 on failure.
 */
int unix_connect(const char *path) {
    struct sockaddr_un addr;
    int sd;

    memset(&addr, 0, sizeof(struct sockaddr_un));
#ifdef __APPLE__   /* o __FreeBSD__, etc. */
    addr.sun_len = sizeof(struct sockaddr_un);
#endif
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, path);

    sd = socket(PF_LOCAL, SOCK_STREAM, 0);
    if (sd < 0)
        return -1;

    if (connect(sd,(struct sockaddr *)&addr, sizeof(struct sockaddr_un)) != 0) {
        close(sd);
        return -1;
    }
    return sd;
}

/**
 * @brief Accepts an incoming connection on a UNIX domain socket.
 *
 * This function accepts a new connection on the socket referred to by the
 * file descriptor `server_fd`. The accepted connection is returned as a new
 * file descriptor, which can be used to communicate with the connected client.
 *
 * @param server_fd File descriptor of the listening server socket.
 * @return A new file descriptor for the accepted connection, or -1 on error.
 */
int unix_accept(int server_fd) {
    struct sockaddr_un addr;
    socklen_t addrlen = sizeof(addr);
    int client_fd = accept(server_fd, (struct sockaddr *)&addr, &addrlen);
    if (client_fd < 0)
        return -1;
    return client_fd;
}
