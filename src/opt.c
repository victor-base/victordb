/**
 * @file opt.c
 * @brief Implementation of command-line argument parsing and configuration management.
 *
 * This file contains the implementation of functions for parsing command-line
 * arguments, setting up configuration defaults, and managing socket paths
 * for the VictorDB vector database server.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "opt.h"

/** @brief Static buffer to store the default UNIX socket path */
static char __default_socket_path[PATH_MAX] = {0};

/**
 * @brief Generate the default UNIX socket path for a database.
 *
 * Creates a standardized socket path in the format: {root}/{name}/socket.unix
 * If no root is provided, uses the default DB_ROOT directory.
 *
 * @param root Database root directory (can be NULL for default)
 * @param name Database name to include in the path
 * 
 * @return Pointer to the generated socket path string
 * 
 * @note The returned pointer is to a static buffer that will be overwritten
 *       on subsequent calls to this function.
 */
static char *set_default_socket_path(const char *root, const char *name) {
    snprintf(__default_socket_path, PATH_MAX, "%s/%s/socket.unix", 
             root == NULL ? get_db_root() : root, name);
    return __default_socket_path;
}

/**
 * @brief Display usage information and command-line options.
 *
 * Prints comprehensive help text showing all available command-line options,
 * their descriptions, default values, and usage examples. The output is
 * sent to stderr to allow proper shell redirection.
 *
 * @param progname Name of the program executable (typically argv[0])
 */
void index_usage(const char *progname) {
    fprintf(stderr,
        "Usage: %s -n <dbname> -d <dimensions> [options]\n\n"
        "Required arguments:\n"
        "  -n <dbname>        Name of the database to create or open\n"
        "  -d <dimensions>    Dimensionality of the vectors\n\n"
        "Optional arguments:\n"
        "  -t <type>          Index type (flat | hnsw) [default: hnsw]\n"
        "  -m <method>        Similarity method (cosine | dotp | l2norm) [default: cosine]\n"
        "  -u <socket_path>   Path to UNIX socket [default: auto-generated]\n"
        "\nExample:\n"
        "  %s -n musicdb -d 128 -t hnsw -m cosine -u /tmp/musicdb.sock\n",
        progname, progname
    );
}

/**
 * @brief Display usage information for table server.
 *
 * Prints comprehensive help text showing all available command-line options
 * for the table (key-value) server, their descriptions, and usage examples.
 * The output is sent to stderr to allow proper shell redirection.
 *
 * @param progname Name of the program executable (typically argv[0])
 */
void table_usage(const char *progname) {
    fprintf(stderr,
        "Usage: %s -n <dbname> [options]\n\n"
        "Required arguments:\n"
        "  -n <dbname>        Name of the database to create or open\n"
        "Optional arguments:\n"
        "  -u <socket_path>   Path to UNIX socket [default: auto-generated]\n"
        "\nExample:\n"
        "  %s -n musicdb -u /tmp/musicdb.sock\n",
        progname, progname
    );
}

/**
 * @brief Parse command-line arguments and populate configuration structure.
 *
 * This function processes command-line arguments using getopt and fills the
 * provided Config structure with parsed values. It handles validation of
 * arguments, sets defaults for optional parameters, and performs error
 * checking on required arguments.
 *
 * Supported options:
 * - -n: Database name (required)
 * - -d: Vector dimensions (required)
 * - -t: Index type (flat|hnsw, default: hnsw)
 * - -m: Distance metric (cosine|dotp|l2norm, default: cosine)
 * - -u: UNIX socket path (default: auto-generated)
 * - -h: TCP host:port (switches to TCP mode)
 *
 * @param argc Number of command-line arguments
 * @param argv Array of command-line argument strings
 * @param cfg  Pointer to Config structure to populate
 * 
 * @return 0 on success, -1 on error or missing required arguments
 * 
 * @note The function will print error messages to stderr for invalid arguments
 * @note TCP socket support is mentioned but marked as "not enabled"
 */
int index_parse_arguments(int argc, char *argv[], IndexConfig *cfg) {
    int opt;
    memset(cfg, 0, sizeof(IndexConfig));

    // Set default values for optional parameters
    cfg->i_type   = DEFAULT_INDEX_TYPE;
    cfg->i_method = DEFAULT_INDEX_METHOD;
    cfg->s_type   = DEFAULT_SOCKET_TYPE;


    while ((opt = getopt(argc, argv, "d:t:n:m:u:h:")) != -1) {
        switch (opt) {
            case 'n':  // Database name
                cfg->name = optarg;
                break;
            case 't':  // Index type
                if (strcmp(optarg, "flat") == 0)
                    cfg->i_type = FLAT_INDEX;
                else if (strcmp(optarg, "hnsw") == 0)
                    cfg->i_type = HNSW_INDEX;
                else
                    fprintf(stderr, 
                        "invalid argument for -t (index type): %s, using default: %s\n", 
                        optarg, "hnsw"
                    );
                break;
            case 'd':  // Vector dimensions
                cfg->i_dims = atoi(optarg);
                break;
            case 'm':  // Distance metric method
                if (strcmp(optarg, "cosine") == 0) 
                    cfg->i_method = COSINE;
                else if (strcmp(optarg, "dotp") == 0)
                    cfg->i_method = DOTP;
                else if (strcmp(optarg, "l2norm") == 0)
                    cfg->i_method = L2NORM;
                else 
                    fprintf(stderr, 
                        "invalid argument for -m (method): %s, using default: %s\n", 
                        optarg, "cosine"
                    );
                break;
            case 'u':  // UNIX socket path
                cfg->s_type = SOCKET_UNIX;
                cfg->socket.unix_path = optarg;
                break;
            case 'h':  // TCP host:port
                cfg->s_type = SOCKET_TCP;
                char *sep = strchr(optarg, ':');
                if (!sep) {
                    fprintf(stderr, "invalid TCP argument host:port - Abort\n");
                    return -1;
                }
                *sep = 0;  // Split the string at the colon
                cfg->socket.tcp.host = optarg;
                cfg->socket.tcp.port = atoi(sep + 1);
                break;
            default:  // Unknown option
                fprintf(stderr, "invalid argument - Abort\n");
                index_usage(argv[0]);  // Fixed: was usage() instead of index_usage()
                return -1;
        }
    }

    // Validate that required arguments are provided
    if (!cfg->name || !cfg->i_dims) {
        fprintf(stderr, "missing required arguments -d <dims> -n <name>\n");
        return -1;
    }

    // Set default socket path if none was specified
    if (cfg->socket.unix_path == NULL)
        cfg->socket.unix_path = set_default_socket_path(NULL, cfg->name);
    return 0;
}

/**
 * @brief Parse command-line arguments for table server configuration.
 *
 * This function processes command-line arguments using getopt and fills the
 * provided TableConfig structure with parsed values. It handles validation of
 * arguments, sets defaults for optional parameters, and performs error
 * checking on required arguments.
 *
 * Supported options:
 * - -n: Database name (required)
 * - -u: UNIX socket path (default: auto-generated)
 * - -h: TCP host:port (switches to TCP mode)
 *
 * @param argc Number of command-line arguments
 * @param argv Array of command-line argument strings
 * @param cfg  Pointer to TableConfig structure to populate
 * 
 * @return 0 on success, -1 on error or missing required arguments
 * 
 * @note The function will print error messages to stderr for invalid arguments
 * @note TCP socket support is mentioned but marked as "not enabled"
 */
int table_parse_arguments(int argc, char *argv[], TableConfig *cfg) {
    int opt;
    memset(cfg, 0, sizeof(TableConfig));  // Fixed: was Config instead of TableConfig

    // Set default values for optional parameters
    cfg->s_type = DEFAULT_SOCKET_TYPE;

    while ((opt = getopt(argc, argv, "n:u:h:")) != -1) {  // Fixed: added 'n' to options
        switch (opt) {
            case 'n':  // Database name
                cfg->name = optarg;
                break;
            case 'u':  // UNIX socket path
                cfg->s_type = SOCKET_UNIX;
                cfg->socket.unix_path = optarg;
                break;
            case 'h':  // TCP host:port
                cfg->s_type = SOCKET_TCP;
                char *sep = strchr(optarg, ':');
                if (!sep) {
                    fprintf(stderr, "invalid TCP argument host:port - Abort\n");
                    return -1;
                }
                *sep = 0;  // Split the string at the colon
                cfg->socket.tcp.host = optarg;
                cfg->socket.tcp.port = atoi(sep + 1);
                break;
            default:  // Unknown option
                fprintf(stderr, "invalid argument - Abort\n");
                table_usage(argv[0]);  // Fixed: was usage() instead of table_usage()
                return -1;
        }
    }

    // Validate that required arguments are provided
    if (!cfg->name) {
        fprintf(stderr, "missing required argument -n <name>\n");
        return -1;
    }

    // Set default socket path if none was specified
    if (cfg->socket.unix_path == NULL)
        cfg->socket.unix_path = set_default_socket_path(NULL, cfg->name);
    return 0;
}

/**
 * @brief Display current configuration settings.
 *
 * Prints a formatted dump of all configuration parameters from the provided
 * Config structure. This is useful for debugging, verification, and logging
 * the server's startup configuration.
 *
 * The output includes:
 * - Database name and vector dimensions
 * - Index type and similarity method
 * - Socket configuration (type and path/host:port)
 *
 * @param cfg Pointer to Config structure to display
 * 
 * @note Output is sent to stdout with a structured format
 * @note TCP socket functionality is marked as "not enabled"
 */
void index_config_dump(const IndexConfig *cfg) {
    // Convert enum values to human-readable strings
    const char *index_type_str = (cfg->i_type == FLAT_INDEX) ? "flat" :
                                 (cfg->i_type == HNSW_INDEX) ? "hnsw" : "unknown";

    const char *method_str = (cfg->i_method == COSINE) ? "cosine" :
                             (cfg->i_method == DOTP) ? "dotp" :
                             (cfg->i_method == L2NORM) ? "l2norm" : "unknown";

    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                       Victor Vector Index Server                      ║\n");
    printf("╠════════════════════════════════════════════════════════════════════════╣\n");
    printf("║  Version: %-61s ║\n", __LIB_VERSION());
    printf("╠════════════════════════════════════════════════════════════════════════╣\n");
    printf("║  Configuration Summary                                                 ║\n");
    printf("╠═══════════════════════╪════════════════════════════════════════════════╣\n");
    printf("║  Database Name         │ %-47s ║\n", cfg->name);
    printf("║  Vector Dimensions     │ %-47d ║\n", cfg->i_dims);
    printf("║  Index Type            │ %-47s ║\n", index_type_str);
    printf("║  Similarity Method     │ %-47s ║\n", method_str);
    printf("╠═══════════════════════╪════════════════════════════════════════════════╣\n");

    // Display socket configuration based on type
    if (cfg->s_type == SOCKET_UNIX) {
        printf("║  Socket Type           │ %-47s ║\n", "UNIX Domain Socket");
        printf("║  Socket Path           │ %-47s ║\n", cfg->socket.unix_path);
    } else if (cfg->s_type == SOCKET_TCP) {
        printf("║  Socket Type           │ %-47s ║\n", "TCP (not enabled)");
        printf("║  Host                  │ %-47s ║\n", cfg->socket.tcp.host);
        printf("║  Port                  │ %-47d ║\n", cfg->socket.tcp.port);
    } else {
        printf("║  Socket Type           │ %-47s ║\n", "Unknown");
    }
    
    printf("╚═══════════════════════╧════════════════════════════════════════════════╝\n");
    printf("\n");
}

/**
 * @brief Display current table server configuration settings.
 *
 * Prints a formatted dump of all configuration parameters from the provided
 * TableConfig structure. This is useful for debugging, verification, and logging
 * the table server's startup configuration.
 *
 * The output includes:
 * - Database name
 * - Socket configuration (type and path/host:port)
 * - Formatted with Unicode box-drawing characters for aesthetic appeal
 *
 * @param cfg Pointer to TableConfig structure to display
 * 
 * @note Output is sent to stdout with a structured format
 * @note TCP socket functionality is marked as "not enabled"
 */
void table_config_dump(const TableConfig *cfg) {  // Fixed: was IndexConfig instead of TableConfig
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════════════╗\n");
    printf("║                         Victor Table Server                           ║\n");
    printf("╠════════════════════════════════════════════════════════════════════════╣\n");
    printf("║  Version: %-61s ║\n", __LIB_VERSION());
    printf("╠════════════════════════════════════════════════════════════════════════╣\n");
    printf("║  Configuration Summary                                                 ║\n");
    printf("╠═══════════════════════╪════════════════════════════════════════════════╣\n");
    printf("║  Database Name         │ %-47s ║\n", cfg->name);
    printf("╠═══════════════════════╪════════════════════════════════════════════════╣\n");

    // Display socket configuration based on type
    if (cfg->s_type == SOCKET_UNIX) {
        printf("║  Socket Type           │ %-47s ║\n", "UNIX Domain Socket");
        printf("║  Socket Path           │ %-47s ║\n", cfg->socket.unix_path);
    } else if (cfg->s_type == SOCKET_TCP) {
        printf("║  Socket Type           │ %-47s ║\n", "TCP (not enabled)");
        printf("║  Host                  │ %-47s ║\n", cfg->socket.tcp.host);
        printf("║  Port                  │ %-47d ║\n", cfg->socket.tcp.port);
    } else {
        printf("║  Socket Type           │ %-47s ║\n", "Unknown");
    }
    
    printf("╚═══════════════════════╧════════════════════════════════════════════════╝\n");
    printf("\n");
}
