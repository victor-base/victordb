#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "opt.h"

static char __default_socket_path[PATH_MAX] = {0};

static char *set_default_socket_path(const char *root, const char *name) {
    snprintf(__default_socket_path, PATH_MAX, "%s/%s/socket.unix", root == NULL ? DB_ROOT : root, name);
    return __default_socket_path;
}


void usage(const char *progname) {
    fprintf(stderr,
        "Usage: %s -d <dbname> -n <dimensions> [options]\n\n"
        "Required arguments:\n"
        "  -d <dbname>        Name of the database to create or open\n"
        "  -n <dimensions>    Dimensionality of the vectors\n\n"
        "Optional arguments:\n"
        "  -t <type>          Index type (flat | hnsw) [default: hnsw]\n"
        "  -m <method>        Similarity method (cosine | dotp | l2norm) [default: cosine]\n"
        "  -u <socket_path>   Path to UNIX socket [default: auto-generated]\n"
        "\nExample:\n"
        "  %s -d musicdb -n 128 -t hnsw -m cosine -u /tmp/musicdb.sock\n",
        progname, progname
    );
}


int parse_arguments(int argc, char *argv[], Config *cfg) {
    int opt;
    memset(cfg, 0, sizeof(Config));

    // Set with defaults first
    cfg->i_type   = DEFAULT_INDEX_TYPE;
    cfg->i_method = DEFAULT_INDEX_METHOD;
    cfg->s_type   = DEFAULT_SOCKET_TYPE;


    while ((opt = getopt(argc, argv, "d:t:n:m:u:h:")) != -1) {
        switch (opt) {
            case 'd':
                cfg->name = optarg;
                break;
            case 't':
                if (strcmp(optarg, "flat") == 0)
                    cfg->i_type = FLAT_INDEX;
                else if (strcmp(optarg, "hnsw") == 0)
                    cfg->i_type = HNSW_INDEX;
                else
                    fprintf(stderr, 
                        "invalid argument for -t (index type): %s, using default: %s\n", optarg, "hnsw"
                    );
                break;
            case 'n':
                cfg->i_dims = atoi(optarg);
                break;
            case 'm':
                if (strcmp(optarg, "cosine") == 0) 
                    cfg->i_method = COSINE;
                else if (strcmp(optarg, "dotp") == 0)
                    cfg->i_method = DOTP;
                else if (strcmp(optarg, "l2norm") == 0)
                    cfg->i_method = L2NORM;
                else 
                    fprintf(stderr, 
                        "invalid argument for -m (method): %s, using default: %s\n", optarg, "cosine"
                    );
                break;
            case 'u':
                cfg->s_type = SOCKET_UNIX;
                cfg->socket.unix = optarg;
                break;
            case 'h':
                cfg->s_type = SOCKET_TCP;
                char *sep = strchr(optarg, ':');
                if (!sep) {
                    fprintf(stderr, "invalid TCP argument host:port - Abort\n");
                    return -1;
                }
                *sep = 0;
                cfg->socket.tcp.host = optarg;
                cfg->socket.tcp.port = atoi(sep + 1);
                break;
            default:
                fprintf(stderr, "invalid argument - Abort\n");
                usage(argv[0]);
                return -1;
        }
    }

    if (!cfg->name || !cfg->i_dims) {
        fprintf(stderr, "missing required arguments -n <dims> -d <name>\n");
        return -1;
    }

    if (cfg->socket.unix == NULL)
        cfg->socket.unix = set_default_socket_path(NULL, cfg->name);
    return 0;
}


void config_dump(const Config *cfg) {
    const char *index_type_str = (cfg->i_type == FLAT_INDEX) ? "flat" :
                                 (cfg->i_type == HNSW_INDEX) ? "hnsw" : "unknown";

    const char *method_str = (cfg->i_method == COSINE) ? "cosine" :
                             (cfg->i_method == DOTP) ? "dotp" :
                             (cfg->i_method == L2NORM) ? "l2norm" : "unknown";

    printf("=== Configuration Dump ===\n");
    printf("Database Name     : %s\n", cfg->name);
    printf("Dimensions        : %d\n", cfg->i_dims);
    printf("Index Type        : %s\n", index_type_str);
    printf("Similarity Method : %s\n", method_str);

    if (cfg->s_type == SOCKET_UNIX) {
        printf("Socket Type       : UNIX\n");
        printf("Socket Path       : %s\n", cfg->socket.unix);
    } else if (cfg->s_type == SOCKET_TCP) {
        printf("Socket Type       : TCP (not enabled)\n");
        printf("Host              : %s\n", cfg->socket.tcp.host);
        printf("Port              : %d\n", cfg->socket.tcp.port);
    } else {
        printf("Socket Type       : Unknown\n");
    }

    printf("==========================\n");
}
