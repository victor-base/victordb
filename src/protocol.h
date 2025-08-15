/**
 * @file protocol.h
 * @brief Common protocol definitions and shared functions.
 *
 * This header defines message types and common protocol functions
 * shared between vector operations (viproto) and key-value operations (kvproto).
 */

#ifndef __PROTOCOL_H
#define __PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>
#include "buffer.h"

/* Vector protocol message types */
#define MSG_INSERT          0x01
#define MSG_INSERT_RESULT   0x02
#define MSG_DELETE          0x03
#define MSG_DELETE_RESULT   0x04
#define MSG_SEARCH          0x05
#define MSG_MATCH_RESULT    0x06


/* Key-Value protocol message types */
#define MSG_PUT             0x08
#define MSG_PUT_RESULT      0x09
#define MSG_GET             0x0A
#define MSG_GET_RESULT      0x0B
#define MSG_DEL             0x0C
#define MSG_DEL_RESULT      0x0D

#define MSG_ERROR           0x07
#define MSG_MAXLEN          0x0FFFFFFF

/**
 * @brief Serializes an operation result response into a CBOR-encoded buffer.
 *
 * Encodes a CBOR array of the form:
 *     [error_code:int, error_message:string]
 *
 * @param buf Output buffer where the CBOR message will be written.
 * @param msg_type The message type (MSG_INSERT_RESULT, MSG_DELETE_RESULT, etc.).
 * @param code Error code (0 for success, non-zero for error).
 * @param msg Error message string (can be NULL for success).
 * @return 0 on success, -1 on error or insufficient buffer space.
 */
int buffer_write_op_result(
    buffer_t *buf,
    int msg_type,
    int code,
    const char *msg
);

/**
 * @brief Deserializes an operation result response from a CBOR-encoded buffer.
 *
 * Expects a CBOR array of the form:
 *     [error_code:int, error_message:string]
 *
 * The function allocates memory for the error message, which must be freed by the caller.
 *
 * @param buf Input buffer containing the CBOR message.
 * @param code Output pointer to the error code (0 for success).
 * @param msg Output pointer to the error message string (allocated).
 * @return 0 on success, -1 on malformed input or memory allocation failure.
 */
int buffer_read_op_result(
    const buffer_t *buf,
    int *code,
    char **msg
);

#endif /* __PROTOCOL_H */