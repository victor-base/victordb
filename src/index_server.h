/**
 * @file index_server.h
 * @brief Vector index server definitions and structures.
 *
 * This header defines the VictorIndex structure and server functions
 * for handling vector database operations including INSERT, SEARCH, and DELETE.
 */

#ifndef __VICTOR_INDEX_SERVER
#define __VICTOR_INDEX_SERVER

#include <victor/victor.h>

/**
 * @brief Vector index database context structure.
 *
 * This structure holds the complete state of a vector index database instance,
 * including the database name, index handle, and operation counters for
 * Write-Ahead Log management.
 */
typedef struct {
    /** @brief Database instance name */
    char     *name;
    
    /** @brief Pointer to the vector index structure */
    Index    *index;
    
    /** @brief Counter for INSERT operations since last export */
    int op_add_counter;
    
    /** @brief Counter for DELETE operations since last export */
    int op_del_counter;
} VictorIndex;

/**
 * @brief Starts the vector index server loop.
 *
 * Implements the main server loop for the vector index database, handling
 * client connections, processing protocol messages (INSERT/SEARCH/DELETE),
 * and managing Write-Ahead Log persistence.
 *
 * @param core Pointer to the VictorIndex database context.
 * @param server File descriptor of a bound and listening socket.
 * @return 0 on clean shutdown, -1 on failure.
 */
extern int victor_index_server(VictorIndex *core, int server);

/**
 * @brief Loads and replays Write-Ahead Log operations.
 *
 * Reads a WAL file and replays all recorded operations to restore
 * the database state after a restart or crash recovery.
 *
 * @param core Pointer to the VictorIndex database context.
 * @param wal FILE pointer to an opened WAL file in read mode.
 * @return 0 on success, -1 on failure or corruption.
 */
extern int victor_index_loadwal(VictorIndex *core, FILE *wal);

#endif /* __VICTOR_SERVER */