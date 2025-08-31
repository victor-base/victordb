/**
 * @file viproto.h
 * @brief Vector index protocol function declarations.
 *
 * This header defines the interface for vector database protocol operations
 * including INSERT, SEARCH, DELETE, and MATCH_RESULT message handling.
 */

#ifndef __VIPROTO_H
#define __VIPROTO_H

#include <stdbool.h>
#include <stdint.h>
#include "buffer.h"
#include "protocol.h"

/**
 * @brief Serializes an INSERT request into a CBOR-encoded buffer.
 *
 * Encodes a CBOR array of the form:
 *     [uint64_t id, [float32]]
 *
 * @param buf Output buffer where the CBOR message will be written.
 * @param id Unique identifier to include in the message.
 * @param vec Pointer to the float vector to insert.
 * @param dims Number of elements in the vector.
 * @return 0 on success, -1 on error or insufficient buffer space.
 */
int buffer_write_insert(
    buffer_t *buf,
    uint64_t id,
    uint64_t tag,
    const float *vec,
    size_t dims
);

/**
 * @brief Deserializes an INSERT request from a CBOR-encoded buffer.
 *
 * Expects a CBOR array of the form:
 *     [uint64_t id, [float32]]
 *
 * The function allocates memory for the vector, which must be freed by the caller.
 *
 * @param buf Input buffer containing the CBOR message.
 * @param id Output pointer to the decoded 64-bit ID.
 * @param vec Output pointer to the float vector (allocated).
 * @param dims Output number of elements in the vector.
 * @return 0 on success, -1 on malformed input or memory allocation failure.
 */
int buffer_read_insert(
    const buffer_t *buf,
    uint64_t *id,
    uint64_t *tag,
    float **vec,
    size_t *dims
);

/**
 * @brief Serializes a SEARCH request into a CBOR-encoded buffer.
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
int buffer_write_search(
    buffer_t *buf,
    uint64_t tag,
    const float *vec,
    size_t dims,
    int n
);

/**
 * @brief Deserializes a SEARCH request from a CBOR-encoded buffer.
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
int buffer_read_search(
    const buffer_t *buf,
    uint64_t *tag,
    float **vec,
    size_t *dims,
    int *n
);

/**
 * @brief Serializes a MATCH_RESULT response into a CBOR-encoded buffer.
 *
 * Encodes a CBOR array of the form:
 *     [[id:uint64, distance:float], ...]
 *
 * @param buf Output buffer where the CBOR message will be written.
 * @param ids Array of result identifiers.
 * @param distances Array of distances to the search vector.
 * @param n Number of results.
 * @return 0 on success, -1 on error or insufficient buffer space.
 */
int buffer_write_match_result(
    buffer_t *buf,
    const uint64_t *ids,
    const float *distances,
    size_t n
);

/**
 * @brief Deserializes a MATCH_RESULT response from a CBOR-encoded buffer.
 *
 * Expects a CBOR array of the form:
 *     [[id:uint64, distance:float], ...]
 *
 * The function extracts IDs and distances from the match results.
 *
 * @param buf Input buffer containing the CBOR message.
 * @param ids Output array of result identifiers (preallocated).
 * @param distances Output array of distances (preallocated).
 * @param n Maximum number of results to read.
 * @param out_count Output number of results actually read.
 * @return 0 on success, -1 on malformed input or memory allocation failure.
 */
int buffer_read_match_result(
    const buffer_t *buf,
    uint64_t *ids,
    float    *distances,
    size_t n,
    size_t *out_count
);

/**
 * @brief Serializes a DELETE request into a CBOR-encoded buffer.
 *
 * Encodes a CBOR array of the form:
 *     [id:uint64]
 *
 * @param buf Output buffer where the CBOR message will be written.
 * @param id Identifier to delete.
 * @return 0 on success, -1 on error or insufficient buffer space.
 */
int buffer_write_delete(
    buffer_t *buf,
    uint64_t id
);

/**
 * @brief Deserializes a DELETE request from a CBOR-encoded buffer.
 *
 * Expects a CBOR array of the form:
 *     [id:uint64]
 *
 * @param buf Input buffer containing the CBOR message.
 * @param id Output pointer to the identifier.
 * @return 0 on success, -1 on malformed input.
 */
int buffer_read_delete(
    const buffer_t *buf,
    uint64_t *id
);

#endif /* __VIPROTO_H */