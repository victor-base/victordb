#include "buffer.h"
#include "protocol.h"
#include <cbor.h>
#include <stdlib.h>
#include <string.h>

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
int buffer_write_add(buffer_t *buf, const float *vec, size_t dims, const uint8_t *payload, size_t payload_len) {
    cbor_item_t *root = cbor_new_definite_array(2);
    cbor_item_t *vec_arr = cbor_new_definite_array(dims);
    for (size_t i = 0; i < dims; i++) {
        cbor_item_t *f = cbor_build_float4(vec[i]);
        (void)cbor_array_push(vec_arr, f);
        cbor_decref(&f);
    }
    (void)cbor_array_push(root, vec_arr);
    cbor_decref(&vec_arr);

    cbor_item_t *payload_item = cbor_build_bytestring(payload, payload_len);
    (void)cbor_array_push(root, payload_item);
    cbor_decref(&payload_item);

    size_t written = cbor_serialize(root, buf->data, MSG_MAXLEN);
    if (written == 0 || written > MSG_MAXLEN) {
        cbor_decref(&root);
        return -1;
    }
    buf->hdr.len = (int)written;
    buf->hdr.type = MSG_ADD;

    cbor_decref(&root);
    return 0;
}

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
int buffer_read_add(const buffer_t *buf, float **vec, size_t *dims, uint8_t **payload, size_t *payload_len) {
    struct cbor_load_result result;
    cbor_item_t *root = cbor_load(buf->data, buf->hdr.len, &result);
    if (!root || !cbor_isa_array(root) || cbor_array_size(root) != 2) {
        if (root) cbor_decref(&root);
        return -1;
    }
    cbor_item_t *vec_arr = cbor_array_handle(root)[0];
    if (!cbor_isa_array(vec_arr)) {
        cbor_decref(&root);
        return -1;
    }
    *dims = cbor_array_size(vec_arr);
    *vec = malloc(sizeof(float) * (*dims));
    for (size_t i = 0; i < *dims; i++) {
        cbor_item_t *f = cbor_array_handle(vec_arr)[i];
        if (!cbor_isa_float_ctrl(f)) {
            cbor_decref(&root);
            free(*vec);
            return -1;
        }
        (*vec)[i] = cbor_float_get_float4(f);
    }
    cbor_item_t *payload_item = cbor_array_handle(root)[1];
    if (!cbor_isa_bytestring(payload_item)) {
        cbor_decref(&root);
        free(*vec);
        return -1;
    }
    *payload_len = cbor_bytestring_length(payload_item);
    *payload = malloc(*payload_len);
    memcpy(*payload, cbor_bytestring_handle(payload_item), *payload_len);
    cbor_decref(&root);
    return 0;
}

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
int buffer_write_add_result(buffer_t *buf, uint64_t id) {
    cbor_item_t *root = cbor_new_definite_array(1);
    cbor_item_t *i = cbor_build_uint64(id);
    (void)cbor_array_push(root, i);
    cbor_decref(&i);

    buf->hdr.len = cbor_serialize(root, buf->data, MSG_MAXLEN);
    buf->hdr.type = MSG_ADD_RESULT;
    cbor_decref(&root);
    return 0;
}

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
int buffer_read_add_result(buffer_t *buf, uint64_t *id) {
    struct cbor_load_result result;
    cbor_item_t *root = cbor_load(buf->data, buf->hdr.len, &result);
    if (!root || !cbor_isa_array(root) || cbor_array_size(root) != 1) {
        if (root) cbor_decref(&root);
        return -1;
    }
    cbor_item_t *item = cbor_array_handle(root)[0];
    if (!cbor_isa_uint(item)) {
        cbor_decref(&root);
        return -1;
    }
    *id = cbor_get_uint64(item);
    cbor_decref(&root);
    return 0;
}

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
int buffer_write_error(buffer_t *buf, int code, const char *msg) {
    cbor_item_t *root = cbor_new_definite_array(2);
    cbor_item_t *c = cbor_build_uint32((uint32_t)code);
    (void)cbor_array_push(root, c);
    cbor_decref(&c);

    cbor_item_t *s = cbor_build_string(msg ? msg : "");
    (void)cbor_array_push(root, s);
    cbor_decref(&s);

    size_t written = cbor_serialize(root, buf->data, MSG_MAXLEN);
    if (written == 0 || written > MSG_MAXLEN) {
        cbor_decref(&root);
        return -1;
    }
    buf->hdr.len = (int)written;
    buf->hdr.type = MSG_ERROR;

    cbor_decref(&root);
    return 0;
}

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
int buffer_read_error(const buffer_t *buf, int *code, char **msg) {
    struct cbor_load_result result;
    cbor_item_t *root = cbor_load(buf->data, buf->hdr.len, &result);
    if (!root || !cbor_isa_array(root) || cbor_array_size(root) != 2) {
        if (root) cbor_decref(&root);
        return -1;
    }
    cbor_item_t *c = cbor_array_handle(root)[0];
    if (!cbor_isa_uint(c)) {
        cbor_decref(&root);
        return -1;
    }
    *code = (int)cbor_get_uint32(c);

    cbor_item_t *s = cbor_array_handle(root)[1];
    if (!cbor_isa_string(s)) {
        cbor_decref(&root);
        return -1;
    }
    size_t len = cbor_string_length(s);
    *msg = malloc(len + 1);
    memcpy(*msg, cbor_string_handle(s), len);
    (*msg)[len] = 0;

    cbor_decref(&root);
    return 0;
}

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
int buffer_write_lookup(buffer_t *buf, const float *vec, size_t dims, int n) {
    cbor_item_t *root = cbor_new_definite_array(2);
    cbor_item_t *vec_arr = cbor_new_definite_array(dims);
    for (size_t i = 0; i < dims; i++) {
        cbor_item_t *f = cbor_build_float4(vec[i]);
        (void)cbor_array_push(vec_arr, f);
        cbor_decref(&f);
    }
    (void)cbor_array_push(root, vec_arr);
    cbor_decref(&vec_arr);

    cbor_item_t *n_item = cbor_build_uint32((uint32_t)n);
    (void)cbor_array_push(root, n_item);
    cbor_decref(&n_item);

    size_t written = cbor_serialize(root, buf->data, MSG_MAXLEN);
    if (written == 0 || written > MSG_MAXLEN) {
        cbor_decref(&root);
        return -1;
    }
    buf->hdr.len = (int)written;
    buf->hdr.type = MSG_LOOKUP;

    cbor_decref(&root);
    return 0;
}

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
int buffer_read_lookup(const buffer_t *buf, float **vec, size_t *dims, int *n) {
    struct cbor_load_result result;
    cbor_item_t *root = cbor_load(buf->data, buf->hdr.len, &result);
    if (!root || !cbor_isa_array(root) || cbor_array_size(root) != 2) {
        if (root) cbor_decref(&root);
        return -1;
    }
    cbor_item_t *vec_arr = cbor_array_handle(root)[0];
    if (!cbor_isa_array(vec_arr)) {
        cbor_decref(&root);
        return -1;
    }
    *dims = cbor_array_size(vec_arr);
    *vec = malloc(sizeof(float) * (*dims));
    for (size_t i = 0; i < *dims; i++) {
        cbor_item_t *f = cbor_array_handle(vec_arr)[i];
        if (!cbor_isa_float_ctrl(f)) {
            cbor_decref(&root);
            free(*vec);
            *vec = NULL;
            return -1;
        }
        (*vec)[i] = cbor_float_get_float4(f);
    }
    cbor_item_t *n_item = cbor_array_handle(root)[1];
    if (!cbor_isa_uint(n_item)) {
        cbor_decref(&root);
        free(*vec);
        *vec = NULL;
        return -1;
    }
    *n = (int)cbor_get_uint32(n_item);

    cbor_decref(&root);
    return 0;
}

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
int buffer_write_lookup_result(buffer_t *buf, const uint64_t *ids, const uint8_t **payloads, const size_t *payload_lens, size_t n) {
    cbor_item_t *root = cbor_new_definite_array(n);
    for (size_t i = 0; i < n; i++) {
        cbor_item_t *pair = cbor_new_definite_array(2);
        cbor_item_t *id_item = cbor_build_uint64(ids[i]);
        (void)cbor_array_push(pair, id_item);
        cbor_decref(&id_item);

        cbor_item_t *payload_item = cbor_build_bytestring(payloads[i], payload_lens[i]);
        (void)cbor_array_push(pair, payload_item);
        cbor_decref(&payload_item);

        (void)cbor_array_push(root, pair);
        cbor_decref(&pair);
    }
    size_t written = cbor_serialize(root, buf->data, MSG_MAXLEN);
    if (written == 0 || written > MSG_MAXLEN) {
        cbor_decref(&root);
        return -1;
    }
    buf->hdr.len = (int)written;
    buf->hdr.type = MSG_LOOKUP_RESULT;

    cbor_decref(&root);
    return 0;
}

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
int buffer_read_lookup_result(const buffer_t *buf, uint64_t *ids, uint8_t **payloads, size_t *payload_lens, size_t n, size_t *out_count) {
    struct cbor_load_result result;
    cbor_item_t *root = cbor_load(buf->data, buf->hdr.len, &result);
    if (!root || !cbor_isa_array(root)) {
        if (root) cbor_decref(&root);
        return -1;
    }
    size_t count = cbor_array_size(root);
    if (count > n) count = n;
    for (size_t i = 0; i < count; i++) {
        cbor_item_t *pair = cbor_array_handle(root)[i];
        if (!cbor_isa_array(pair) || cbor_array_size(pair) != 2) {
            cbor_decref(&root);
            return -1;
        }
        cbor_item_t *id_item = cbor_array_handle(pair)[0];
        if (!cbor_isa_uint(id_item)) {
            cbor_decref(&root);
            return -1;
        }
        ids[i] = cbor_get_uint64(id_item);

        cbor_item_t *payload_item = cbor_array_handle(pair)[1];
        if (!cbor_isa_bytestring(payload_item)) {
            cbor_decref(&root);
            return -1;
        }
        payload_lens[i] = cbor_bytestring_length(payload_item);
        payloads[i] = malloc(payload_lens[i]);
        if (!payloads[i]) {
            cbor_decref(&root);
            return -1;
        }
        memcpy(payloads[i], cbor_bytestring_handle(payload_item), payload_lens[i]);
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
int buffer_write_del(buffer_t *buf, uint64_t id) {
    cbor_item_t *root = cbor_new_definite_array(1);
    cbor_item_t *id_item = cbor_build_uint64(id);
    (void)cbor_array_push(root, id_item);
    cbor_decref(&id_item);

    size_t written = cbor_serialize(root, buf->data, MSG_MAXLEN);
    if (written == 0 || written > MSG_MAXLEN) {
        cbor_decref(&root);
        return -1;
    }
    buf->hdr.len = (int)written;
    buf->hdr.type = 0; // Asigna el tipo de mensaje DEL según tu protocolo

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
int buffer_read_del(const buffer_t *buf, uint64_t *id) {
    struct cbor_load_result result;
    cbor_item_t *root = cbor_load(buf->data, buf->hdr.len, &result);
    if (!root || !cbor_isa_array(root) || cbor_array_size(root) != 1) {
        if (root) cbor_decref(&root);
        return -1;
    }
    cbor_item_t *id_item = cbor_array_handle(root)[0];
    if (!cbor_isa_uint(id_item)) {
        cbor_decref(&root);
        return -1;
    }
    *id = cbor_get_uint64(id_item);
    cbor_decref(&root);
    return 0;
}

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
int buffer_write_del_result(buffer_t *buf, bool ok) {
    cbor_item_t *root = cbor_new_definite_array(1);
    cbor_item_t *b = cbor_build_bool(ok);
    (void)cbor_array_push(root, b);
    cbor_decref(&b);

    size_t written = cbor_serialize(root, buf->data, MSG_MAXLEN);
    if (written == 0 || written > MSG_MAXLEN) {
        cbor_decref(&root);
        return -1;
    }
    buf->hdr.len = (int)written;
    buf->hdr.type = 0; // Asigna el tipo de mensaje DEL_RESULT según tu protocolo

    cbor_decref(&root);
    return 0;
}

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
int buffer_read_del_result(const buffer_t *buf, bool *ok) {
    struct cbor_load_result result;
    cbor_item_t *root = cbor_load(buf->data, buf->hdr.len, &result);
    if (!root || !cbor_isa_array(root) || cbor_array_size(root) != 1) {
        if (root) cbor_decref(&root);
        return -1;
    }
    cbor_item_t *b = cbor_array_handle(root)[0];
    if (!cbor_is_bool(b)) {
        cbor_decref(&root);
        return -1;
    }
    *ok = cbor_get_bool(b);
    cbor_decref(&root);
    return 0;
}