#ifndef _BUFFER_H
#define _BUFFER_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
/**
 * @brief Protocol header structure.
 *
 * Contains metadata about the message such as its length and type.
 */
typedef struct {
    int len;
    int type;
} proto_header_t;

/**
 * @brief Buffer structure for message transmission.
 *
 * Includes the protocol header and a preallocated data buffer for reading/writing messages.
 * The maximum size is now 2^28-1 (268,435,455 bytes) + 4 bytes for the header.
 */
typedef struct {
    proto_header_t hdr;
    uint8_t *data;
    uint8_t _data[0x0FFFFFFF + 4];
} buffer_t;

/**
 * @brief Allocates and initializes a buffer_t structure.
 *
 * @return Pointer to a new buffer_t structure, or NULL on failure.
 */
extern buffer_t *alloc_buffer(void);

/**
 * @brief Receives a complete message from a file descriptor.
 *
 * Reads the protocol header followed by the message payload.
 *
 * @param fd File descriptor to read from.
 * @param buffer Pointer to buffer where the message will be stored.
 * @return 0 on success, -1 on failure or connection closed.
 */
extern int recv_msg(int fd, buffer_t *buffer);


/**
 * @brief Sends a complete message to a file descriptor.
 *
 * Writes the protocol header followed by the message payload.
 *
 * @param fd File descriptor to write to.
 * @param buffer Pointer to buffer containing the message to send.
 * @return 0 on success, -1 on failure.
 */
extern int send_msg(int fd, buffer_t *buffer);

extern int buffer_dump_wal(const buffer_t *buf, FILE *file);

extern int buffer_load_wal(buffer_t *buf, FILE *file);
#endif