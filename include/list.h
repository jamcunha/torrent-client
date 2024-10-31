#ifndef LIST_H
#define LIST_H

#include "stdlib.h"

typedef struct list list_t;

/**
 * @brief Create a new list
 * 
 * @return list_t* The list
 */
list_t *list_create(void);

/**
 * @brief Free the list
 * 
 * @param list The list
 */
void list_free(list_t *list);

/**
 * @brief Add an element to the list
 * 
 * @param list The list
 * @param element The element
 * @param size The size of the element
 * @return int 0 if successful, -1 otherwise
 */
int list_add(list_t *list, void *element, size_t size);

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
 * 
 * @param list The list
 * @param index The index of the element
 * @return void* The element
 */
int list_remove(list_t *list, size_t index);

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
 * @param iterator The list iterator
 * @return void* The next element
 */
const list_iterator_t *list_iterator_next(list_iterator_t *iterator);

/**
 * @brief Get the current element from the list iterator
 * 
 * @param iterator The list iterator
 * @return void* The current element
 */
void *list_iterator_get(list_iterator_t *iterator);

#endif // !LIST_H
