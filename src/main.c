#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include "fileutils.h"
#include "socket.h"
#include "server.h"
#include "opt.h"
#include "log.h"



/**
 * @brief Entry point for the IndexKV vector database daemon.
 *
 * Initializes the database environment, loads the configuration and WAL,
 * registers signal handlers, and starts the UNIX socket server to handle
 * incoming client connections and protocol messages.
 *
 * The program expects arguments to configure the database name, index type,
 * similarity method, and dimensionality. If a WAL file exists, it is imported
 * before serving new client requests.
 *
 * @param argn Argument count (should be greater than 1).
 * @param argv Argument vector containing configuration parameters.
 *
 * @return 0 on successful execution, -1 on failure.
 *
 * @note Signal handlers are registered for SIGINT, SIGTERM, and SIGHUP.
 *       These allow the server to shut down gracefully when interrupted.
 * @note On shutdown, all resources are released: index, table, socket.
 */
int main(int argn, char *argv[]) {
    struct sigaction sa;
    Config  cfg;
    IndexKV core;
    int server, ret;

    if (argn == 1) {
        fprintf(stderr, "missing arguments");
        usage(argv[0]);
        return -1;
    }

    if (parse_arguments(argn, argv, &cfg) != 0)
        return -1;

    config_dump(&cfg);
    set_logfile(stderr);

    errno = 0;
    if (set_database_cwd(cfg.name) == -1) {
        log_message(LOG_ERROR, 
            "setting cwd (%s): %s", get_database_cwd(), strerror(errno)
        );
        return -1;
    }

    core.id = 0;
    core.name = cfg.name;
    core.op_add_counter = 0;
    core.op_del_counter = 0;

    ret = safe_alloc_index(&core.index, cfg.i_type, cfg.i_method, cfg.i_dims, NULL);
    if (ret != SUCCESS) {
        log_message(LOG_ERROR, 
            "alloc index memory: %s", index_strerror(ret)
        );
        return -1;
    }

    if (access(INDEX_FILE, F_OK) == 0) {
        if ((ret = import(core.index, INDEX_FILE, IMPORT_OVERWITE)) != SUCCESS) {
            destroy_index(&core.index);
            log_message(LOG_ERROR, 
                "importing index: %s", index_strerror(ret)
            );
            return -1;
        }
    }

    errno = 0;
    core.table = access(TABLE_FILE, F_OK) == 0 ?
        load_kvtable(TABLE_FILE) : alloc_kvtable(core.name);
    if (core.table == NULL) {
        destroy_index(&core.index);
        log_message(LOG_ERROR,
            "alloc or load table: %s", strerror(errno)
        );
        return -1;
    }

    if (access(WAL_FILE, F_OK) == 0) {
        log_message(LOG_INFO, "importing WAL file");
        FILE *wal = fopen(WAL_FILE, "rb");
        if (wal == NULL) {
            log_message(LOG_ERROR, 
                "open WAL (%s): %s", WAL_FILE, strerror(errno)
            );
            destroy_index(&core.index);
            destroy_kvtable(&core.table);
            return -1;
        }
        if (indexkv_load_wal(&core, wal) != 0) {
            fclose(wal);
            destroy_index(&core.index);
            destroy_kvtable(&core.table);
            return -1;
        }
        fclose(wal);
    }

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGHUP,  &sa, NULL);

    server = unix_server(cfg.socket.unix);
    if (server == -1) {
        log_message(LOG_ERROR, 
            "creating unix server: %s", strerror(errno)
        );
        destroy_index(&core.index);
        destroy_kvtable(&core.table);
        return -1;
    }

    ret = indexkv_server(&core, server);
    close(server);
    destroy_index(&core.index);
    destroy_kvtable(&core.table);
    return ret;
}

