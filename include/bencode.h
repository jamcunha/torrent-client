#ifndef BENCODE_H
#define BENCODE_H

#include "dict.h"
#include "list.h"
#include "sha1.h"

#include <stdint.h>

typedef enum {
    BENCODE_INT,
    BENCODE_STR,
    BENCODE_LIST,
    BENCODE_DICT
} bencode_type_t;

typedef struct node {
    bencode_type_t type;
    union {
        int64_t i;
        char *s;
        list_t *l;
        dict_t *d;
    } value;
    uint8_t digest[SHA1_DIGEST_SIZE];
} bencode_node_t;

/**
 * @brief Parse a bencode string
 *
 * @param data The bencode string
 * @param endptr A pointer to the character after the parsed bencode string
 * @return bencode_node* The parsed bencode node
 */
bencode_node_t *bencode_parse(const char *data, const char **endptr);

/**
 * @brief Free a bencode node
 *
 * @param node The node to free
 */
void bencode_free(bencode_node_t *node);

#endif // !BENCODE_H
