#ifndef FILE_H
#define FILE_H

#include <stdbool.h>
#include <stdlib.h>

typedef struct file file_t;

/**
 * @brief Create a new file
 *
 * @param path The path to the file
 * @param size The size of the file
 * @return file_t* The file
 */
file_t *file_create(const char *path, size_t size);

/**
 * @brief Free the file
 *
 * @param file The file
 */
void file_free(file_t *file);

/**
 * @brief Creates a new directory
 *
 * @param path The path to the directory
 * @return int 0 if successful, -1 otherwise
 */
int create_dir(const char *path);

/**
 * @brief Check if a directory exists
 *
 * @param path The path to the directory
 * @return true if the directory exists, false otherwise
 */
bool dir_exists(const char *path);

#endif // !FILE_H
