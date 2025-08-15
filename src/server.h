#ifndef __VICTOR_SERVER
#define __VICTOR_SERVER
#include <signal.h>
#define MAX_CONNECTIONS  128
#define EXPORT_THRESHOLD 10

extern void handle_signal(int signo);

extern sig_atomic_t running;
#endif