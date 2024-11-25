#ifndef LIST_H
#define LIST_H

#include <stdlib.h>

typedef struct list list_t;

typedef void (*list_free_data_fn_t)(void *);

/**
 * @brief Create a new list
 * 
 * @param data_free The function to free the data in the list
 * @return list_t* The list
 */
list_t *list_create(list_free_data_fn_t data_free);

/**
 * @brief Free the list
 * 
 * @param list The list
 */
void list_free(list_t *list);

/**
 * @brief Add an element to the list
 * @detail The value stored is a copy of the given value.
 * If the value is a pointer, the caller should free it.
 *
 * WARN: In case the value stores a pointer, the caller should
 * not free the pointer outside of the list, but free it with
 * the data_free function provided in list_create.
 * 
 * @param list The list
 * @param element The element
 * @param size The size of the element
 * @return int 0 if successful, -1 otherwise
 */
int list_push(list_t *list, void *element, size_t size);

/**
 * @brief Get an element from the list
 * 
 * @param list The list
 * @param index The index
 * @return void* The element
 */
void *list_at(list_t *list, size_t index);

/**
 * @brief Check if the list contains an element
 * 
 * @param list The list
 * @param element The element
 * @return int 1 if the element is in the list, 0 otherwise
 */
int list_contains(list_t *list, void *element);

/**
 * @brief Get an element from the list
 * @detail This function removes the element from the list
 * but it does not free its data
 * 
 * @param list The list
 * @param index The index of the element
 * @return void* The element
 */
void *list_remove(list_t *list, size_t index);

/**
 * @brief Get the size of the list
 * 
 * @param list The list
 * @return size_t The size of the list
 */
size_t list_size(list_t *list);

typedef void list_iterator_t;

/**
 * @brief Create a new list iterator at the first element
 * 
 * @param list The list
 * @return list_iterator_t* The list iterator
 */
const list_iterator_t *list_iterator_first(list_t *list);

/**
 * @brief Get the next element from the list iterator
 * 
 * @param list_iterator_t* The list iterator
 * @return list_iterator_t* The next element
 */
const list_iterator_t *list_iterator_next(const list_iterator_t *iterator);

/**
 * @brief Get the current element from the list iterator
 * 
 * @param list_iterator_t* The list iterator
 * @return void* The current element
 */
void *list_iterator_get(const list_iterator_t *iterator);

#endif // !LIST_H
