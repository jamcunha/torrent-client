#include <stdio.h>
#include <string.h>

#include "dict.h"

int main(void) {
    printf("Hello, World!\n");

    dict_t *dict = dict_create(10);
    if (dict == NULL) {
        fprintf(stderr, "Failed to create dictionary\n");
        return 1;
    }

    char *to_add = "value1";
    if (dict_add(dict, "key1", to_add, strlen(to_add) + 1)) {
        fprintf(stderr, "Failed to add key1\n");
        return 1;
    }

    char *to_add2 = "value2";
    if (dict_add(dict, "key2", to_add2, strlen(to_add2) + 1)) {
        fprintf(stderr, "Failed to add key2\n");
        return 1;
    }

    char *v1 = dict_get(dict, "key1");
    char *v2 = dict_get(dict, "key2");

    printf("Value 1: %s\n", v1);
    printf("Value 2: %s\n", v2);

    if (dict_remove(dict, "key1")) {
        fprintf(stderr, "Failed to remove key1\n");
        return 1;
    }

    char *v3 = dict_get(dict, "key1");
    char *v4 = dict_get(dict, "key2");

    printf("Value 3: %s\n", v3);
    printf("Value 4: %s\n", v4);

    dict_free(dict);
    return 0;
}
