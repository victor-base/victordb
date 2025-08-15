#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
/**
 * @brief Global flag to control server loop execution.
 *
 * This volatile variable is modified asynchronously by signal handlers 
 * to indicate that the server should terminate gracefully.
 */
volatile sig_atomic_t running = 1;

/**
 * @brief Signal handler to request server shutdown.
 *
 * Sets the global `running` flag to 0 when a termination signal (e.g., SIGINT, SIGTERM)
 * is received, causing the main server loop to exit cleanly.
 *
 * @param signo The signal number that was received.
 */
void handle_signal(int signo) {
    (void)signo;
    running = 0;
}
