#include "fileutils.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static char __database_path[PATH_MAX] = {0};

static int ensure_db_dir(const char *dbdir) {
    struct stat st;
    if (!dbdir) return -1;
    
    if (stat(dbdir, &st) == -1) {
        if (mkdir(dbdir, 0700) != 0) {
            if (errno != EEXIST) return -1;
        }
    } else if (!S_ISDIR(st.st_mode)) {
        return -1;
    }
    return 0;
}

const char *get_database_cwd(void) {
    return __database_path;
}

const char *get_db_root(void) {
    const char *env_root = getenv("VICTOR_DB_ROOT");
    return env_root ? env_root : DEFAULT_DB_ROOT;
}

int set_database_cwd(const char *name) {
    snprintf(__database_path, PATH_MAX, "%s/%s", get_db_root(), name);
    if (ensure_db_dir(__database_path) == 0)
        return chdir(__database_path);
    return -1;
}

