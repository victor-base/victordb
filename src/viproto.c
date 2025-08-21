/**
 * @file viproto.c  
 * @brief Vector index protocol functions for CBOR message serialization/deserialization.
 *
 * This file contains functions for handling vector database operations including
 * INSERT, SEARCH, DELETE, and MATCH_RESULT message types specific to vector operations.
 */

#include "buffer.h"
#include "viproto.h"
#include <cbor.h>
#include <stdlib.h>
#include <string.h>
#include "panic.h"


/**
 * @brief Serializes an INSERT request into a CBOR-encoded buffer.
 *
 * Encodes a CBOR array of the form:
 *     [uint64_t id, [float32]]
 *
 * This function builds a CBOR array with two elements:
 *   - The first element is an unsigned 64-bit integer representing the ID.
 *   - The second element is an array of float32 values representing the vector.
 *
 * The resulting CBOR-encoded message is written into the provided buffer
 * and the buffer header is populated accordingly (type = MSG_INSERT).
 *
 * @param buf Output buffer where the CBOR message will be written.
 * @param id  Unique identifier to include in the message.
 * @param vec Pointer to the float vector to insert.
 * @param dims Number of elements in the vector.
 * @return 0 on success, -1 on error or if the message exceeds buffer size.
 */
int buffer_write_insert(buffer_t *buf, uint64_t id, const float *vec, size_t dims) {
    cbor_item_t *root = NULL;
    cbor_item_t *vec_arr = NULL; 
    cbor_item_t *id_item = NULL;
    size_t written;

    PANIC_IF(!buf, "buffer cannot be null");
    PANIC_IF(!buf->data, "buffer data cannot be null");
    PANIC_IF(!vec && dims > 0, "vector cannot be null with non-zero dimensions");

    root = cbor_new_definite_array(2);
    if (!root) return -1;

    vec_arr = cbor_new_definite_array(dims);
    if (!vec_arr) {
        cbor_decref(&root);
        return -1;
    }

    id_item = cbor_build_uint64(id);
    if (!id_item) {
        cbor_decref(&vec_arr);
        cbor_decref(&root);
        return -1;
    }
    if (!cbor_array_push(root, id_item)) {
        cbor_decref(&id_item);
        cbor_decref(&vec_arr);
        cbor_decref(&root);
        return -1;
    }
    cbor_decref(&id_item);

    for (size_t i = 0; i < dims; i++) {
        cbor_item_t *f = cbor_build_float4(vec[i]);
        if (!f) {
            cbor_decref(&vec_arr);
            cbor_decref(&root);
            return -1;
        }
        if (!cbor_array_push(vec_arr, f)) {
            cbor_decref(&f);
            cbor_decref(&vec_arr);
            cbor_decref(&root);
            return -1;
        }
        cbor_decref(&f);
    }
    if (!cbor_array_push(root, vec_arr)) {
        cbor_decref(&vec_arr);
        cbor_decref(&root);
        return -1;
    }
    cbor_decref(&vec_arr);

    written = cbor_serialize(root, buf->data, MSG_MAXLEN);
    if (written == 0 || written > MSG_MAXLEN) {
        cbor_decref(&root);
        return -1;
    }

    buf->hdr.len = (int)written;
    buf->hdr.type = MSG_INSERT;

    cbor_decref(&root);
    return 0;
}


/**
 * @brief Deserializes an INSERT request from a CBOR-encoded buffer.
 *
 * Expects a CBOR array of the form:
 *     [uint64_t id, [float32]]
 *
 * The function parses the CBOR message and extracts:
 *   - A 64-bit unsigned integer as the ID.
 *   - A float vector from the second element.
 *
 * Memory for the vector is dynamically allocated and must be freed by the caller.
 *
 * @param buf Input buffer containing the CBOR message.
 * @param id Output pointer to the decoded 64-bit ID.
 * @param vec Output pointer to the float vector (allocated).
 * @param dims Output number of elements in the vector.
 * @return 0 on success, -1 on malformed input or memory allocation failure.
 */
int buffer_read_insert(const buffer_t *buf, uint64_t *id, float **vec, size_t *dims) {
    struct cbor_load_result result;
    cbor_item_t *root = NULL;
    cbor_item_t *id_item = NULL;
    cbor_item_t *vec_arr = NULL;

    PANIC_IF(!buf, "buffer cannot be null");
    PANIC_IF(!buf->data, "buffer data cannot be null");
    PANIC_IF(!id, "id output parameter cannot be null");
    PANIC_IF(!vec, "vec output parameter cannot be null");
    PANIC_IF(!dims, "dims output parameter cannot be null");
    if (buf->hdr.len == 0 || buf->hdr.len > MSG_MAXLEN) return -1;

    root = cbor_load(buf->data, buf->hdr.len, &result);
    if (!root || !cbor_isa_array(root) || cbor_array_size(root) != 2) {
        if (root) cbor_decref(&root);
        return -1;
    }

    // Extract ID
    id_item = cbor_array_handle(root)[0];
    if (!id_item || !cbor_isa_uint(id_item) || cbor_int_get_width(id_item) > CBOR_INT_64) {
        cbor_decref(&root);
        return -1;
    }
    *id = cbor_get_uint64(id_item);

    // Extract vector
    vec_arr = cbor_array_handle(root)[1];
    if (!vec_arr || !cbor_isa_array(vec_arr)) {
        cbor_decref(&root);
        return -1;
    }

    *dims = cbor_array_size(vec_arr);
    if (*dims == 0) {
        *vec = NULL;
        cbor_decref(&root);
        return 0;
    }

    *vec = malloc(sizeof(float) * (*dims));
    if (!*vec) {
        cbor_decref(&root);
        return -1;
    }

    for (size_t i = 0; i < *dims; i++) {
        cbor_item_t *f = cbor_array_handle(vec_arr)[i];
        if (!f || !cbor_isa_float_ctrl(f)) {
            free(*vec);
            *vec = NULL;
            cbor_decref(&root);
            return -1;
        }

        switch (cbor_float_get_width(f)) {
            case CBOR_FLOAT_32:
                (*vec)[i] = cbor_float_get_float4(f);
                break;
            case CBOR_FLOAT_64:
                (*vec)[i] = (float)cbor_float_get_float8(f);
                break;
            default:
                free(*vec);
                *vec = NULL;
                cbor_decref(&root);
                return -1;
        }
    }

    cbor_decref(&root);
    return 0;
}

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
int buffer_write_search(buffer_t *buf, const float *vec, size_t dims, int n) {
    cbor_item_t *root = NULL;
    cbor_item_t *vec_arr = NULL;
    cbor_item_t *n_item = NULL;
    size_t written;

    PANIC_IF(!buf, "buffer cannot be null");
    PANIC_IF(!buf->data, "buffer data cannot be null");
    PANIC_IF(!vec && dims > 0, "vector cannot be null with non-zero dimensions");

    root = cbor_new_definite_array(2);
    if (!root) return -1;

    vec_arr = cbor_new_definite_array(dims);
    if (!vec_arr) {
        cbor_decref(&root);
        return -1;
    }

    for (size_t i = 0; i < dims; i++) {
        cbor_item_t *f = cbor_build_float4(vec[i]);
        if (!f) {
            cbor_decref(&vec_arr);
            cbor_decref(&root);
            return -1;
        }
        if (!cbor_array_push(vec_arr, f)) {
            cbor_decref(&f);
            cbor_decref(&vec_arr);
            cbor_decref(&root);
            return -1;
        }
        cbor_decref(&f);
    }
    if (!cbor_array_push(root, vec_arr)) {
        cbor_decref(&vec_arr);
        cbor_decref(&root);
        return -1;
    }
    cbor_decref(&vec_arr);

    n_item = cbor_build_uint32((uint32_t)n);
    if (!n_item) {
        cbor_decref(&root);
        return -1;
    }
    if (!cbor_array_push(root, n_item)) {
        cbor_decref(&n_item);
        cbor_decref(&root);
        return -1;
    }
    cbor_decref(&n_item);

    written = cbor_serialize(root, buf->data, MSG_MAXLEN);
    if (written == 0 || written > MSG_MAXLEN) {
        cbor_decref(&root);
        return -1;
    }

    buf->hdr.len = (int)written;
    buf->hdr.type = MSG_SEARCH;

    cbor_decref(&root);
    return 0;
}

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
int buffer_read_search(const buffer_t *buf, float **vec, size_t *dims, int *n) {
    struct cbor_load_result result;
    cbor_item_t *root = NULL;
    cbor_item_t *vec_arr = NULL;
    cbor_item_t *n_item = NULL;

    PANIC_IF(!buf, "buffer cannot be null");
    PANIC_IF(!buf->data, "buffer data cannot be null");
    PANIC_IF(!vec, "vec output parameter cannot be null");
    PANIC_IF(!dims, "dims output parameter cannot be null");
    PANIC_IF(!n, "n output parameter cannot be null");
    if (buf->hdr.len == 0 || buf->hdr.len > MSG_MAXLEN) return -1;

    root = cbor_load(buf->data, buf->hdr.len, &result);
    if (!root || !cbor_isa_array(root) || cbor_array_size(root) != 2) {
        if (root) cbor_decref(&root);
        return -1;
    }

    vec_arr = cbor_array_handle(root)[0];
    if (!vec_arr || !cbor_isa_array(vec_arr)) {
        cbor_decref(&root);
        return -1;
    }

    *dims = cbor_array_size(vec_arr);
    if (*dims == 0) {
        *vec = NULL;
    } else {
        *vec = malloc(sizeof(float) * (*dims));
        if (!*vec) {
            cbor_decref(&root);
            return -1;
        }

        for (size_t i = 0; i < *dims; i++) {
            cbor_item_t *f = cbor_array_handle(vec_arr)[i];
            if (!f || !cbor_isa_float_ctrl(f)) {
                free(*vec);
                *vec = NULL;
                cbor_decref(&root);
                return -1;
            }
            switch (cbor_float_get_width(f)) {
                case CBOR_FLOAT_32:
                    (*vec)[i] = cbor_float_get_float4(f);
                    break;
                case CBOR_FLOAT_64:
                    (*vec)[i] = (float)cbor_float_get_float8(f);
                    break;
                default:
                    free(*vec);
                    *vec = NULL;
                    cbor_decref(&root);
                    return -1;
            }
        }
    }

    n_item = cbor_array_handle(root)[1];
    if (!n_item || !cbor_isa_uint(n_item)) {
        if (*vec) {
            free(*vec);
            *vec = NULL;
        }
        cbor_decref(&root);
        return -1;
    }
    
    // Handle different integer sizes from CBOR encoding
    switch (cbor_int_get_width(n_item)) {
        case CBOR_INT_8:
            *n = (int)cbor_get_uint8(n_item);
            break;
        case CBOR_INT_16:
            *n = (int)cbor_get_uint16(n_item);
            break;
        case CBOR_INT_32:
            *n = (int)cbor_get_uint32(n_item);
            break;
        case CBOR_INT_64:
            *n = (int)cbor_get_uint64(n_item);
            break;
        default:
            if (*vec) {
                free(*vec);
                *vec = NULL;
            }
            cbor_decref(&root);
            return -1;
    }

    cbor_decref(&root);
    return 0;
}

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
    const float    *distances,
    size_t n
) {
    cbor_item_t *root = NULL;
    size_t written;

    PANIC_IF(!buf, "buffer cannot be null");
    PANIC_IF(!buf->data, "buffer data cannot be null");
    PANIC_IF(!ids, "ids array cannot be null");
    PANIC_IF(!distances, "distances array cannot be null");

    root = cbor_new_definite_array(n);
    if (!root) return -1;

    for (size_t i = 0; i < n; i++) {
        cbor_item_t *pair = cbor_new_definite_array(2);
        if (!pair) {
            cbor_decref(&root);
            return -1;
        }

        cbor_item_t *id_item = cbor_build_uint64(ids[i]);
        if (!id_item) {
            cbor_decref(&pair);
            cbor_decref(&root);
            return -1;
        }
        if (!cbor_array_push(pair, id_item)) {
            cbor_decref(&id_item);
            cbor_decref(&pair);
            cbor_decref(&root);
            return -1;
        }
        cbor_decref(&id_item);

        cbor_item_t *distance_item = cbor_build_float4(distances[i]);
        if (!distance_item) {
            cbor_decref(&pair);
            cbor_decref(&root);
            return -1;
        }
        if (!cbor_array_push(pair, distance_item)) {
            cbor_decref(&distance_item);
            cbor_decref(&pair);
            cbor_decref(&root);
            return -1;
        }
        cbor_decref(&distance_item);

        if (!cbor_array_push(root, pair)) {
            cbor_decref(&pair);
            cbor_decref(&root);
            return -1;
        }
        cbor_decref(&pair);
    }

    written = cbor_serialize(root, buf->data, MSG_MAXLEN);
    if (written == 0 || written > MSG_MAXLEN) {
        cbor_decref(&root);
        return -1;
    }

    buf->hdr.len = (int)written;
    buf->hdr.type = MSG_MATCH_RESULT;

    cbor_decref(&root);
    return 0;
}

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
    size_t    n, 
    size_t *out_count
) {
    struct cbor_load_result result;
    cbor_item_t *root = NULL;

    PANIC_IF(!buf, "buffer cannot be null");
    PANIC_IF(!buf->data, "buffer data cannot be null");
    PANIC_IF(!ids, "ids array cannot be null");
    PANIC_IF(!distances, "distances array cannot be null");
    PANIC_IF(!out_count, "out_count cannot be null");
    if (buf->hdr.len == 0 || buf->hdr.len > MSG_MAXLEN) return -1;
    
    root = cbor_load(buf->data, buf->hdr.len, &result);
    if (!root || !cbor_isa_array(root)) {
        if (root) cbor_decref(&root);
        return -1;
    }

    size_t count = cbor_array_size(root);
    if (count > n) count = n;

    for (size_t i = 0; i < count; i++) {
        cbor_item_t *pair = cbor_array_handle(root)[i];
        if (!pair || !cbor_isa_array(pair) || cbor_array_size(pair) != 2) {
            cbor_decref(&root);
            return -1;
        }

        cbor_item_t *id_item = cbor_array_handle(pair)[0];
        if (!id_item || !cbor_isa_uint(id_item)) {
            cbor_decref(&root);
            return -1;
        }
        ids[i] = cbor_get_uint64(id_item);

        cbor_item_t *distance_item = cbor_array_handle(pair)[1];
        if (!distance_item || !cbor_is_float(distance_item)) {
            cbor_decref(&root);
            return -1;
        }
        distances[i] = cbor_float_get_float4(distance_item);
    }
    *out_count = count;
    cbor_decref(&root);
    return 0;
}

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
int buffer_write_delete(buffer_t *buf, uint64_t id) {
    cbor_item_t *root = NULL;
    cbor_item_t *id_item = NULL;
    size_t written;

    PANIC_IF(!buf, "buffer cannot be null");
    PANIC_IF(!buf->data, "buffer data cannot be null");

    root = cbor_new_definite_array(1);
    if (!root) return -1;

    id_item = cbor_build_uint64(id);
    if (!id_item) {
        cbor_decref(&root);
        return -1;
    }
    if (!cbor_array_push(root, id_item)) {
        cbor_decref(&id_item);
        cbor_decref(&root);
        return -1;
    }
    cbor_decref(&id_item);

    written = cbor_serialize(root, buf->data, MSG_MAXLEN);
    if (written == 0 || written > MSG_MAXLEN) {
        cbor_decref(&root);
        return -1;
    }

    buf->hdr.len = (int)written;
    buf->hdr.type = MSG_DELETE;

    cbor_decref(&root);
    return 0;
}

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
int buffer_read_delete(const buffer_t *buf, uint64_t *id) {
    struct cbor_load_result result;
    cbor_item_t *root = NULL;
    cbor_item_t *id_item = NULL;

    PANIC_IF(!buf, "buffer cannot be null");
    PANIC_IF(!buf->data, "buffer data cannot be null");
    PANIC_IF(!id, "id output parameter cannot be null");
    if (buf->hdr.len == 0 || buf->hdr.len > MSG_MAXLEN) return -1;

    root = cbor_load(buf->data, buf->hdr.len, &result);
    if (!root || !cbor_isa_array(root) || cbor_array_size(root) != 1) {
        if (root) cbor_decref(&root);
        return -1;
    }

    id_item = cbor_array_handle(root)[0];
    if (!id_item || !cbor_isa_uint(id_item)) {
        cbor_decref(&root);
        return -1;
    }
    *id = cbor_get_uint64(id_item);
    cbor_decref(&root);
    return 0;
}
