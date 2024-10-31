#include "list.h"

#include <string.h>

typedef struct list_node {
    void *data;
    size_t size;
    struct list_node *next;
} list_node_t;

struct list {
    list_node_t *head;
    list_node_t *tail;
    size_t size;
};

static list_node_t *list_node_create(void *data, size_t size) {
    list_node_t *node = malloc(sizeof(list_node_t));
    if (node == NULL) {
        return NULL;
    }

    node->data = malloc(size);
    if (node->data == NULL) {
        free(node);
        return NULL;
    }

    memcpy(node->data, data, size);
    node->size = size;
    node->next = NULL;
    return node;
}

list_t *list_create(void) {
    list_t *list = malloc(sizeof(list_t));
    if (list == NULL) {
        return NULL;
    }

    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    return list;
}

void list_free(list_t *list) {
    if (list == NULL) {
        return;
    }

    list_node_t *current = list->head;
    while (current != NULL) {
        list_node_t *next = current->next;
        free(current->data);
        free(current);
        current = next;
    }

    free(list);
}

int list_add(list_t *list, void *element, size_t size) {
    if (list == NULL) {
        return -1;
    }

    list_node_t *node = list_node_create(element, size);
    if (node == NULL) {
        return -1;
    }

    if (list->head == NULL) {
        list->head = node;
        list->tail = node;
    } else {
        list->tail->next = node;
        list->tail = node;
    }

    list->size++;
    return 0;
}

void *list_at(list_t *list, size_t index) {
    if (list == NULL || index >= list->size) {
        return NULL;
    }

    list_node_t *current = list->head;
    for (size_t i = 0; i < index; ++i) {
        current = current->next;
    }

    return current->data;
}

int list_contains(list_t *list, void *element) {
    if (list == NULL) {
        return 0;
    }

    list_node_t *current = list->head;
    while (current != NULL) {
        if (memcmp(current->data, element, current->size) == 0) {
            return 1;
        }
        current = current->next;
    }

    return 0;
}

int list_remove(list_t *list, size_t index) {
    if (list == NULL || index >= list->size) {
        return -1;
    }

    list_node_t *current = list->head;
    list_node_t *prev = NULL;
    for (size_t i = 0; i < index; ++i) {
        prev = current;
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

    free(current->data);
    free(current);
    list->size--;
    return 0;
}

size_t list_size(list_t *list) {
    return list->size;
}

const list_iterator_t *list_iterator_first(list_t *list) {
    return list->head;
}

const list_iterator_t *list_iterator_next(list_iterator_t *iterator) {
    return ((list_node_t *)iterator)->next;
}

void *list_iterator_get(list_iterator_t *iterator) {
    return ((list_node_t *)iterator)->data;
}
