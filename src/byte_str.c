#include "byte_str.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>

byte_str_t *byte_str_create(const uint8_t *data, size_t len) {
    if (data == NULL) {
        LOG_WARN("[byte_str.c] Must provide data to create byte string");
        return NULL;
    }

    byte_str_t *byte_str = malloc(sizeof(byte_str_t) + len + 1);
    if (byte_str == NULL) {
        LOG_ERROR("[byte_str.c] Failed to allocate memory for byte string");
        return NULL;
    }

    byte_str->len = len;
    memcpy(byte_str->data, data, len);
    byte_str->data[len] = '\0';

    return byte_str;
}

int byte_str_cmp(const byte_str_t *a, const byte_str_t *b) {
    if (a == NULL || b == NULL) {
        LOG_WARN("[byte_str.c] Must provide two byte strings to compare");
        return -1;
    }

    if (a->len != b->len) {
        return -1;
    }

    return memcmp(a->data, b->data, a->len);
}
