#include "buffer.h"
#include <string.h>
#include <stdio.h>
#include "socket.h"

/**
 * @brief Serializes a protocol header into a 4-byte buffer.
 *
 * El header ahora usa 4 bits para el tipo y 28 bits para el tamaño.
 * - Los 4 bits más altos: tipo (0-15)
 * - Los 28 bits restantes: tamaño (0-268435455)
 *
 * El resultado se almacena en orden de red de 4 bytes (big-endian).
 *
 * @param buff Pointer to a 4-byte buffer where the header will be written.
 * @param hdr Pointer to the protocol header to serialize.
 * @return 0 on success, -1 if the length exceeds 28-bit range.
 */
static int hdr_serialize(uint8_t *buff, const proto_header_t *hdr) {
    if (hdr->type > 0xF || hdr->len > 0x0FFFFFFF)
        return -1;

    uint32_t raw = ((uint32_t)(hdr->type & 0xF) << 28) | (hdr->len & 0x0FFFFFFF);
    raw = htonl(raw);
    memcpy(buff, &raw, sizeof(raw));  
    return 0;
}

/**
 * @brief Deserializes a 4-byte buffer into a protocol header.
 *
 * Ahora espera:
 * - 4 bits altos: tipo
 * - 28 bits bajos: tamaño
 *
 * @param buff Pointer to the 4-byte buffer to unpack.
 * @param hdr Pointer to the protocol header structure to populate.
 * @return 0 on success, -1 if input pointers are NULL.
 */
static int hdr_deserialize(uint8_t *buff, proto_header_t *hdr) {
    if (!buff || !hdr)
        return -1;

    uint32_t raw;
    memcpy(&raw, buff, 4);
    raw = ntohl(raw);
    hdr->type = (raw >> 28) & 0xF;
    hdr->len  = raw & 0x0FFFFFFF;
    return 0;
}

/**
 * @brief Allocates and initializes a buffer_t structure.
 *
 * @return Pointer to a new buffer_t structure, or NULL on failure.
 */
buffer_t *alloc_buffer(void) {
    buffer_t *b = (buffer_t *)calloc(1, sizeof(buffer_t));
    b->data = &b->_data[4];
    return b;
}

/**
 * @brief Receives a complete message from a file descriptor.
 *
 * Reads the protocol header followed by the message payload.
 *
 * @param fd File descriptor to read from.
 * @param buffer Pointer to buffer where the message will be stored.
 * @return 0 on success, -1 on failure or connection closed.
 */
int recv_msg(int fd, buffer_t *buffer) {
    if (!buffer) return -1;

    if (recv_all(fd, buffer->_data, 4) < 0)
        return -1;

    if (hdr_deserialize(buffer->_data, &buffer->hdr) < 0)
        return -1;

    if (buffer->hdr.len > 0x0FFFFFFF)
        return -1;

    return recv_all(fd, buffer->data, buffer->hdr.len);
}

/**
 * @brief Sends a complete message to a file descriptor.
 *
 * Writes the protocol header followed by the message payload.
 *
 * @param fd File descriptor to write to.
 * @param buffer Pointer to buffer containing the message to send.
 * @return 0 on success, -1 on failure.
 */
int send_msg(int fd, buffer_t *buffer) {
    if (!buffer) return -1;

    if (hdr_serialize(buffer->_data, &buffer->hdr) < 0)
        return -1;

    return send_all(fd, buffer->_data, buffer->hdr.len+4);
}


/**
 * @brief Dumps the buffer (header + payload) to a file (WAL).
 *
 * @param buf Buffer to dump.
 * @param file FILE* already opened for writing (binary).
 * @return 0 on success, -1 on error.
 */
int buffer_dump_wal(const buffer_t *buf, FILE *file) {
    size_t total = 4 + buf->hdr.len;
    if (fwrite(buf->_data, 1, total, file) != total)
        return -1;
    return 0;
}

/**
 * @brief Loads the buffer (header + payload) from a file (WAL).
 *
 * Reads a serialized message from the given file stream into the provided buffer.
 * The message must start with a 4-byte header, followed by a payload of variable size.
 *
 * @param buf Pointer to a buffer_t structure to populate.
 * @param file FILE* already opened in binary mode for reading.
 *
 * @return 
 *   1 if a complete message was successfully read,  
 *   0 if EOF was reached cleanly before reading a new message,  
 *  -1 if an error occurred during reading or if the data is invalid.
 *
 * @note The caller should check for `== 1` before processing the buffer contents.
 */
int buffer_load_wal(buffer_t *buf, FILE *file) {
    if (!buf || !file)
        return -1;

    size_t read = fread(buf->_data, 1, 4, file);
    if (read == 0 && feof(file))
        return 0; 
    if (read != 4)
        return -1;

    if (hdr_deserialize(buf->_data, &buf->hdr) < 0)
        return -1;

    if (buf->hdr.len > 0x0FFFFFFF)
        return -1;

    if (fread(buf->data, 1, buf->hdr.len, file) != (size_t) buf->hdr.len)
        return -1;

    return 1;
}