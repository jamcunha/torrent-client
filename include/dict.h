#ifndef DICT_H
#define DICT_H

#include <stdlib.h>

typedef struct dict dict_t;

/**
 * @brief Create a new dictionary (HashMap)
 *
 * @param capacity The initial capacity of the dictionary
 * @param data_free The function to free the values in the dictionary
 * @return dict_t* The dictionary
 */
dict_t *dict_create(size_t capacity, void (*value_free)(void *));

/**
 * @brief Free the dictionary
 * @detail This function frees the dictionary and it's entries,
 * but it does not free the data in the entries
 * 
 * @param dict The dictionary
 */
void dict_free(dict_t *dict);

/**
 * @brief Add a new key-value pair to the dictionary
 * @detail The value stored is a copy of the given value.
 * If the value is a pointer, the caller should free it.
 *
 * WARN: In case the value stores a pointer, the caller should
 * not free the pointer outside of the dictionary, but free it with
 * the value_free function provided in dict_create.
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
 * @detail This function removes the key from the dictionary
 * and returns the value of the key. The value is not freed.
 * 
 * @param dict The dictionary
 * @param key The key
 * @return void* The value of the key
 */
void *dict_remove(dict_t *dict, const char *key);

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

typedef void dict_iterator_t;

/**
 * @brief Get the first element of the dictionary
 *
 * @param dict The dictionary
 * @return dict_iterator_t* The dictionary iterator
 */
const dict_iterator_t *dict_iterator_first(dict_t *dict);

/**
 * @brief Get the current element of the dictionary iterator
 *
 * @param iterator The dictionary iterator
 * @return void* The current element
 */
const dict_iterator_t *dict_iterator_next(dict_t *dict, const dict_iterator_t *iterator);

/**
 * @brief Get the key of the current element of the dictionary iterator
 *
 * @param iterator The dictionary iterator
 * @return const char* The key of the current element
 */
const char *dict_iterator_key(const dict_iterator_t *iterator);

/**
 * @brief Get the value of the current element of the dictionary iterator
 *
 * @param iterator The dictionary iterator
 * @return void* The value of the current element
 */
void *dict_iterator_value(const dict_iterator_t *iterator);

#endif // !DICT_H
