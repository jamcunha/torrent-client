#include "bencode.h"
#include <stdio.h>

int main(void) {
    char *data = "d3:fool1:a1:be3:bari-42ee";
    const char *endptr = data;

    bencode_node_t *node = bencode_parse(data, &endptr);
    if (node == NULL) {
        printf("Failed to parse bencode string\n");
        return 1;
    }

    // print the node
    list_t *l = ((bencode_node_t *)dict_get(node->value.d, "foo"))->value.l;
    printf("Dictionary:\n");
    printf("  foo:\n");
    printf("    List:\n");
    printf("      0: %s\n", ((bencode_node_t *)list_at(l, 0))->value.s);
    printf("      1: %s\n", ((bencode_node_t *)list_at(l, 1))->value.s);
    printf("  bar: %ld\n", ((bencode_node_t *)dict_get(node->value.d, "bar"))->value.i);

    bencode_free(node);
    return 0;
}
