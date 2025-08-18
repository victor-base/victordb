#include <victor/victor.h>
#include <victor/victorkv.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "fileutils.h"
#include "kvproto.h"
#include "protocol.h"
#include "socket.h"
#include "buffer.h"
#include "table_server.h"
#include "server.h"
#include "log.h"


/**
 * @brief Handles a DEL (key deletion) message.
 *
 * This function proces        }
        if (core->op_add_counter + core->op_del_counter > get_export_threshold()) {
            int ret;
            log_message(LOG_INFO, "Exporting table to disk (operations: %d)", 
                       core->op_add_counter + core->op_del_counter);n incoming `MSG_DEL` message, parses the key to delete,
 * and attempts to remove it from the key-value store.
 *
 * Workflow:
 * 1. Parse the message with `buffer_read_del()` to extract the key.
 * 2. Attempt to delete the key using `kv_del()`.
 * 3. If operation fails, log the error and respond with an error message.
 * 4. If successful, increment the delete operation counter and log to WAL.
 *
 * @param core Pointer to the VictorTable database context.
 * @param msg  Pointer to the input/output message buffer.
 * @param wal  FILE pointer to the Write-Ahead Log (can be NULL).
 *
 * @return 0 on success, -1 on failure, or error code response written into the message buffer.
 */
static int handle_del_message(VictorTable *core, buffer_t *msg, FILE *wal) {
    void   *key = NULL;
    size_t klen;
    int ret;

    if (buffer_read_del(msg, &key, &klen) == -1) {
        log_message(LOG_ERROR, "Failed to parse DELETE message");
        return -1;
    }
        
    if ((ret = kv_del(core->table, key, (int)klen)) != KV_SUCCESS) {
        log_message(LOG_ERROR, 
            "Unable to delete key from table: %s",
            table_strerror(ret)
        );
    } else {
        core->op_del_counter++;
        if (wal && buffer_dump_wal(msg, wal) != 0)
            log_message(LOG_WARNING,
                "writing wal (%d) - message: %s",
                errno, strerror(errno)
            );
    }
    
    if (key) free(key);
    return buffer_write_op_result(msg, MSG_OP_RESULT, ret, table_strerror(ret));
}

/**
 * @brief Handles a PUT (key-value insertion) message.
 *
 * This function processes an incoming `MSG_PUT` message, extracts the key and value,
 * and stores them in the key-value store.
 *
 * Workflow:
 * 1. Parse the message using `buffer_read_put()` to extract the key and value.
 * 2. Insert the key-value pair into the table using `kv_put()`.
 * 3. If successful, log the operation to WAL and increment the counter.
 * 4. On success, respond with operation result via `buffer_write_op_result()`.
 * 5. On any failure, send an appropriate error response.
 *
 * @param core Pointer to the VictorTable database context.
 * @param msg  Pointer to the input/output message buffer.
 * @param wal  FILE pointer to the Write-Ahead Log (can be NULL).
 *
 * @return 0 on success, -1 on failure.
 *
 * @note The function dynamically allocates memory for key and value, which
 *       are freed before returning.
 */
static int handle_put_message(VictorTable *core, buffer_t *msg, FILE *wal) {
    void   *key = NULL, *val = NULL;
    size_t klen, vlen;
    int ret;

    ret = buffer_read_put(msg, &key, &klen, &val, &vlen); 
    if (ret == -1) {
        log_message(LOG_ERROR, "Failed to parse PUT message");
        return -1;
    }

    if ((ret = kv_put(core->table, key, (int)klen, val, (int)vlen)) != KV_SUCCESS) {
        if (ret == SYSTEM_ERROR)
            log_message(LOG_ERROR, 
                "System error during key-value insert - code: %d - message: %s", 
                ret, table_strerror(ret)
            );
        else
            log_message(LOG_WARNING, 
                "at key-value insert - code: %d - message: %s", 
                ret, table_strerror(ret)
            );
        goto cleanup;
    }

    if (wal && buffer_dump_wal(msg, wal) != 0)
        log_message(LOG_WARNING,
            "writing wal (%d) - message: %s",
            errno, strerror(errno)
        );

    core->op_add_counter++;
cleanup:
    if (key) free(key);
    if (val) free(val);
    return buffer_write_op_result(msg, MSG_OP_RESULT, ret, table_strerror(ret));
}

/**
 * @brief Handles a GET (key lookup) message.
 *
 * This function processes an incoming `MSG_GET` message, extracts the key,
 * performs a lookup in the key-value store, and returns the associated value.
 *
 * Workflow:
 * 1. Parse the message and extract the key via `buffer_read_get()`.
 * 2. Execute `kv_get()` on the table to retrieve the value.
 * 3. If found, write the value back using `buffer_write_get_result()`.
 * 4. On any error, write an error message with `buffer_write_op_result()`.
 *
 * @param core Pointer to the VictorTable database structure.
 * @param msg  Pointer to the input/output buffer containing the message.
 *
 * @return 0 on success, -1 on failure.
 *
 * @note The function allocates memory for the key, which is freed before returning.
 *
 * @warning If the key is not found, an appropriate error response is sent to the client.
 */
static int handle_get_message(VictorTable *core, buffer_t *msg) {
    void *key = NULL;
    void *val = NULL;
    size_t klen;
    int vlen;  // Changed from size_t to int to match kv_get signature
    int ret;

    if (buffer_read_get(msg, &key, &klen) == -1) {
        log_message(LOG_ERROR, "Failed to parse GET message");
        return -1;
    }

    ret = kv_get(core->table, key, (int)klen, &val, &vlen);
    if (ret == KV_SUCCESS && val != NULL && vlen > 0) {
        ret = buffer_write_get_result(msg, val, (size_t)vlen);
    } else {
        if (ret == KV_SUCCESS && (val == NULL || vlen <= 0)) {
            // Key exists but value is NULL or invalid - treat as not found
            ret = KV_KEY_NOT_FOUND;
        }
        ret = buffer_write_op_result(msg, MSG_ERROR, ret, table_strerror(ret));
    }
    
    if (key) free(key);
    return ret;
}


/**
 * @brief Loads and applies WAL operations to the VictorTable database.
 *
 * This function reads and replays all operations stored in a Write-Ahead Log (WAL) file.
 * Each message is parsed and executed according to its type (e.g., `MSG_PUT`, `MSG_DEL`).
 * It ensures database state restoration after a crash or restart.
 *
 * @param core Pointer to the VictorTable database structure to apply the WAL to.
 * @param wal  FILE* pointer to an already opened WAL file in binary read mode.
 *
 * @return 
 *   0 on successful import of all WAL entries,  
 *  -1 on failure due to I/O error or corrupted WAL content.
 *
 * @note 
 *   If the WAL contains unknown message types, they will be skipped with a warning.
 *   If an error occurs and `errno` is 0, it is assumed the WAL is corrupted.
 *   If `errno` is non-zero, a system-level I/O error is assumed.
 */
int victor_table_loadwal(VictorTable *core, FILE *wal) {
    buffer_t *buff = alloc_buffer();
    int successful_entries = 0, failed_entries = 0;
    int ret;

    if (!buff) {
        log_message(LOG_ERROR, "Failed to allocate buffer for WAL processing");
        return -1;
    }

    while ((ret = buffer_load_wal(buff, wal)) == 1) {
        switch (buff->hdr.type) {
            case MSG_PUT:
                if (handle_put_message(core, buff, NULL) != 0 || 
                    buff->hdr.type == MSG_ERROR)
                    failed_entries++;
                else
                    successful_entries++;

                break;
            case MSG_DEL:
                if (handle_del_message(core, buff, NULL) != 0 || 
                    buff->hdr.type == MSG_ERROR)
                    failed_entries++;
                else
                    successful_entries++;
                break;
            default:
                log_message(LOG_WARNING,
                    "unknown message type in WAL: %d", buff->hdr.type);
                break;
        }
    }

    free(buff);

    if (ret == 0) {
        log_message(LOG_INFO, 
            "WAL import completed: %d entries loaded successfully, %d with errors", 
            successful_entries, failed_entries);
        return 0;
    }

    if (errno == 0)
        log_message(LOG_ERROR, "WAL corruption detected during import");
    else
        log_message(LOG_ERROR, "I/O error during WAL import: %s", strerror(errno));

    return -1;
}


/**
 * @brief Starts the VictorTable server loop, handling client requests and writing to WAL.
 *
 * This function implements the main server loop for the VictorTable key-value database. 
 * It accepts incoming UNIX domain socket connections, processes protocol messages from clients,
 * and writes changes to a Write-Ahead Log (WAL) file for durability.
 *
 * Supported message types:
 * - `MSG_PUT`: Adds a new key-value pair to the database and appends to the WAL.
 * - `MSG_DEL`: Removes a key-value pair and appends to the WAL.
 * - `MSG_GET`: Performs a key lookup (no WAL entry).
 *
 * The server uses `select()` to multiplex connections and listens until a termination
 * signal is received (`running == 0`), at which point it gracefully shuts down.
 *
 * @param core Pointer to the VictorTable database structure.
 * @param server File descriptor of a bound and listening UNIX socket.
 *
 * @return 0 on clean shutdown, -1 on failure (e.g., memory, I/O, or socket error).
 *
 * @note The WAL file is opened in append mode. If it cannot be opened, the server
 *       fails to start. The server respects signals via the global `running` flag.
 *
 * @warning Only `MSG_PUT` and `MSG_DEL` are persisted in the WAL.
 * @warning Maximum number of simultaneous connections is limited by `MAX_CONNECTIONS`.
 * @warning If `recv_msg` or `send_msg` fail, the client connection is closed.
 */
int victor_table_server(VictorTable *core, int server) {
    FILE *wal = NULL;
    int conn[MAX_CONNECTIONS];
    fd_set set, check;
    int max = server, n;
    buffer_t *buff = alloc_buffer();
    
    if (!buff) {
        log_message(LOG_ERROR, 
            "failed to allocate buffer"
        );
        close(server);
        return -1;
    }

    wal = fopen(TWAL_FILE, "ab");
    if (!wal) { 
        log_message(LOG_ERROR, 
            "failed to open WAL file '%s': %s", 
            TWAL_FILE, 
            strerror(errno)
        );
        free(buff);
        close(server);
        return -1;
    }
    for (int i=0; i < MAX_CONNECTIONS; i++) conn[i] = -1;

    FD_ZERO(&set);
    FD_ZERO(&check);
    FD_SET(server, &set);

    while (running) {
        memcpy(&check, &set, sizeof(fd_set));
        n = select(max+1, &check, NULL, NULL, NULL);
        if (n < 0 && (errno == EAGAIN || errno == EINTR))
            continue;
        else if (n < 0) {
            log_message(LOG_ERROR, 
                "fatal error on select (%d) - %s",
                errno, strerror(errno)
            );
            break;
        }
        if (FD_ISSET(server, &check)) {
            int i;
            int sd = unix_accept(server);
            if (sd == -1)
                if (errno != EAGAIN && errno != EINTR) {
                    log_message(LOG_ERROR, 
                        "fatal error on unix_accept (%d) - %s",
                        errno, strerror(errno)
                    );
                    break;
                }
            for (i = 0; i < MAX_CONNECTIONS; i++)
                if (conn[i] == -1) {
                    conn[i] = sd;
                    break;
                }
            if (i == MAX_CONNECTIONS) {
                log_message(LOG_WARNING,
                    "max connections reached - new client closed"
                );
                close(sd);
            } else {
                max = sd > max ? sd : max;
                FD_SET(sd, &set);
            }
            n--;
        }

        for (int i = 0; i < MAX_CONNECTIONS && n > 0; i++) {
            int ret = 0;
            if (FD_ISSET(conn[i], &check)) {
                n--;
                ret = recv_msg(conn[i], buff);
                if (ret == -1) {
                    log_message(LOG_WARNING,
                        "connection closed due to protocol or receive error"
                    );
                    FD_CLR(conn[i], &set);
                    close(conn[i]);
                    conn[i] = -1;
                    continue;
                }
                switch (buff->hdr.type) {
                case MSG_PUT: 
                    ret = handle_put_message(core, buff, wal);   
                    break;
                case MSG_DEL:
                    ret = handle_del_message(core, buff, wal);
                    break;
                case MSG_GET:
                    ret = handle_get_message(core, buff);
                    break;
                default:
                    log_message(LOG_WARNING,
                        "invalid protocol message type: %d",
                        buff->hdr.type
                    );
                    ret = -1;
                }
                if (ret != -1) {
                    if (send_msg(conn[i], buff) == -1) {
                        FD_CLR(conn[i], &set);
                        close(conn[i]);
                        conn[i] = -1;
                    }
                } else {
                    FD_CLR(conn[i], &set);
                    close(conn[i]);
                    conn[i] = -1;
                }
            }
        }
        if (core->op_add_counter + core->op_del_counter > get_export_threshold()) {
            int ret;
            log_message(LOG_INFO, "Exporting table to disk (operations: %d)", 
                       core->op_add_counter + core->op_del_counter);
            if ((ret = kv_dump(core->table, TABLE_FILE)) != KV_SUCCESS)
                log_message(LOG_WARNING, 
                    "Error during table export: %s", table_strerror(ret));
            else {
                log_message(LOG_INFO, "Table exported successfully, WAL file cleared");
                unlink(TWAL_FILE);
                core->op_add_counter = core->op_del_counter = 0;
            }
        }
    }
    log_message(LOG_INFO, "end main loop");
    fclose(wal);
    free(buff);
    for (int i = 0; i < MAX_CONNECTIONS; i ++)
        if (conn[i] != -1)
            close(conn[i]);
    close(server);
    return 0;
}