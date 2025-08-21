/**
 * @file table_server.h
 * @brief Header for VictorTable key-value server functionality.
 *
 * This header defines the interface for the VictorTable server that handles
 * key-value operations including PUT, GET, and DELETE operations.
 */

#ifndef __VICTOR_TABLE_SERVER_H
#define __VICTOR_TABLE_SERVER_H

#include <victor/victor.h>
#include <victor/victorkv.h>
#include <stdio.h>
#include <stdint.h>


/**
 * @brief VictorTable structure for key-value operations.
 * 
 * This structure contains the key-value table and operation counters
 * for managing the table server state.
 */
typedef struct {
    char     *name;           /**< Table name identifier */
    KVTable  *table;          /**< Pointer to the key-value table */
    int op_add_counter;  /**< Counter for PUT operations */
    int op_del_counter;  /**< Counter for DELETE operations */
} VictorTable;


/**
 * @brief Loads and applies WAL operations to the VictorTable database.
 *
 * @param core Pointer to the VictorTable database structure.
 * @param wal  FILE* pointer to an opened WAL file in binary read mode.
 * @return 0 on success, -1 on failure.
 */
extern int victor_table_loadwal(VictorTable *core, FILE *wal);

/**
 * @brief Starts the VictorTable server loop.
 *
 * @param core Pointer to the VictorTable database structure.
 * @param server File descriptor of a bound and listening UNIX socket.
 * @return 0 on clean shutdown, -1 on failure.
 */
extern int victor_table_server(VictorTable *core, int server);


#endif /* __TABLE_SERVER_H */