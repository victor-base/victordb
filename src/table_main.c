/**
 * @file table_main.c
 * @brief Entry point for the VictorDB table (key-value) server.
 *
 * This file contains the main function that initializes the VictorDB table server,
 * loads configuration, imports existing data, and starts the server loop.
 */

#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include "fileutils.h"
#include "socket.h"
#include "table_server.h"
#include "server.h"
#include "opt.h"
#include "log.h"

/**
 * @brief Entry point for the VictorDB table (key-value) server.
 *
 * Initializes the table database environment, loads the configuration and WAL,
 * registers signal handlers, and starts the UNIX socket server to handle
 * incoming client connections and key-value protocol messages.
 *
 * The program expects arguments to configure the database name and socket path.
 * If a WAL file exists, it is imported before serving new client requests.
 *
 * Startup sequence:
 * 1. Parse command-line arguments and validate configuration
 * 2. Set up database working directory and logging
 * 3. Initialize key-value table with specified parameters
 * 4. Import existing table file if present
 * 5. Replay WAL file if present to restore recent changes
 * 6. Register signal handlers for graceful shutdown
 * 7. Create and bind UNIX domain socket
 * 8. Start main server loop
 *
 * @param argc Argument count (should be greater than 1)
 * @param argv Argument vector containing configuration parameters
 *
 * @return 0 on successful execution, -1 on failure
 *
 * @note Signal handlers are registered for SIGINT, SIGTERM, and SIGHUP
 *       to allow graceful shutdown when interrupted
 * @note On shutdown, all resources are released: table, socket, etc.
 * @note Uses VictorTable structure for database context
 */
int main(int argc, char *argv[]) {
    struct sigaction sa;
    TableConfig cfg;  // Fixed: was Config instead of TableConfig
    VictorTable core;
    int server, ret;

    if (argc == 1) {
        fprintf(stderr, "missing arguments\n");
        table_usage(argv[0]);  // Fixed: was usage() instead of table_usage()
        return -1;
    }

    if (table_parse_arguments(argc, argv, &cfg) != 0)
        return -1;

    table_config_dump(&cfg);
    set_logfile(stderr);

    errno = 0;
    if (set_database_cwd(cfg.name) == -1) {
        log_message(LOG_ERROR, 
            "setting cwd (%s): %s", get_database_cwd(), strerror(errno)
        );
        return -1;
    }

    // Initialize VictorTable structure
    core.name = cfg.name;
    core.op_add_counter = 0;
    core.op_del_counter = 0;

    // Import existing table file if present
    if (access(TABLE_FILE, F_OK) == 0) {
        core.table = load_kvtable(TABLE_FILE);
    } else {
        core.table = alloc_kvtable(cfg.name);
    }

    if (!core.table) {
        log_message(LOG_ERROR, 
            "alloc table memory"
        );
        return -1;
    }

    // Replay WAL file if present to restore recent changes
    if (access(TWAL_FILE, F_OK) == 0) {
        log_message(LOG_INFO, "importing WAL file");
        FILE *wal = fopen(TWAL_FILE, "rb");
        if (wal == NULL) {
            log_message(LOG_ERROR, 
                "open WAL (%s): %s", TWAL_FILE, strerror(errno)
            );
            destroy_kvtable(&core.table);
            return -1;
        }
        if (victor_table_loadwal(&core, wal) != 0) {
            fclose(wal);
            destroy_kvtable(&core.table);
            return -1;
        }
        fclose(wal);
    }

    // Register signal handlers for graceful shutdown
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGHUP,  &sa, NULL);

    // Create and bind UNIX domain socket
    server = unix_server(cfg.socket.unix);
    if (server == -1) {
        log_message(LOG_ERROR, 
            "creating unix server: %s", strerror(errno)
        );
        destroy_kvtable(&core.table);
        return -1;
    }

    // Start main server loop
    ret = victor_table_server(&core, server);
    close(server);
    destroy_kvtable(&core.table);
    return ret;
}

