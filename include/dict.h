#ifndef DICT_H
#define DICT_H

#include "stdlib.h"

typedef struct dict dict_t;

/**
 * @brief Create a new dictionary (HashMap)
 *
 * @param capacity The initial capacity of the dictionary
 * @return dict_t* The dictionary
 */
dict_t *dict_create(size_t capacity);

/**
 * @brief Free the dictionary
 * 
 * @param dict The dictionary
 */
void dict_free(dict_t *dict);

/**
 * @brief Add a new key-value pair to the dictionary
 * 
 * @param dict The dictionary
 * @param key The key
 * @param value The value
 * @param size The size of the value
 * @return int 0 if successful, -1 otherwise
 */
int dict_add(dict_t *dict, const char *key, void *value, size_t size);

/**
 * @brief Get the value of a key
 * 
 * @param dict The dictionary
 * @param key The key
 * @return void* The value of the key
 */
void *dict_get(dict_t *dict, const char *key);

/**
 * @brief Remove a key from the dictionary
 * 
 * @param dict The dictionary
 * @param key The key
 * @return int 0 if successful, -1 otherwise
 */
int dict_remove(dict_t *dict, const char *key);

/**
 * @brief Resize the dictionary
 *
 * This function is called when the load factor of the dictionary
 * exceeds a certain threshold. The new capacity is calculated by
 * multiplying the current capacity by 2.
 *
 * The dictionary is then rehashed and all the entries are inserted
 * into the new dictionary.
 * 
 * @param dict The dictionary
 * @param new_capacity The new capacity
 * @return int 0 if successful, -1 otherwise
 */
int dict_resize(dict_t *dict, size_t new_capacity);

/**
 * @brief Get the size of the dictionary
 * 
 * @param dict The dictionary
 * @return size_t The size of the dictionary
 */
size_t dict_size(dict_t *dict);

#endif // !DICT_H
