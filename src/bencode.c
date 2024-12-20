#include "bencode.h"

#include "byte_str.h"
#include "dict.h"
#include "list.h"
#include "log.h"
#include "sha1.h"

#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static bencode_node_t* bencode_node_create(bencode_type_t type) {
    bencode_node_t* node = malloc(sizeof(bencode_node_t));
    if (node == NULL) {
        LOG_ERROR("Failed to allocate memory for bencode node");
        return NULL;
    }

    node->type = type;
    return node;
}

static char* bencode_type_to_string(bencode_type_t type) {
    switch (type) {
    case BENCODE_INT:
        return "INT";
    case BENCODE_STR:
        return "STR";
    case BENCODE_LIST:
        return "LIST";
    case BENCODE_DICT:
        return "DICT";
    default:
        assert(0 && "Invalid bencode type");
    }
}

// parsing a type of bencode may need to parse other types of bencode
// so we need to declare the parsing functions before we define them

static bencode_node_t* bencode_parse_str(const char* data, const char** endptr);
static bencode_node_t* bencode_parse_int(const char* data, const char** endptr);
static bencode_node_t* bencode_parse_list(const char*  data,
                                          const char** endptr);
static bencode_node_t* bencode_parse_dict(const char*  data,
                                          const char** endptr);

static bencode_node_t* bencode_parse_str(const char*  data,
                                         const char** endptr) {
    if (data == NULL) {
        LOG_WARN("Must provide a string to parse");
        return NULL;
    }

    uint64_t len = strtoull(data, (char**)endptr, 10);
    assert(**endptr == ':');
    (*endptr)++;

    LOG_DEBUG("Parsing string of length %lu", len);

    bencode_node_t* node = bencode_node_create(BENCODE_STR);
    if (node == NULL) {
        return NULL;
    }

    node->value.s = byte_str_create((uint8_t*)*endptr, len);
    if (node->value.s == NULL) {
        free(node);
        return NULL;
    }

    *endptr += len;
    return node;
}

static bencode_node_t* bencode_parse_int(const char*  data,
                                         const char** endptr) {
    if (data == NULL) {
        LOG_WARN("Must provide an integer to parse");
        return NULL;
    }

    assert(*data == 'i');
    data++;

    LOG_DEBUG("Parsing integer");

    int64_t i = strtoll(data, (char**)endptr, 10);
    assert(**endptr == 'e');
    (*endptr)++;

    bencode_node_t* node = bencode_node_create(BENCODE_INT);
    if (node == NULL) {
        return NULL;
    }
    node->value.i = i;

    return node;
}

static bencode_node_t* bencode_parse_list(const char*  data,
                                          const char** endptr) {
    if (data == NULL) {
        LOG_WARN("Must provide a list to parse");
        return NULL;
    }

    assert(*data == 'l');
    *endptr = data + 1;

    LOG_DEBUG("Parsing list");

    bencode_node_t* node = bencode_node_create(BENCODE_LIST);
    if (node == NULL) {
        return NULL;
    }

    node->value.l = list_create((list_free_data_fn_t)bencode_free);
    if (node->value.l == NULL) {
        free(node);
        return NULL;
    }

    while (**endptr != 'e') {
        data = *endptr;

        bencode_node_t* elem = bencode_parse(data, endptr);
        if (elem == NULL) {
            list_free(node->value.l);
            free(node);
            return NULL;
        }

        LOG_DEBUG("Adding element of type %s to list",
                  bencode_type_to_string(elem->type));

        if (list_push(node->value.l, elem, sizeof(bencode_node_t))) {
            list_free(node->value.l);
            bencode_free(elem);
            free(node);
            return NULL;
        }

        // pointer copied to the list, free the original
        free(elem);
    }

    assert(**endptr == 'e');
    (*endptr)++;

    return node;
}

static bencode_node_t* bencode_parse_dict(const char*  data,
                                          const char** endptr) {
    if (data == NULL) {
        LOG_WARN("Must provide a dictionary to parse");
        return NULL;
    }

    assert(*data == 'd');
    *endptr = data + 1;

    LOG_DEBUG("Parsing dictionary");

    bencode_node_t* node = bencode_node_create(BENCODE_DICT);
    if (node == NULL) {
        return NULL;
    }

    node->value.d = dict_create(1, (dict_free_data_fn_t)bencode_free);
    if (node->value.d == NULL) {
        free(node);
        return NULL;
    }

    while (**endptr != 'e') {
        data = *endptr;

        LOG_DEBUG("Parsing dictionary key");

        bencode_node_t* key = bencode_parse_str(data, endptr);
        if (key == NULL) {
            dict_free(node->value.d);
            free(node);
            return NULL;
        }

        data = *endptr;

        LOG_DEBUG("Parsing dictionary value");

        bencode_node_t* value = bencode_parse(data, endptr);
        if (value == NULL) {
            dict_free(node->value.d);
            free(node);
            return NULL;
        }

        LOG_DEBUG("Adding value of type %s to dictionary",
                  bencode_type_to_string(value->type));

        if (dict_add(node->value.d, (char*)key->value.s->data, value,
                     sizeof(bencode_node_t))) {
            dict_free(node->value.d);
            bencode_free(key);
            bencode_free(value);
            free(node);
            return NULL;
        }

        // free the key, we don't need it anymore
        bencode_free(key);

        // pointer copied to the dictionary, free the original
        free(value);
    }

    assert(**endptr == 'e');
    (*endptr)++;

    return node;
}

bencode_node_t* bencode_parse(const char* data, const char** endptr) {
    if (data == NULL) {
        LOG_WARN("Must provide a bencode string to parse");
        return NULL;
    }

    bencode_node_t* node = NULL;
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

    // NOTE: calculate the hash only for the info dictionary?
    if (node != NULL) {
        sha1((uint8_t*)data, *endptr - data, node->digest);
    }

    return node;
}

void bencode_free(bencode_node_t* node) {
    if (node == NULL) {
        LOG_WARN("Trying to free NULL bencode node");
        return;
    }

    switch (node->type) {
    case BENCODE_INT:
        LOG_DEBUG("Freeing integer");
        break;
    case BENCODE_STR:
        LOG_DEBUG("Freeing string");
        free(node->value.s);
        break;
    case BENCODE_LIST:
        LOG_DEBUG("Freeing list");
        list_free(node->value.l);
        break;
    case BENCODE_DICT:
        LOG_DEBUG("Freeing dictionary");
        dict_free(node->value.d);
        break;
    default:
        assert(0 && "Invalid bencode type");
    }

    free(node);
}
