#ifndef __LOG_H
#define __LOG_H

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#define LOG_WARNING "WARNING"
#define LOG_ERROR   "ERROR"
#define LOG_START   "START"
#define LOG_INFO    "INFO"

extern void log_message(const char *level, const char *fmt, ...);
extern void set_logfile(const FILE *out);


#endif