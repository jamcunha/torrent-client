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

// TODO: For now we are doing "[<file>] ..." manually
//       we could do it in the macro (investigate how to add file name and line
//       [__FILE__, ...])

#define LOG_DEBUG(...) log_message(LOG_LEVEL_DEBUG, __VA_ARGS__)
#define LOG_INFO(...)  log_message(LOG_LEVEL_INFO, __VA_ARGS__)
#define LOG_WARN(...)  log_message(LOG_LEVEL_WARN, __VA_ARGS__)
#define LOG_ERROR(...) log_message(LOG_LEVEL_ERROR, __VA_ARGS__)

#endif // !LOG_H
