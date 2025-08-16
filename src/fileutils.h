/**
 * @file fileutils.h
 * @brief File path utilities and database file definitions.
 *
 * This header defines standard file names, paths, and utility functions
 * for managing database files, WAL files, and working directories.
 */

#ifndef __FILE_UTILS_H
#define __FILE_UTILS_H

#include <limits.h>

#ifndef PATH_MAX
/** @brief Maximum path length if not defined by system */
#define PATH_MAX 4096
#endif 

/** @brief Vector index database file name */
#define INDEX_FILE  "db.index"

/** @brief Key-value table database file name */
#define TABLE_FILE  "db.table"

/** @brief Write-Ahead Log file for vector index operations */
#define IWAL_FILE   "db.iwal"

/** @brief Write-Ahead Log file for table operations */
#define TWAL_FILE   "db.twal"

/** @brief Default root directory for all database instances */
#define DEFAULT_DB_ROOT "/var/lib/victord"

/**
 * @brief Gets the database root directory from environment or default value.
 * 
 * Reads the VICTOR_DB_ROOT environment variable.
 * If not set, returns DEFAULT_DB_ROOT.
 * 
 * @return Database root directory path
 */
extern const char *get_db_root(void);

/**
 * @brief Sets the current working directory for database operations.
 *
 * Changes the process working directory to the specified database directory
 * under DB_ROOT. Creates the directory if it doesn't exist.
 *
 * @param name Name of the database instance directory.
 * @return 0 on success, -1 on failure.
 */
extern int set_database_cwd(const char *name);

/**
 * @brief Gets the current database working directory.
 *
 * Returns the absolute path of the current database working directory.
 *
 * @return Pointer to the current database directory path, or NULL if not set.
 */
extern const char *get_database_cwd(void);

#endif /* __FILE_UTILS_H */