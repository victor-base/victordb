/**
 * @file opt.h
 * @brief Configuration options and argument parsing for the VictorDB server.
 *
 * This header defines the configuration structure and constants used to
 * configure the VictorDB vector database server, including socket types,
 * index parameters, and command-line argument parsing functions.
 */

#ifndef __OPT_H
#define __OPT_H

/** @brief Socket type constant for TCP connections */
#define SOCKET_TCP  0x01
/** @brief Socket type constant for UNIX domain socket connections */
#define SOCKET_UNIX 0x02

#include <victor/victor.h>
#include <string.h>

/** @brief Default vector index type (HNSW - Hierarchical Navigable Small World) */
#define DEFAULT_INDEX_TYPE    HNSW_INDEX
/** @brief Default distance metric for vector similarity (cosine similarity) */
#define DEFAULT_INDEX_METHOD  COSINE
/** @brief Default socket type for server connections (UNIX domain socket) */
#define DEFAULT_SOCKET_TYPE   SOCKET_UNIX

#include "fileutils.h"

/**
 * @brief Configuration structure for the VictorDB vector index server.
 *
 * This structure holds all configuration parameters needed to initialize
 * and run the VictorDB vector index server, including database settings,
 * index parameters, and socket configuration.
 */
typedef struct {
    char *name;     /**< Database name/identifier */
    int i_dims;     /**< Vector dimensionality for the index */
    int i_type;     /**< Index type (e.g., HNSW_INDEX, FLAT_INDEX) */
    int i_method;   /**< Distance metric method (e.g., COSINE, L2, DOT_PRODUCT) */
    int s_type;     /**< Socket type (SOCKET_TCP or SOCKET_UNIX) */
    
    /**
     * @brief Socket configuration union.
     * 
     * Stores either UNIX domain socket path or TCP host/port configuration
     * depending on the socket type specified in s_type.
     */
    union {
        char *unix_path;  /**< Path to UNIX domain socket file */
        struct {
            char *host;  /**< TCP hostname or IP address */
            int   port;  /**< TCP port number */
        } tcp;           /**< TCP socket configuration */
    } socket;
} IndexConfig;

/**
 * @brief Configuration structure for the VictorDB table (key-value) server.
 *
 * This structure holds all configuration parameters needed to initialize
 * and run the VictorDB table server for key-value operations.
 */
typedef struct {
    char *name;     /**< Database name/identifier */
    int s_type;     /**< Socket type (SOCKET_TCP or SOCKET_UNIX) */
    
    /**
     * @brief Socket configuration union.
     * 
     * Stores either UNIX domain socket path or TCP host/port configuration
     * depending on the socket type specified in s_type.
     */
    union {
        char *unix;  /**< Path to UNIX domain socket file */
        struct {
            char *host;  /**< TCP hostname or IP address */
            int   port;  /**< TCP port number */
        } tcp;           /**< TCP socket configuration */
    } socket;
} TableConfig;

/** @brief Legacy type alias for backward compatibility */
typedef IndexConfig Config;

/**
 * @brief Parse command-line arguments for vector index server configuration.
 *
 * This function processes command-line arguments and fills the provided
 * IndexConfig structure with the parsed values. It handles various options
 * such as database name, vector dimensions, index parameters, socket configuration, etc.
 *
 * @param argc Number of command-line arguments
 * @param argv Array of command-line argument strings
 * @param cfg  Pointer to IndexConfig structure to be populated
 * 
 * @return 0 on success, -1 on error or invalid arguments
 */
extern int index_parse_arguments(int argc, char *argv[], IndexConfig *cfg);

/**
 * @brief Parse command-line arguments for table server configuration.
 *
 * This function processes command-line arguments and fills the provided
 * TableConfig structure with the parsed values. It handles options
 * such as database name and socket configuration.
 *
 * @param argc Number of command-line arguments
 * @param argv Array of command-line argument strings
 * @param cfg  Pointer to TableConfig structure to be populated
 * 
 * @return 0 on success, -1 on error or invalid arguments
 */
extern int table_parse_arguments(int argc, char *argv[], TableConfig *cfg);

/**
 * @brief Display usage information for vector index server.
 *
 * Prints help text showing all available command-line options for the
 * vector index server, their descriptions, and usage examples to stderr.
 *
 * @param progname Name of the program (usually argv[0])
 */
extern void index_usage(const char *progname);

/**
 * @brief Display usage information for table server.
 *
 * Prints help text showing all available command-line options for the
 * table (key-value) server, their descriptions, and usage examples to stderr.
 *
 * @param progname Name of the program (usually argv[0])
 */
extern void table_usage(const char *progname);

/**
 * @brief Print the current vector index server configuration settings.
 *
 * Displays all configuration parameters from the provided IndexConfig structure
 * for debugging and verification purposes.
 *
 * @param cfg Pointer to IndexConfig structure to display
 */
extern void index_config_dump(const IndexConfig *cfg);

/**
 * @brief Print the current table server configuration settings.
 *
 * Displays all configuration parameters from the provided TableConfig structure
 * for debugging and verification purposes.
 *
 * @param cfg Pointer to TableConfig structure to display
 */
extern void table_config_dump(const TableConfig *cfg);

/** @brief Legacy function alias for backward compatibility */
extern int parse_arguments(int argc, char *argv[], Config *cfg);

/** @brief Legacy function alias for backward compatibility */
extern void usage(const char *progname);

/** @brief Legacy function alias for backward compatibility */
extern void config_dump(const Config *cfg);

#endif
