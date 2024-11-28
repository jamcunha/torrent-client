#include "list.h"

#include "log.h"

#include <string.h>

typedef struct list_node {
    void*             data;
    size_t            size;
    struct list_node* next;
} list_node_t;

struct list {
    list_node_t* head;
    list_node_t* tail;
    size_t       size;
    void         (*data_free)(void*);
};

static list_node_t* list_node_create(void* data, size_t size) {
    list_node_t* node = malloc(sizeof(list_node_t));
    if (node == NULL) {
        LOG_ERROR("[list.c] Failed to allocate memory for list node");
        return NULL;
    }

    node->data = malloc(size);
    if (node->data == NULL) {
        LOG_ERROR("[list.c] Failed to allocate memory for list node data");
        free(node);
        return NULL;
    }

    memcpy(node->data, data, size);
    node->size = size;
    node->next = NULL;
    return node;
}

list_t* list_create(list_free_data_fn_t data_free) {
    list_t* list = malloc(sizeof(list_t));
    if (list == NULL) {
        LOG_ERROR("[list.c] Failed to allocate memory for list");
        return NULL;
    }

    list->head      = NULL;
    list->tail      = NULL;
    list->size      = 0;
    list->data_free = data_free;
    return list;
}

void list_free(list_t* list) {
    if (list == NULL) {
        LOG_WARN("[list.c] Trying to free NULL list");
        return;
    }

    list_node_t* current = list->head;
    while (current != NULL) {
        list_node_t* next = current->next;

        if (list->data_free != NULL) {
            LOG_DEBUG(
                "[list.c] Freeing list node data with custom free function");
            list->data_free(current->data);
        } else {
            LOG_DEBUG(
                "[list.c] Freeing list node data with default free function");
            free(current->data);
        }

        free(current);
        current = next;
    }

    free(list);
}

int list_push(list_t* list, void* element, size_t size) {
    if (list == NULL) {
        LOG_WARN("[list.c] Trying to push element to NULL list");
        return -1;
    }

    list_node_t* node = list_node_create(element, size);
    if (node == NULL) {
        return -1;
    }

    if (list->head == NULL) {
        list->head = node;
        list->tail = node;
    } else {
        list->tail->next = node;
        list->tail       = node;
    }

    list->size++;
    return 0;
}

void* list_at(list_t* list, size_t index) {
    if (list == NULL) {
        LOG_WARN("[list.c] Trying to get element from NULL list");
        return NULL;
    }

    if (index >= list->size) {
        LOG_WARN("[list.c] Index out of bounds");
        return NULL;
    }

    list_node_t* current = list->head;
    for (size_t i = 0; i < index; ++i) {
        current = current->next;
    }

    return current->data;
}

int list_contains(list_t* list, void* element) {
    if (list == NULL) {
        LOG_WARN("[list.c] Trying to check if element is in NULL list");
        return 0;
    }

    list_node_t* current = list->head;
    while (current != NULL) {
        if (memcmp(current->data, element, current->size) == 0) {
            return 1;
        }
        current = current->next;
    }

    return 0;
}

void* list_remove(list_t* list, size_t index) {
    if (list == NULL) {
        LOG_WARN("[list.c] Trying to remove element from NULL list");
        return NULL;
    }

    if (index >= list->size) {
        LOG_WARN("[list.c] Index out of bounds");
        return NULL;
    }

    list_node_t* current = list->head;
    list_node_t* prev    = NULL;
    for (size_t i = 0; i < index; ++i) {
        prev    = current;
        current = current->next;
    }

    if (prev == NULL) {
        list->head = current->next;
    } else {
        prev->next = current->next;
    }

    if (list->tail == current) {
        list->tail = prev;
    }

    void* data = current->data;
    free(current);
    list->size--;
    return data;
}

size_t list_size(list_t* list) {
    return list->size;
}

const list_iterator_t* list_iterator_first(list_t* list) {
    return list->head;
}

const list_iterator_t* list_iterator_next(const list_iterator_t* iterator) {
    return ((list_node_t*)iterator)->next;
}

void* list_iterator_get(const list_iterator_t* iterator) {
    return ((list_node_t*)iterator)->data;
}
