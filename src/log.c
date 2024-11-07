#include "log.h"

#include <assert.h>
#include <stdarg.h>
#include <time.h>

struct log {
    log_level_t level;
    FILE *file;
};

static struct log log = {
    .level = LOG_LEVEL_INFO,
    .file = NULL
};

static char *log_level_to_string(log_level_t level) {
    switch (level) {
        case LOG_LEVEL_DEBUG:
            return "DEBUG";
        case LOG_LEVEL_INFO:
            return "INFO";
        case LOG_LEVEL_WARN:
            return "WARN";
        case LOG_LEVEL_ERROR:
            return "ERROR";
        default:
            assert(0 && "Invalid log level");
    }
}

static char *log_level_colour(log_level_t level) {
    switch (level) {
        case LOG_LEVEL_DEBUG:
            // Blue
            return "\033[0;34m";
        case LOG_LEVEL_INFO:
            // Green
            return "\033[0;32m";
        case LOG_LEVEL_WARN:
            // Yellow
            return "\033[0;33m";
        case LOG_LEVEL_ERROR:
            // Red
            return "\033[0;31m";
        default:
            assert(0 && "Invalid log level");
    }
}

inline static char *log_level_colour_reset(void) {
    return "\033[0m";
}

void set_log_level(log_level_t level) {
    log.level = level;
}

void set_log_file(FILE *file) {
    log.file = file;
}

void log_message(log_level_t level, const char *format, ...) {
    if (level < log.level) {
        return;
    }

    // Get the current time
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char time_buf[20];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm);

    // TODO: add color only if the output is a terminal

    // If the log file is not set, print to stderr
    FILE *output = log.file ? log.file : stderr;

    // Print time and log level
    fprintf(output, "[%s] %s[%s]%s ", time_buf, log_level_colour(level), log_level_to_string(level), log_level_colour_reset());

    // Handle the variadic arguments
    va_list args;
    va_start(args, format);
    vfprintf(output, format, args);
    va_end(args);

    fprintf(output, "\n");
}
