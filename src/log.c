#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>

static FILE *output = NULL;

void log_message(const char *level, const char *fmt, ...) {
    time_t now = time(NULL);
    struct tm tm_info;
    char timebuf[32];
    pid_t pid = getpid();

    localtime_r(&now, &tm_info);
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &tm_info);

    fprintf(output, "[%s] [%s] [pid:%d] ", timebuf, level, pid);

    va_list args;
    va_start(args, fmt);
    vfprintf(output, fmt, args);
    va_end(args);

    fputc('\n', output);
}

void set_logfile(FILE *out) {
    output = out;
}