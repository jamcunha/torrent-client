#include "dict.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define FNV_OFFSET_BASIS_32 0x811c9dc5
#define FNV_OFFSET_BASIS_64 0xcbf29ce484222325

#define FNV_PRIME_32 0x01000193
#define FNV_PRIME_64 0x100000001b3

typedef struct dict_entry {
    char *key;
    void *value;
    size_t size;
    struct dict_entry *next;
} dict_entry_t;

struct dict {
    dict_entry_t **nodes;
    size_t size;
    size_t capacity;
};

static dict_entry_t *dict_entry_create(const char *key, void *value, size_t size) {
    dict_entry_t *entry = malloc(sizeof(dict_entry_t));
    if (entry == NULL) {
        return NULL;
    }

    entry->key = malloc(strlen(key) + 1);
    if (entry->key == NULL) {
        free(entry);
        return NULL;
    }
    strcpy(entry->key, key);

    entry->size = size;
    entry->next = NULL;

    entry->value = malloc(size);
    if (entry->value == NULL) {
        free(entry->key);
        free(entry);
        return NULL;
    }

    if (value != NULL) {
        memcpy(entry->value, value, size);
    }

    return entry;
}

static void dict_entry_free(dict_entry_t *entry) {
    if (entry == NULL) {
        return;
    }

    free(entry->key);
    free(entry->value);
    free(entry);
}

/**
 * @brief FNV-1a hash function
 * @see http://www.isthe.com/chongo/tech/comp/fnv/#FNV-1a
 *
 * Only supports 32bit and 64bit architectures
 *
 * @param key The key to hash
 * @return size_t The hash
 */
static size_t dict_hash(const char *key) {
    size_t hash;
    size_t prime;

    if (sizeof(size_t) == 4) {
        hash = FNV_OFFSET_BASIS_32;
        prime = FNV_PRIME_32;
    } else if (sizeof(size_t) == 8) {
        hash = FNV_OFFSET_BASIS_64;
        prime = FNV_PRIME_64;
    } else {
        assert(0 && "Unsupported architecture, only 32bit and 64bit are supported");
    }

    while (*key) {
        hash ^= (uint8_t)(*key++);
        hash *= prime;
    }

    return hash;
}

dict_t *dict_create(size_t capacity) {
    dict_t *dict = malloc(sizeof(dict_t));
    if (dict == NULL) {
        return NULL;
    }

    dict->nodes = calloc(capacity, sizeof(dict_entry_t *));
    if (dict->nodes == NULL) {
        free(dict);
        return NULL;
    }

    dict->size = 0;
    dict->capacity = capacity;

    return dict;
}

void dict_free(dict_t *dict) {
    if (dict == NULL) {
        return;
    }

    for (size_t i = 0; i < dict->capacity; ++i) {
        dict_entry_t *entry = dict->nodes[i];
        while (entry != NULL) {
            dict_entry_t *next = entry->next;
            dict_entry_free(entry);
            entry = next;
        }
    }

    free(dict->nodes);
    free(dict);
}

int dict_add(dict_t *dict, const char *key, void *value, size_t size) {
    if (dict == NULL) {
        return -1;
    }

    size_t idx = dict_hash(key) % dict->capacity;

    dict_entry_t *entry = dict_entry_create(key, value, size);
    if (entry == NULL) {
        return -1;
    }

    if (dict_get(dict, key) != NULL) {
        if (dict_remove(dict, key) == -1) {
            return -1;
        }
    }

    if (dict->nodes[idx] == NULL) {
        dict->nodes[idx] = entry;
    } else {
        dict_entry_t *current = dict->nodes[idx];
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = entry;
    }

    dict->size++;
    return 0;
}

void *dict_get(dict_t *dict, const char *key) {
    if (dict == NULL) {
        return NULL;
    }

    size_t idx = dict_hash(key) % dict->capacity;

    dict_entry_t *entry = dict->nodes[idx];
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }

    return NULL;
}

int dict_remove(dict_t *dict, const char *key) {
    if (dict == NULL) {
        return -1;
    }

    size_t idx = dict_hash(key) % dict->capacity;

    dict_entry_t *entry = dict->nodes[idx];
    dict_entry_t *prev = NULL;
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            if (prev == NULL) {
                dict->nodes[idx] = entry->next;
            } else {
                prev->next = entry->next;
            }

            dict_entry_free(entry);
            dict->size--;
            return 0;
        }
        prev = entry;
        entry = entry->next;
    }

    return 0;
}

int dict_resize(dict_t *dict, size_t new_capacity) {
    if (dict == NULL) {
        return -1;
    }

    dict_entry_t **new_nodes = calloc(new_capacity, sizeof(dict_entry_t *));
    if (new_nodes == NULL) {
        return -1;
    }

    for (size_t i = 0; i < dict->capacity; ++i) {
        dict_entry_t *entry = dict->nodes[i];
        dict_entry_t *prev = NULL;

        while (entry != NULL) {
            // Remove the entry from the old node
            if (prev != NULL) {
                prev->next = NULL;
            }

            size_t idx = dict_hash(entry->key) % new_capacity;

            if (new_nodes[idx] == NULL) {
                new_nodes[idx] = entry;
            } else {
                dict_entry_t *current = new_nodes[idx];
                while (current->next != NULL) {
                    current = current->next;
                }
                current->next = entry;
            }
        }
    }

    free(dict->nodes);
    dict->nodes = new_nodes;
    dict->capacity = new_capacity;
    return 0;
}

size_t dict_size(dict_t *dict) {
    return dict->size;
}
