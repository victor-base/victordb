#include "buffer.h"
#include "protocol.h"
#include <cbor.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "panic.h"

/**
 * @brief Serializes an operation result response into a CBOR-encoded buffer.
 *
 * Encodes a CBOR array of the form:
 *     [error_code:int, error_message:string]
 *
 * @param buf Output buffer where the CBOR message will be written.
 * @param msg_type The message type (MSG_INSERT_RESULT, MSG_DEL_RESULT, etc.).
 * @param code Error code (0 for success, non-zero for error).
 * @param msg Error message string (can be NULL for success).
 * @return 0 on success, -1 on error or insufficient buffer space.
 */
int buffer_write_op_result(buffer_t *buf, int msg_type, int code, const char *msg) {
    cbor_item_t *root = NULL;
    cbor_item_t *c = NULL;
    cbor_item_t *s = NULL;
    size_t written;

    if (!buf || !buf->data) return -1;

    root = cbor_new_definite_array(2);
    if (!root) return -1;

    c = cbor_build_uint32((uint32_t)code);
    if (!c) {
        cbor_decref(&root);
        return -1;
    }
    if (!cbor_array_push(root, c)) {
        cbor_decref(&c);
        cbor_decref(&root);
        return -1;
    }
    cbor_decref(&c);

    s = cbor_build_string(msg ? msg : "");
    if (!s) {
        cbor_decref(&root);
        return -1;
    }
    if (!cbor_array_push(root, s)) {
        cbor_decref(&s);
        cbor_decref(&root);
        return -1;
    }
    cbor_decref(&s);

    written = cbor_serialize(root, buf->data, MSG_MAXLEN);
    if (written == 0 || written > MSG_MAXLEN) {
        cbor_decref(&root);
        return -1;
    }

    if (written > (size_t)INT_MAX) {
        cbor_decref(&root);
        return -1;
    }

    buf->hdr.len = (int)written;
    buf->hdr.type = msg_type;

    cbor_decref(&root);
    return 0;
}

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
int buffer_read_op_result(const buffer_t *buf, int *code, char **msg) {
    struct cbor_load_result result;
    cbor_item_t *root = NULL;
    cbor_item_t *c = NULL;
    cbor_item_t *s = NULL;

    if (!buf || !buf->data || buf->hdr.len == 0 || buf->hdr.len > MSG_MAXLEN) return -1;

    root = cbor_load(buf->data, buf->hdr.len, &result);
    if (!root || !cbor_isa_array(root) || cbor_array_size(root) != 2) {
        if (root) cbor_decref(&root);
        return -1;
    }

    c = cbor_array_handle(root)[0];
    if (!c || !cbor_isa_uint(c)) {
        cbor_decref(&root);
        return -1;
    }
    *code = (int)cbor_get_uint32(c);

    s = cbor_array_handle(root)[1];
    if (!s || !cbor_isa_string(s)) {
        cbor_decref(&root);
        return -1;
    }

    size_t len = cbor_string_length(s);
    /* allocate len+1 even for empty string to keep contract that caller frees */
    *msg = malloc(len + 1);
    if (!*msg) {
        cbor_decref(&root);
        return -1;
    }
    if (len > 0) memcpy(*msg, cbor_string_handle(s), len);
    (*msg)[len] = '\0';

    cbor_decref(&root);
    return 0;
}
