/**
 * @file index_main.c
 * @brief Entry point for the VictorDB vector index server.
 *
 * This file contains the main function that initializes the VictorDB vector index server,
 * loads configuration, imports existing data, and starts the server loop.
 */

#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include "fileutils.h"
#include "socket.h"
#include "index_server.h"
#include "server.h"
#include "opt.h"
#include "log.h"

/**
 * @brief Entry point for the VictorDB vector index server.
 *
 * Initializes the vector index database environment, loads the configuration and WAL,
 * registers signal handlers, and starts the UNIX socket server to handle
 * incoming client connections and vector protocol messages.
 *
 * The program expects arguments to configure the database name, index type,
 * similarity method, and dimensionality. If a WAL file exists, it is imported
 * before serving new client requests.
 *
 * Startup sequence:
 * 1. Parse command-line arguments and validate configuration
 * 2. Set up database working directory and logging
 * 3. Initialize vector index with specified parameters
 * 4. Import existing index file if present
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
 * @note On shutdown, all resources are released: index, socket, etc.
 * @note Uses VictorIndex structure for database context
 */
int main(int argc, char *argv[]) {
    struct sigaction sa;
    IndexConfig cfg;  // Fixed: was Config instead of IndexConfig
    VictorIndex core;
    int server, ret;

    if (argc == 1) {
        fprintf(stderr, "missing arguments\n");
        index_usage(argv[0]);  // Fixed: was usage() instead of index_usage()
        return -1;
    }

    if (index_parse_arguments(argc, argv, &cfg) != 0)  // Fixed: was parse_arguments()
        return -1;

    index_config_dump(&cfg);  // Fixed: was config_dump()
    set_logfile(stderr);

    errno = 0;
    if (set_database_cwd(cfg.name) == -1) {
        log_message(LOG_ERROR, 
            "setting cwd (%s): %s", get_database_cwd(), strerror(errno)
        );
        return -1;
    }

    // Initialize VictorIndex structure
    core.name = cfg.name;
    core.op_add_counter = 0;
    core.op_del_counter = 0;

    // Initialize vector index with specified parameters
    ret = safe_alloc_index(&core.index, cfg.i_type, cfg.i_method, cfg.i_dims, NULL);
    if (ret != SUCCESS) {
        log_message(LOG_ERROR, 
            "alloc index memory: %s", index_strerror(ret)
        );
        return -1;
    }

    // Import existing index file if present
    if (access(INDEX_FILE, F_OK) == 0) {
        if ((ret = import(core.index, INDEX_FILE, IMPORT_OVERWITE)) != SUCCESS) {
            destroy_index(&core.index);
            log_message(LOG_ERROR, 
                "importing index: %s", index_strerror(ret)
            );
            return -1;
        }
    }

    // Replay WAL file if present to restore recent changes
    if (access(IWAL_FILE, F_OK) == 0) {  // Fixed: was WAL_FILE instead of IWAL_FILE
        log_message(LOG_INFO, "importing WAL file");
        FILE *wal = fopen(IWAL_FILE, "rb");  // Fixed: was WAL_FILE instead of IWAL_FILE
        if (wal == NULL) {
            log_message(LOG_ERROR, 
                "open WAL (%s): %s", IWAL_FILE, strerror(errno)  // Fixed: was WAL_FILE
            );
            destroy_index(&core.index);
            return -1;
        }
        if (victor_index_loadwal(&core, wal) != 0) {  // Fixed: was vector_index_loadwal()
            fclose(wal);
            destroy_index(&core.index);
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
        destroy_index(&core.index);
        return -1;
    }

    // Start main server loop
    ret = victor_index_server(&core, server);  // Fixed: was vector_index_server()
    close(server);
    destroy_index(&core.index);
    return ret;
}

