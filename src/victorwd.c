/**
 * @file wal_dump.c
 * @brief WAL (Write-Ahead Log) dump utility for VictorDB.
 *
 * This utility reads and displays the contents of VictorDB WAL files,
 * showing operation types, keys, values, and raw data in both hex and ASCII formats.
 * Supports both table WAL (db.twal) and index WAL (db.iwal) files.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>

#include "buffer.h"
#include "protocol.h"
#include "kvproto.h"
#include "viproto.h"
#include "fileutils.h"
#include "log.h"

/**
 * @brief Print data in hex dump format with ASCII representation.
 */
static void print_hex_dump(const void *data, size_t len, const char *prefix) {
    const uint8_t *bytes = (const uint8_t *)data;
    printf("%s", prefix);
    
    for (size_t i = 0; i < len; i++) {
        if (i > 0 && i % 16 == 0) {
            // Print ASCII representation
            printf("  |");
            for (size_t j = i - 16; j < i; j++) {
                printf("%c", isprint(bytes[j]) ? bytes[j] : '.');
            }
            printf("|\n%s", prefix);
        }
        
        if (i % 8 == 0 && i % 16 != 0) {
            printf(" ");
        }
        
        printf("%02x ", bytes[i]);
    }
    
    // Pad the last line and print ASCII
    size_t remaining = len % 16;
    if (remaining != 0) {
        for (size_t i = remaining; i < 16; i++) {
            if (i % 8 == 0 && i != 0) {
                printf(" ");
            }
            printf("   ");
        }
        printf("  |");
        for (size_t j = len - remaining; j < len; j++) {
            printf("%c", isprint(bytes[j]) ? bytes[j] : '.');
        }
        for (size_t i = remaining; i < 16; i++) {
            printf(" ");
        }
        printf("|");
    } else if (len > 0) {
        printf("  |");
        for (size_t j = len - 16; j < len; j++) {
            printf("%c", isprint(bytes[j]) ? bytes[j] : '.');
        }
        printf("|");
    }
    printf("\n");
}

/**
 * @brief Print a safe string representation, handling binary data.
 */
static void print_safe_string(const void *data, size_t len) {
    const uint8_t *bytes = (const uint8_t *)data;
    printf("\"");
    for (size_t i = 0; i < len && i < 100; i++) { // Limit to 100 chars
        if (isprint(bytes[i]) && bytes[i] != '"' && bytes[i] != '\\') {
            printf("%c", bytes[i]);
        } else {
            printf("\\x%02x", bytes[i]);
        }
    }
    if (len > 100) {
        printf("... (truncated, %zu bytes total)", len);
    }
    printf("\"");
}

/**
 * @brief Get message type name string.
 */
static const char* get_message_type_name(int type) {
    switch (type) {
        case MSG_INSERT:        return "INSERT";
        case MSG_DELETE:        return "DELETE";
        case MSG_SEARCH:        return "SEARCH";
        case MSG_MATCH_RESULT:  return "MATCH_RESULT";
        case MSG_PUT:           return "PUT";
        case MSG_DEL:           return "DEL";
        case MSG_GET:           return "GET";
        case MSG_GET_RESULT:    return "GET_RESULT";
        case MSG_OP_RESULT:     return "OP_RESULT";
        case MSG_ERROR:         return "ERROR";
        default:                return "UNKNOWN";
    }
}

/**
 * @brief Dump a single WAL entry with detailed information.
 */
static void dump_wal_entry(const buffer_t *buf, int entry_num, bool verbose) {
    printf("=== Entry #%d ===\n", entry_num);
    printf("Message Type: 0x%02x (%s)\n", buf->hdr.type, get_message_type_name(buf->hdr.type));
    printf("Message Length: %d bytes\n", buf->hdr.len);
    
    // Try to parse based on message type
    switch (buf->hdr.type) {
        case MSG_PUT: {
            void *key = NULL, *val = NULL;
            size_t klen = 0, vlen = 0;
            
            // Create a mutable copy for parsing
            buffer_t *temp_buf = alloc_buffer();
            if (temp_buf) {
                memcpy(temp_buf, buf, sizeof(buffer_t));
                
                if (buffer_read_put(temp_buf, &key, &klen, &val, &vlen) == 0) {
                    printf("Operation: PUT\n");
                    printf("Key (%zu bytes): ", klen);
                    print_safe_string(key, klen);
                    printf("\n");
                    printf("Value (%zu bytes): ", vlen);
                    print_safe_string(val, vlen);
                    printf("\n");
                    
                    if (verbose) {
                        printf("Key hex dump:\n");
                        print_hex_dump(key, klen, "  ");
                        printf("Value hex dump:\n");
                        print_hex_dump(val, vlen, "  ");
                    }
                    
                    if (key) free(key);
                    if (val) free(val);
                } else {
                    printf("Failed to parse PUT message\n");
                }
                free(temp_buf);
            }
            break;
        }
        
        case MSG_DEL: {
            void *key = NULL;
            size_t klen = 0;
            
            buffer_t *temp_buf = alloc_buffer();
            if (temp_buf) {
                memcpy(temp_buf, buf, sizeof(buffer_t));
                
                if (buffer_read_del(temp_buf, &key, &klen) == 0) {
                    printf("Operation: DELETE\n");
                    printf("Key (%zu bytes): ", klen);
                    print_safe_string(key, klen);
                    printf("\n");
                    
                    if (verbose) {
                        printf("Key hex dump:\n");
                        print_hex_dump(key, klen, "  ");
                    }
                    
                    if (key) free(key);
                } else {
                    printf("Failed to parse DELETE message\n");
                }
                free(temp_buf);
            }
            break;
        }
        
        case MSG_GET: {
            void *key = NULL;
            size_t klen = 0;
            
            buffer_t *temp_buf = alloc_buffer();
            if (temp_buf) {
                memcpy(temp_buf, buf, sizeof(buffer_t));
                
                if (buffer_read_get(temp_buf, &key, &klen) == 0) {
                    printf("Operation: GET\n");
                    printf("Key (%zu bytes): ", klen);
                    print_safe_string(key, klen);
                    printf("\n");
                    
                    if (verbose) {
                        printf("Key hex dump:\n");
                        print_hex_dump(key, klen, "  ");
                    }
                    
                    if (key) free(key);
                } else {
                    printf("Failed to parse GET message\n");
                }
                free(temp_buf);
            }
            break;
        }
        
        default:
            printf("Operation: %s (raw data only)\n", get_message_type_name(buf->hdr.type));
            break;
    }
    
    if (verbose || (buf->hdr.type != MSG_PUT && buf->hdr.type != MSG_DEL && buf->hdr.type != MSG_GET)) {
        printf("Raw message data:\n");
        print_hex_dump(buf->data, buf->hdr.len, "  ");
    }
    
    printf("\n");
}

/**
 * @brief Print usage information.
 */
static void print_usage(const char *prog_name) {
    printf("Usage: %s [OPTIONS] [WAL_FILE]\n", prog_name);
    printf("\nVictorDB WAL Dump Utility\n");
    printf("Reads and displays the contents of VictorDB Write-Ahead Log files.\n\n");
    printf("OPTIONS:\n");
    printf("  -v, --verbose     Show detailed hex dumps of all data\n");
    printf("  -t, --table       Dump table WAL file (db.twal) - default if no file specified\n");
    printf("  -i, --index       Dump index WAL file (db.iwal)\n");
    printf("  -c, --count       Only show entry count, don't dump contents\n");
    printf("  -h, --help        Show this help message\n\n");
    printf("EXAMPLES:\n");
    printf("  %s                    # Dump table WAL (db.twal) from current directory\n", prog_name);
    printf("  %s -v db.twal         # Verbose dump of specific WAL file\n", prog_name);
    printf("  %s -i                 # Dump index WAL (db.iwal)\n", prog_name);
    printf("  %s -c                 # Just count entries in table WAL\n", prog_name);
    printf("\n");
}

int main(int argc, char *argv[]) {
    int opt;
    bool verbose = false;
    bool count_only = false;
    bool use_index_wal = false;
    char *wal_file = NULL;
    
    struct option long_options[] = {
        {"verbose", no_argument, 0, 'v'},
        {"table", no_argument, 0, 't'},
        {"index", no_argument, 0, 'i'},
        {"count", no_argument, 0, 'c'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    while ((opt = getopt_long(argc, argv, "vtich", long_options, NULL)) != -1) {
        switch (opt) {
            case 'v':
                verbose = true;
                break;
            case 't':
                use_index_wal = false;
                break;
            case 'i':
                use_index_wal = true;
                break;
            case 'c':
                count_only = true;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    // Determine WAL file to use
    if (optind < argc) {
        wal_file = argv[optind];
    } else {
        wal_file = use_index_wal ? IWAL_FILE : TWAL_FILE;
    }
    
    // Open WAL file
    FILE *wal = fopen(wal_file, "rb");
    if (!wal) {
        fprintf(stderr, "Error: Failed to open WAL file '%s': %s\n", wal_file, strerror(errno));
        return 1;
    }
    
    printf("VictorDB WAL Dump - File: %s\n", wal_file);
    printf("Timestamp: %s", ctime(&(time_t){time(NULL)}));
    printf("=====================================\n\n");
    
    buffer_t *buf = alloc_buffer();
    if (!buf) {
        fprintf(stderr, "Error: Failed to allocate buffer\n");
        fclose(wal);
        return 1;
    }
    
    int entry_count = 0;
    int ret;
    
    while ((ret = buffer_load_wal(buf, wal)) == 1) {
        entry_count++;
        
        if (!count_only) {
            dump_wal_entry(buf, entry_count, verbose);
        }
    }
    
    if (ret == -1) {
        fprintf(stderr, "Warning: Error reading WAL file at entry %d: %s\n", 
                entry_count + 1, strerror(errno));
    }
    
    printf("=====================================\n");
    printf("Total entries processed: %d\n", entry_count);
    
    if (entry_count == 0) {
        printf("WAL file is empty or contains no valid entries.\n");
    }
    
    free(buf);
    fclose(wal);
    
    return 0;
}
