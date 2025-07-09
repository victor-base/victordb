#ifndef __OPT_H
#define __OPT_H

#define SOCKET_TCP  0x01
#define SOCKET_UNIX 0x02

#include <victor/victor.h>
#include <string.h>

#define DEFAULT_INDEX_TYPE    HNSW_INDEX
#define DEFAULT_INDEX_METHOD  COSINE
#define DEFAULT_SOCKET_TYPE   SOCKET_UNIX

#include "fileutils.h"

typedef struct {
    char *name;     // Database Name
    int i_dims;     // Index Dimensions
    int i_type;     // Index Type
    int i_method;   // Index Method
    int s_type;     // Socket Type
    union {
        char *unix;
        struct {
            char *host;
            int   port;
        } tcp;
    } socket;
} Config;

extern int parse_arguments(int argc, char *argv[], Config *cfg);
extern void usage(const char *progname);
extern void config_dump(const Config *cfg);
#endif
