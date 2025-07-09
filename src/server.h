#ifndef __VICTOR_SERVER
#define __VICTOR_SERVER

#include <victor/victor.h>
#include <victor/victorkv.h>

#define MAX_CONNECTIONS  128
#define EXPORT_THRESHOLD 1000
typedef struct {
    uint64_t id;
    char     *name;
    Index    *index;
    kvTable  *table;
    int op_add_counter;
    int op_del_counter;
} IndexKV;

extern int indexkv_server(IndexKV *core, int server);
extern int indexkv_load_wal(IndexKV *core, FILE *wal);
extern void handle_signal(int signo);
#endif