#ifndef BYTE_STR_H
#define BYTE_STR_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct byte_str {
    size_t  len;
    uint8_t data[];
} byte_str_t;

typedef struct get_byte_result {
    uint8_t byte;
    bool    success;
} get_byte_result_t;

/**
 * @brief Create a new byte string
 * @details This object can be freed with a call to `free`
 *
 * @param data The data
 * @param len The length of the data
 * @return byte_str_t* The byte string
 */
byte_str_t* byte_str_create(const uint8_t* data, size_t len);

/**
 * @brief Compare two byte strings
 *
 * @param a The first byte string
 * @param b The second byte string
 * @return int 0 if the byte strings are equal,
 *            -1 if the byte strings are different sizes,
 *            otherwise the difference between the first differing byte
 *            (positive if a > b, negative if a < b)
 */
int byte_str_cmp(const byte_str_t* a, const byte_str_t* b);

/**
 * @brief Get a byte from a byte string
 *
 * @param str The byte string
 * @param idx The index of the byte
 * @return get_byte_result_t The byte and whether the operation was successful
 */
get_byte_result_t byte_str_get_byte(const byte_str_t* str, size_t idx);

#endif // !BYTE_STR_H
