#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdbool.h>
#include "buffer.h"

#define MSG_ADD             0x01
#define MSG_ADD_RESULT      0x02
#define MSG_DEL             0x03
#define MSG_DEL_RESULT      0x04
#define MSG_LOOKUP          0x05
#define MSG_LOOKUP_RESULT   0x06
#define MSG_ERROR           0x07

#define MSG_MAXLEN          0x0FFFFFFF

/**
 * @brief Serializes an ADD request into a CBOR-encoded buffer.
 *
 * Encodes a CBOR array of the form:
 *     [[float32], bytes]
 *
 * The function writes the CBOR message into the provided buffer.
 *
 * @param buf Output buffer where the CBOR message will be written.
 * @param vec Pointer to the float vector to add.
 * @param dims Number of elements in the vector.
 * @param payload Pointer to the binary payload.
 * @param payload_len Length of the binary payload.
 * @return 0 on success, -1 on error or insufficient buffer space.
 */
int buffer_write_add(
    buffer_t *buf,
    const float *vec,
    size_t dims,
    const uint8_t *payload,
    size_t payload_len
);

/**
 * @brief Deserializes an ADD request from a CBOR-encoded buffer.
 *
 * Expects a CBOR array of the form:
 *     [[float32], bytes]
 *
 * The function allocates memory for the vector and payload, which must be freed by the caller.
 *
 * @param buf Input buffer containing the CBOR message.
 * @param vec Output pointer to the float vector (allocated).
 * @param dims Output number of elements in the vector.
 * @param payload Output pointer to the binary payload (allocated).
 * @param payload_len Output length of the binary payload.
 * @return 0 on success, -1 on malformed input or memory allocation failure.
 */
int buffer_read_add(
    const buffer_t *buf,
    float **vec,
    size_t *dims,
    uint8_t **payload,
    size_t *payload_len
);

/**
 * @brief Serializes an ADD_RESULT response into a CBOR-encoded buffer.
 *
 * Encodes a CBOR array of the form:
 *     [id:uint64]
 *
 * @param buf Output buffer where the CBOR message will be written.
 * @param id The identifier assigned to the added vector.
 * @return 0 on success, -1 on error or insufficient buffer space.
 */
int buffer_write_add_result(
    buffer_t *buf,
    uint64_t id
);

/**
 * @brief Deserializes an ADD_RESULT response from a CBOR-encoded buffer.
 *
 * Expects a CBOR array of the form:
 *     [id:uint64]
 *
 * @param buf Input buffer containing the CBOR message.
 * @param id Output pointer to the identifier.
 * @return 0 on success, -1 on malformed input.
 */
int buffer_read_add_result(
    buffer_t *buf,
    uint64_t *id
);

/**
 * @brief Serializes an ERROR response into a CBOR-encoded buffer.
 *
 * Encodes a CBOR array of the form:
 *     [error_code:int, error_message:string]
 *
 * @param buf Output buffer where the CBOR message will be written.
 * @param code Error code.
 * @param msg Error message string.
 * @return 0 on success, -1 on error or insufficient buffer space.
 */
int buffer_write_error(
    buffer_t *buf,
    int code,
    const char *msg
);

/**
 * @brief Deserializes an ERROR response from a CBOR-encoded buffer.
 *
 * Expects a CBOR array of the form:
 *     [error_code:int, error_message:string]
 *
 * The function allocates memory for the error message, which must be freed by the caller.
 *
 * @param buf Input buffer containing the CBOR message.
 * @param code Output pointer to the error code.
 * @param msg Output pointer to the error message string (allocated).
 * @return 0 on success, -1 on malformed input or memory allocation failure.
 */
int buffer_read_error(
    const buffer_t *buf,
    int *code,
    char **msg
);

/**
 * @brief Serializes a LOOKUP request into a CBOR-encoded buffer.
 *
 * Encodes a CBOR array of the form:
 *     [[float32], int]
 *
 * @param buf Output buffer where the CBOR message will be written.
 * @param vec Pointer to the float vector to search.
 * @param dims Number of elements in the vector.
 * @param n Number of results requested.
 * @return 0 on success, -1 on error or insufficient buffer space.
 */
int buffer_write_lookup(
    buffer_t *buf,
    const float *vec,
    size_t dims,
    int n
);

/**
 * @brief Deserializes a LOOKUP request from a CBOR-encoded buffer.
 *
 * Expects a CBOR array of the form:
 *     [[float32], int]
 *
 * The function allocates memory for the vector, which must be freed by the caller.
 *
 * @param buf Input buffer containing the CBOR message.
 * @param vec Output pointer to the float vector (allocated).
 * @param dims Output number of elements in the vector.
 * @param n Output number of results requested.
 * @return 0 on success, -1 on malformed input or memory allocation failure.
 */
int buffer_read_lookup(
    const buffer_t *buf,
    float **vec,
    size_t *dims,
    int *n
);

/**
 * @brief Serializes a LOOKUP_RESULT response into a CBOR-encoded buffer.
 *
 * Encodes a CBOR array of the form:
 *     [[id:uint64, payload:bytes], ...]
 *
 * @param buf Output buffer where the CBOR message will be written.
 * @param ids Array of result identifiers.
 * @param payloads Array of pointers to result payloads.
 * @param payload_lens Array of payload lengths.
 * @param n Number of results.
 * @return 0 on success, -1 on error or insufficient buffer space.
 */
int buffer_write_lookup_result(
    buffer_t *buf,
    const uint64_t *ids,
    const uint8_t **payloads,
    const size_t *payload_lens,
    size_t n
);

/**
 * @brief Deserializes a LOOKUP_RESULT response from a CBOR-encoded buffer.
 *
 * Expects a CBOR array of the form:
 *     [[id:uint64, payload:bytes], ...]
 *
 * The function allocates memory for each payload, which must be freed by the caller.
 *
 * @param buf Input buffer containing the CBOR message.
 * @param ids Output array of result identifiers (preallocated).
 * @param payloads Output array of pointers to result payloads (allocated).
 * @param payload_lens Output array of payload lengths (preallocated).
 * @param n Maximum number of results to read.
 * @param out_count Output number of results actually read.
 * @return 0 on success, -1 on malformed input or memory allocation failure.
 */
int buffer_read_lookup_result(
    const buffer_t *buf,
    uint64_t *ids,
    uint8_t **payloads,
    size_t *payload_lens,
    size_t n,
    size_t *out_count
);

/**
 * @brief Serializes a DEL request into a CBOR-encoded buffer.
 *
 * Encodes a CBOR array of the form:
 *     [id:uint64]
 *
 * @param buf Output buffer where the CBOR message will be written.
 * @param id Identifier to delete.
 * @return 0 on success, -1 on error or insufficient buffer space.
 */
int buffer_write_del(
    buffer_t *buf,
    uint64_t id
);

/**
 * @brief Deserializes a DEL request from a CBOR-encoded buffer.
 *
 * Expects a CBOR array of the form:
 *     [id:uint64]
 *
 * @param buf Input buffer containing the CBOR message.
 * @param id Output pointer to the identifier.
 * @return 0 on success, -1 on malformed input.
 */
int buffer_read_del(
    const buffer_t *buf,
    uint64_t *id
);

/**
 * @brief Serializes a DEL_RESULT response into a CBOR-encoded buffer.
 *
 * Encodes a CBOR array of the form:
 *     [ok:bool]
 *
 * @param buf Output buffer where the CBOR message will be written.
 * @param ok Boolean indicating success or failure.
 * @return 0 on success, -1 on error or insufficient buffer space.
 */
int buffer_write_del_result(
    buffer_t *buf,
    bool ok
);

/**
 * @brief Deserializes a DEL_RESULT response from a CBOR-encoded buffer.
 *
 * Expects a CBOR array of the form:
 *     [ok:bool]
 *
 * @param buf Input buffer containing the CBOR message.
 * @param ok Output pointer to the boolean result.
 * @return 0 on success, -1 on malformed input.
 */
int buffer_read_del_result(
    const buffer_t *buf,
    bool *ok
);

#endif // PROTOCOL_H