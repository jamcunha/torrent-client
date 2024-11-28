#ifndef FILE_H
#define FILE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct file file_t;

/**
 * @brief Create a new file
 *
 * @param path The path to the file
 * @param size The size of the file
 * @return file_t* The file
 */
file_t* file_create(const char* path, size_t size);

/**
 * @brief Get the size of the file
 *
 * @param file The file
 * @return size_t The size of the file
 */
size_t get_file_size(const file_t* file);

/**
 * @brief Get the path to the file
 *
 * @param file The file
 * @return const char* The path to the file
 */
const char* get_file_path(const file_t* file);

/**
 * @brief Write data to a file
 *
 * @param file The file
 * @param offset The offset to write the data
 * @param data The data to write
 * @param len The length of the data
 * @return int 0 if successful, -1 otherwise
 */
int write_data_to_file(file_t* file, size_t offset, uint8_t* data, size_t len);

/**
 * @brief Creates a new directory
 *
 * @param path The path to the directory
 * @return int 0 if successful, -1 otherwise
 */
int create_dir(const char* path);

/**
 * @brief Check if a directory exists
 *
 * @param path The path to the directory
 * @return true if the directory exists, false otherwise
 */
bool dir_exists(const char* path);

#endif // !FILE_H
