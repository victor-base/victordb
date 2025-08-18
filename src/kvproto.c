/**
 * @file kvproto.c
 * @brief Key-Value protocol functions for CBOR message serialization/deserialization.
 *
 * This file contains functions for handling key-value operations including
 * PUT, GET, DELETE, and GET_RESULT message types. It uses master functions
 * to reduce code duplication.
 */

#include "buffer.h"
#include "kvproto.h"
#include "panic.h"
#include <cbor.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Master function for writing key-value operations.
 *
 * This function handles PUT and DEL operations by encoding different CBOR arrays:
 * - PUT: [key, value] 
 * - DEL: [key]
 * - GET_RESULT: [value]
 *
 * @param buf Output buffer where the CBOR message will be written
 * @param msg_type Message type (MSG_PUT, MSG_DEL, MSG_GET_RESULT, etc.)
 * @param key Pointer to the key data
 * @param klen Length of the key
 * @param val Pointer to the value data (can be NULL for DEL operations)
 * @param vlen Length of the value (ignored if val is NULL)
 * @return 0 on success, -1 on error
 */
static int buffer_write_kv_master(buffer_t *buf, int msg_type, void *key, size_t klen, void *val, size_t vlen) {
    cbor_item_t *root = NULL;
    cbor_item_t *item = NULL;
    int ret = -1;

    PANIC_IF(!buf, "buffer cannot be null");
    PANIC_IF(!buf->data, "buffer data cannot be null");
    
    // For GET_RESULT, we only send the value (not key-value pair)
    if (msg_type == MSG_GET_RESULT) {
        root = cbor_new_definite_array(1);
        if (!root)
            return -1;

        if (val && vlen > 0) {
            item = cbor_build_bytestring(val, vlen);
        } else {
            // Send empty byte string for NULL value
            item = cbor_build_bytestring((const unsigned char *)"", 0);
        }
        if (!item) {
            ret = -1;
            goto cleanup;
        }
        bool success = cbor_array_push(root, item);
        cbor_decref(&item);
        item = NULL;
        if (!success) {
            ret = -1;
            goto cleanup;
        }
    } else {
        // For PUT and DEL operations
        int array_size = val ? 2 : 1;  // PUT has 2 elements, DEL has 1
        root = cbor_new_definite_array(array_size);
        if (!root)
            return -1;
        // Add key (must not be NULL for PUT/DEL)
        if (!key) {
            cbor_decref(&root);
            return -1;
        }

        item = cbor_build_bytestring(key, klen);
        if (!item) {
            ret = -1;
            goto cleanup;
        }
        bool success = cbor_array_push(root, item);
        cbor_decref(&item);
        if (!success) {
            ret = -1;
            goto cleanup;
        }
        // Add value if present (for PUT operations)
        if (val) {
            item = cbor_build_bytestring(val, vlen);
            if (!item) {
                ret = -1;
                goto cleanup;
            }
            success = cbor_array_push(root, item);
            cbor_decref(&item);
            item = NULL;
            if (!success) {
                ret = -1;
                goto cleanup;
            }
        }
    }
    
    size_t written = cbor_serialize(root, buf->data, MSG_MAXLEN);
    if (written == 0 || written > MSG_MAXLEN) {
        ret = -1;
        goto cleanup;
    }


    buf->hdr.len = (int)written;
    buf->hdr.type = msg_type;
    ret = 0;
cleanup:
    if (root) cbor_decref(&root);
    return ret;
}

/**
 * @brief Master function for reading key-value operations.
 *
 * This function handles parsing of PUT, DEL, GET, and GET_RESULT operations.
 * Memory is allocated for key and value (if present) and must be freed by caller.
 *
 * @param buf Input buffer containing the CBOR message
 * @param key Output pointer to allocated key data (can be NULL if not needed)
 * @param klen Output pointer to key length (can be NULL if not needed)
 * @param val Output pointer to allocated value data (can be NULL if not needed)
 * @param vlen Output pointer to value length (can be NULL if not needed)
 * @return 0 on success, -1 on error
 */
static int buffer_read_kv_master(const buffer_t *buf, void **key, size_t *klen, void **val, size_t *vlen) {
    struct cbor_load_result result;
    cbor_item_t *root = NULL;

    PANIC_IF(!buf, "buffer cannot be null");
    PANIC_IF(!buf->data, "buffer data cannot be null");

    if (buf->hdr.len == 0 || buf->hdr.len > MSG_MAXLEN) {
        return -1;
    }

    root = cbor_load(buf->data, buf->hdr.len, &result);
    if (!root || !cbor_isa_array(root)) {
        if (root) cbor_decref(&root);
        return -1;
    }

    size_t array_size = cbor_array_size(root);
    if (array_size < 1 || array_size > 2) {
        cbor_decref(&root);
        return -1;
    }

    // Extract key (if requested and available)
    if (key && klen && array_size >= 1) {
        cbor_item_t *key_item = cbor_array_handle(root)[0];
        if (!cbor_isa_bytestring(key_item)) {
            cbor_decref(&root);
            return -1;
        }

        *klen = cbor_bytestring_length(key_item);
        if (*klen == 0) {
            *key = NULL;
        } else {
            *key = malloc(*klen);
            if (!*key) {
                cbor_decref(&root);
                return -1;
            }
            memcpy(*key, cbor_bytestring_handle(key_item), *klen);
        }
    }

    // Extract value
    if (val && vlen) {
        cbor_item_t *val_item = NULL;

        if (array_size == 2) {
            // For PUT messages: [key, value]
            val_item = cbor_array_handle(root)[1];
        } else if (array_size == 1) {
            // For GET_RESULT messages: [value]
            val_item = cbor_array_handle(root)[0];
        }

        if (val_item && cbor_isa_bytestring(val_item)) {
            *vlen = cbor_bytestring_length(val_item);
            if (*vlen == 0) {
                *val = NULL;
            } else {
                *val = malloc(*vlen);
                if (!*val) {
                    if (key && *key) free(*key);
                    cbor_decref(&root);
                    return -1;
                }
                memcpy(*val, cbor_bytestring_handle(val_item), *vlen);
            }
        } else {
            *val = NULL;
            *vlen = 0;
        }
    }

    cbor_decref(&root);
    return 0;
}

// Wrapper functions for key-value operations
int buffer_write_put(buffer_t *buf, void *key, size_t klen, void *val, size_t vlen) {
    return buffer_write_kv_master(buf, MSG_PUT, key, klen, val, vlen);
}

int buffer_read_put(buffer_t *buf, void **key, size_t *klen, void **val, size_t *vlen) {
    return buffer_read_kv_master(buf, key, klen, val, vlen);
}

int buffer_write_del(buffer_t *buf, void *key, size_t klen) {
    return buffer_write_kv_master(buf, MSG_DEL, key, klen, NULL, 0);
}

int buffer_read_del(buffer_t *buf, void **key, size_t *klen) {
    return buffer_read_kv_master(buf, key, klen, NULL, NULL);
}

int buffer_write_get(buffer_t *buf, void *key, size_t klen) {
    return buffer_write_kv_master(buf, MSG_GET, key, klen, NULL, 0);
}

int buffer_read_get(buffer_t *buf, void **key, size_t *klen) {
    return buffer_read_kv_master(buf, key, klen, NULL, NULL);
}

int buffer_write_get_result(buffer_t *buf, void *value, size_t vlen) {
    return buffer_write_kv_master(buf, MSG_GET_RESULT, NULL, 0, value, vlen);
}

int buffer_read_get_result(buffer_t *buf, void **value, size_t *vlen) {
    return buffer_read_kv_master(buf, NULL, NULL, value, vlen);
}
