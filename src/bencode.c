#include "bencode.h"
#include "dict.h"
#include "list.h"
#include "sha1.h"

#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static bencode_node_t *bencode_node_create(bencode_type_t type) {
    bencode_node_t *node = malloc(sizeof(bencode_node_t));
    if (node == NULL) {
        return NULL;
    }

    node->type = type;
    return node;
}

// parsing a type of bencode may need to parse other types of bencode
// so we need to declare the parsing functions before we define them

static bencode_node_t *bencode_parse_str(const char *data, const char **endptr);
static bencode_node_t *bencode_parse_int(const char *data, const char **endptr);
static bencode_node_t *bencode_parse_list(const char *data, const char **endptr);
static bencode_node_t *bencode_parse_dict(const char *data, const char **endptr);

static bencode_node_t *bencode_parse_str(const char *data, const char **endptr) {
    if (data == NULL) {
        return NULL;
    }

    uint64_t len = strtoull(data, (char **)endptr, 10);
    assert(**endptr == ':');
    (*endptr)++;

    bencode_node_t *node = bencode_node_create(BENCODE_STR);
    if (node == NULL) {
        return NULL;
    }

    node->value.s = malloc(len + 1);
    if (node->value.s == NULL) {
        free(node);
        return NULL;
    }

    strncpy(node->value.s, *endptr, len);
    node->value.s[len] = '\0';
    *endptr += len;

    return node;
}

static bencode_node_t *bencode_parse_int(const char *data, const char **endptr) {
    if (data == NULL) {
        return NULL;
    }

    assert(*data == 'i');
    data++;

    int64_t i = strtoll(data, (char **)endptr, 10);
    assert(**endptr == 'e');
    (*endptr)++;

    bencode_node_t *node = bencode_node_create(BENCODE_INT);
    if (node == NULL) {
        return NULL;
    }
    node->value.i = i;

    return node;
}

static bencode_node_t *bencode_parse_list(const char *data, const char **endptr) {
    if (data == NULL) {
        return NULL;
    }

    assert(*data == 'l');
    *endptr = data + 1;

    bencode_node_t *node = bencode_node_create(BENCODE_LIST);
    if (node == NULL) {
        return NULL;
    }

    node->value.l = list_create((void (*)(void *))bencode_free);
    if (node->value.l == NULL) {
        free(node);
        return NULL;
    }

    while (**endptr != 'e') {
        data = *endptr;

        bencode_node_t *elem = bencode_parse(data, endptr);
        if (elem == NULL) {
            list_free(node->value.l);
            free(node);
            return NULL;
        }

        if (list_push(node->value.l, elem, sizeof(bencode_node_t))) {
            list_free(node->value.l);
            bencode_free(elem);
            free(node);
            return NULL;
        }

        // NOTE: Just free the value pointer. Other pointers memory is managed by the element in the list
        free(elem);
    }

    assert(**endptr == 'e');
    (*endptr)++;

    return node;
}

static bencode_node_t *bencode_parse_dict(const char *data, const char **endptr) {
    if (data == NULL) {
        return NULL;
    }

    assert(*data == 'd');
    *endptr = data + 1;

    bencode_node_t *node = bencode_node_create(BENCODE_DICT);
    if (node == NULL) {
        return NULL;
    }

    node->value.d = dict_create(1, (void (*)(void *))bencode_free);
    if (node->value.d == NULL) {
        free(node);
        return NULL;
    }

    while (**endptr != 'e') {
        data = *endptr;

        bencode_node_t *key = bencode_parse_str(data, endptr);
        if (key == NULL) {
            dict_free(node->value.d);
            free(node);
            return NULL;
        }

        data = *endptr;

        bencode_node_t *value = bencode_parse(data, endptr);
        if (value == NULL) {
            dict_free(node->value.d);
            free(node);
            return NULL;
        }

        if (dict_add(node->value.d, key->value.s, value, sizeof(bencode_node_t))) {
            dict_free(node->value.d);
            bencode_free(key);
            bencode_free(value);
            free(node);
            return NULL;
        }

        // free the key, we don't need it anymore
        bencode_free(key);

        // NOTE: Just free the value pointer. Other pointers memory is managed by the entry in the dictionary
        free(value);
    }

    assert(**endptr == 'e');
    (*endptr)++;

    return node;
}

bencode_node_t *bencode_parse(const char *data, const char **endptr) {
    if (data == NULL) {
        return NULL;
    }

    bencode_node_t *node = NULL;
    if (isdigit(*data)) {
        node = bencode_parse_str(data, endptr);
    } else if (*data == 'i') {
        node = bencode_parse_int(data, endptr);
    } else if (*data == 'l') {
        node = bencode_parse_list(data, endptr);
    } else if (*data == 'd') {
        node = bencode_parse_dict(data, endptr);
    } else {
        assert(0 && "Invalid bencode type");
    }

    // NOTE: sha1 is calculated for every node, maybe should be calculated only for dictionaries?
    if (node != NULL) {
        sha1((uint8_t *)data, *endptr - data, node->digest);
    }

    return node;
}

void bencode_free(bencode_node_t *node) {
    if (node == NULL) {
        return;
    }

    /* const char *key; */
    /* bencode_node_t *child_node; */

    switch (node->type) {
    case BENCODE_INT:
        break;
    case BENCODE_STR:
        free(node->value.s);
        break;
    case BENCODE_LIST:
        /* LIST_FOREACH(child_node, node->value.l) { */
        /*     bencode_free(child_node); */
        /* } */

        list_free(node->value.l);
        break;
    case BENCODE_DICT:
        /* DICT_FOREACH(key, child_node, node->value.d) { */
        /*     bencode_free(child_node); */
        /* } */

        dict_free(node->value.d);
        break;
    default:
        assert(0 && "Invalid bencode type");
    }

    free(node);
}
