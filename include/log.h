#ifndef LOG_H
#define LOG_H

#include <stdbool.h>
#include <stdio.h>

typedef enum {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_NONE
} log_level_t;

/**
 * @brief Set the log level
 *
 * @param level The log level
 */
void set_log_level(log_level_t level);

/**
 * @brief Set the log file
 * @details If the file is NULL, the log will be printed to stderr
 *
 * @param file The log file
 */
void set_log_file(FILE* file);

/**
 * @brief Check if a message will be logged
 *
 * @param level The log level
 * @return true if the message will be logged, false otherwise
 */
bool will_log(log_level_t level);

/**
 * @brief Log a message
 *
 * @param level The log level
 * @param format The format string
 * @param ... The format arguments
 */
void log_message(log_level_t level, const char* format, ...);

// NOTE: should log info also have the extra info?

#define LOG_DEBUG(format, ...)                                               \
    log_message(LOG_LEVEL_DEBUG, "[%s:%d (%s)] " format, __FILE__, __LINE__, \
                __func__, ##__VA_ARGS__)

#define LOG_INFO(...) log_message(LOG_LEVEL_INFO, __VA_ARGS__)

#define LOG_WARN(format, ...)                                               \
    log_message(LOG_LEVEL_WARN, "[%s:%d (%s)] " format, __FILE__, __LINE__, \
                __func__, ##__VA_ARGS__)

#define LOG_ERROR(format, ...)                                               \
    log_message(LOG_LEVEL_ERROR, "[%s:%d (%s)] " format, __FILE__, __LINE__, \
                __func__, ##__VA_ARGS__)

#endif // !LOG_H
