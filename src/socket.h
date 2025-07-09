#ifndef _IO_SOCKET__H
#define _IO_SOCKET__H 1

#include <stdio.h>
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
extern int recv_all(int fd, void *buf, size_t len);

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
extern int send_all(int fd, const void *buf, size_t len);


/**
 * @brief Creates a UNIX domain socket server at the specified path.
 *
 * This function creates a local stream socket (`AF_UNIX`, `SOCK_STREAM`),
 * binds it to the given file path, and sets it to listen for incoming connections.
 *
 * @param path Filesystem path where the socket will be created (e.g., "/tmp/mysocket").
 * @return A listening socket file descriptor on success, -1 on failure.
 */
extern int unix_server(const char *path);


/**
 * @brief Connects to a UNIX domain socket server at the specified path.
 *
 * This function creates a local stream socket and connects it to the
 * server socket located at the given file system path.
 *
 * @param path Filesystem path of the server socket to connect to.
 * @return A connected socket file descriptor on success, -1 on failure.
 */
extern int unix_connect(const char *path);

/**
 * @brief Acepta una conexión entrante en un socket UNIX.
 *
 * @param server_fd Descriptor del socket servidor (escuchando).
 * @return Descriptor del socket conectado, o -1 en error.
 */
extern int unix_accept(int server_fd);
#endif