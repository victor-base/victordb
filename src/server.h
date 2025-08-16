#ifndef __VICTOR_SERVER
#define __VICTOR_SERVER
#include <signal.h>
#include <stdlib.h>

#define MAX_CONNECTIONS  128
#define DEFAULT_EXPORT_THRESHOLD 10

/**
 * @brief Gets the export threshold from environment or default value.
 * 
 * Reads the VICTOR_EXPORT_THRESHOLD environment variable.
 * If not set or invalid, returns DEFAULT_EXPORT_THRESHOLD.
 * 
 * @return Export threshold value
 */
static inline int get_export_threshold(void) {
    const char *env_val = getenv("VICTOR_EXPORT_THRESHOLD");
    if (env_val) {
        int threshold = atoi(env_val);
        if (threshold > 0) {
            return threshold;
        }
    }
    return DEFAULT_EXPORT_THRESHOLD;
}

extern void handle_signal(int signo);

extern sig_atomic_t running;
#endif