#ifndef BYTE_STR_H
#define BYTE_STR_H

#include <stdint.h>
#include <stdlib.h>

typedef struct byte_str {
    size_t len;
    uint8_t data[];
} byte_str_t;

/**
 * @brief Create a new byte string
 * @details This object can be freed with a call to `free`
 *
 * @param data The data
 * @param len The length of the data
 * @return byte_str_t* The byte string
 */
byte_str_t *byte_str_create(const uint8_t *data, size_t len);

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
int byte_str_cmp(const byte_str_t *a, const byte_str_t *b);

#endif // !BYTE_STR_H
