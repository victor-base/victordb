#ifndef __FILE_UTILS_H
#define __FILE_UTILS_H

#include <limits.h>
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif 

// Vector file for the database
#define INDEX_FILE  "db.index"
// Table file for the database
#define TABLE_FILE  "db.table"
// Autoincrement ID file
#define ID_FILE     "db.id"
// Write-Ahead Log (WAL) file
#define WAL_FILE    "db.wal"
// Root directory for all databases
#define DB_ROOT     "/var/lib/victord"

extern int set_database_cwd(const char *name);
extern const char *get_database_cwd(void);
#endif