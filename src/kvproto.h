/**
 * @file kvproto.h
 * @brief Key-Value protocol function declarations.
 *
 * This header defines the interface for key-value protocol operations
 * including PUT, GET, DELETE, and GET_RESULT message handling.
 */

#ifndef __KVPROTO_H
#define __KVPROTO_H

#include "buffer.h"
#include "protocol.h"
#include <stddef.h>

/**
 * @brief Write a PUT message to buffer.
 * @param buf Output buffer
 * @param key Key data
 * @param klen Key length
 * @param val Value data
 * @param vlen Value length
 * @return 0 on success, -1 on error
 */
int buffer_write_put(buffer_t *buf, void *key, size_t klen, void *val, size_t vlen);

/**
 * @brief Read a PUT message from buffer.
 * @param buf Input buffer
 * @param key Output key data (allocated)
 * @param klen Output key length
 * @param val Output value data (allocated)
 * @param vlen Output value length
 * @return 0 on success, -1 on error
 */
int buffer_read_put(buffer_t *buf, void **key, size_t *klen, void **val, size_t *vlen);

/**
 * @brief Write a DELETE message to buffer.
 * @param buf Output buffer
 * @param key Key data
 * @param klen Key length
 * @return 0 on success, -1 on error
 */
int buffer_write_del(buffer_t *buf, void *key, size_t klen);

/**
 * @brief Read a DELETE message from buffer.
 * @param buf Input buffer
 * @param key Output key data (allocated)
 * @param klen Output key length
 * @return 0 on success, -1 on error
 */
int buffer_read_del(buffer_t *buf, void **key, size_t *klen);

/**
 * @brief Write a GET message to buffer.
 * @param buf Output buffer
 * @param key Key data
 * @param klen Key length
 * @return 0 on success, -1 on error
 */
int buffer_write_get(buffer_t *buf, void *key, size_t klen);

/**
 * @brief Read a GET message from buffer.
 * @param buf Input buffer
 * @param key Output key data (allocated)
 * @param klen Output key length
 * @return 0 on success, -1 on error
 */
int buffer_read_get(buffer_t *buf, void **key, size_t *klen);

/**
 * @brief Write a GET_RESULT message to buffer.
 * @param buf Output buffer
 * @param value Value data
 * @param vlen Value length
 * @return 0 on success, -1 on error
 */
int buffer_write_get_result(buffer_t *buf, void *value, size_t vlen);

/**
 * @brief Read a GET_RESULT message from buffer.
 * @param buf Input buffer
 * @param value Output value data (allocated)
 * @param vlen Output value length
 * @return 0 on success, -1 on error
 */
int buffer_read_get_result(buffer_t *buf, void **value, size_t *vlen);

#endif /* __KVPROTO_H */
