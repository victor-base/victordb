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
#include <cbor.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Master function for writing key-value operations.
 *
 * This function handles PUT and DEL operations by encoding different CBOR arrays:
 * - PUT: [key, value] 
 * - DEL: [key]
 *
 * @param buf Output buffer where the CBOR message will be written
 * @param msg_type Message type (MSG_PUT, MSG_DEL, etc.)
 * @param key Pointer to the key data
 * @param klen Length of the key
 * @param val Pointer to the value data (can be NULL for DEL operations)
 * @param vlen Length of the value (ignored if val is NULL)
 * @return 0 on success, -1 on error
 */
static int buffer_write_kv_master(buffer_t *buf, int msg_type, void *key, size_t klen, void *val, size_t vlen) {
    int array_size = val ? 2 : 1;  // PUT has 2 elements, DEL has 1
    cbor_item_t *root = cbor_new_definite_array(array_size);
    
    // Add key
    cbor_item_t *key_item = cbor_build_bytestring(key, klen);
    (void)cbor_array_push(root, key_item);
    cbor_decref(&key_item);
    
    // Add value if present (for PUT operations)
    if (val) {
        cbor_item_t *val_item = cbor_build_bytestring(val, vlen);
        (void)cbor_array_push(root, val_item);
        cbor_decref(&val_item);
    }
    
    size_t written = cbor_serialize(root, buf->data, MSG_MAXLEN);
    if (written == 0 || written > MSG_MAXLEN) {
        cbor_decref(&root);
        return -1;
    }
    
    buf->hdr.len = (int)written;
    buf->hdr.type = msg_type;
    cbor_decref(&root);
    return 0;
}

/**
 * @brief Master function for reading key-value operations.
 *
 * This function handles parsing of PUT, DEL, and GET operations.
 * Memory is allocated for key and value (if present) and must be freed by caller.
 *
 * @param buf Input buffer containing the CBOR message
 * @param key Output pointer to allocated key data
 * @param klen Output pointer to key length
 * @param val Output pointer to allocated value data (can be NULL for single-element arrays)
 * @param vlen Output pointer to value length (can be NULL for single-element arrays)
 * @return 0 on success, -1 on error
 */
static int buffer_read_kv_master(const buffer_t *buf, void **key, size_t *klen, void **val, size_t *vlen) {
    struct cbor_load_result result;
    cbor_item_t *root = cbor_load(buf->data, buf->hdr.len, &result);
    
    if (!root || !cbor_isa_array(root)) {
        if (root) cbor_decref(&root);
        return -1;
    }
    
    size_t array_size = cbor_array_size(root);
    if (array_size < 1 || array_size > 2) {
        cbor_decref(&root);
        return -1;
    }
    
    // Extract key
    cbor_item_t *key_item = cbor_array_handle(root)[0];
    if (!cbor_isa_bytestring(key_item)) {
        cbor_decref(&root);
        return -1;
    }
    
    *klen = cbor_bytestring_length(key_item);
    *key = malloc(*klen);
    if (!*key) {
        cbor_decref(&root);
        return -1;
    }
    memcpy(*key, cbor_bytestring_handle(key_item), *klen);
    
    // Extract value if present
    if (array_size == 2 && val && vlen) {
        cbor_item_t *val_item = cbor_array_handle(root)[1];
        if (!cbor_isa_bytestring(val_item)) {
            cbor_decref(&root);
            free(*key);
            return -1;
        }
        
        *vlen = cbor_bytestring_length(val_item);
        *val = malloc(*vlen);
        if (!*val) {
            cbor_decref(&root);
            free(*key);
            return -1;
        }
        memcpy(*val, cbor_bytestring_handle(val_item), *vlen);
    } else if (val) {
        *val = NULL;
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
    return buffer_write_kv_master(buf, MSG_GET_RESULT, value, vlen, NULL, 0);
}

int buffer_read_get_result(buffer_t *buf, void **value, size_t *vlen) {
    return buffer_read_kv_master(buf, value, vlen, NULL, NULL);
}
