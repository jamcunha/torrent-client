#include <stdio.h>
#include <string.h>

#include "dict.h"
#include "list.h"
#include "sha1.h"

int main(void) {
    printf("Testing dict\n");

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

    printf("Testing list\n");

    list_t *list = list_create();
    if (list == NULL) {
        fprintf(stderr, "Failed to create list\n");
        return 1;
    }

    char *to_add_list = "value1";
    if (list_add(list, to_add_list, strlen(to_add_list) + 1)) {
        fprintf(stderr, "Failed to add value1\n");
        return 1;
    }

    char *to_add_list2 = "value2";
    if (list_add(list, to_add_list2, strlen(to_add_list2) + 1)) {
        fprintf(stderr, "Failed to add value2\n");
        return 1;
    }

    char *v5 = list_at(list, 0);
    char *v6 = list_at(list, 1);

    printf("Value 5: %s\n", v5);
    printf("Value 6: %s\n", v6);

    if (list_remove(list, 0)) {
        fprintf(stderr, "Failed to remove value1\n");
        return 1;
    }

    char *v7 = list_at(list, 0);
    char *v8 = list_at(list, 1);

    printf("Value 7: %s\n", v7);
    printf("Value 8: %s\n", v8);

    list_free(list);

    printf("Testing sha1\n");

    sha1_ctx_t *ctx = sha1_create();

    char *data = "hello world";
    sha1_update(ctx, (const uint8_t *)data, strlen(data));

    uint8_t digest[SHA1_DIGEST_SIZE];
    sha1_final(ctx, digest);

    for (int i = 0; i < SHA1_DIGEST_SIZE; i++) {
        printf("%02x", digest[i]);
    }
    printf("\n");

    sha1_free(ctx);

    return 0;
}
