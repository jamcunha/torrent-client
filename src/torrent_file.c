#include "torrent_file.h"

#include <stdio.h>

bencode_node_t *torrent_file_parse(const char *filename) {
    if (filename == NULL) {
        return NULL;
    }

    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *data = malloc(size + 1);
    if (data == NULL) {
        fclose(fp);
        return NULL;
    }

    size_t read = fread(data, 1, size, fp);
    if (read != size) {
        free(data);
        fclose(fp);
        return NULL;
    }

    data[size] = '\0';
    const char *endptr = data;

    bencode_node_t *node = bencode_parse(data, &endptr);
    if (node == NULL) {
        free(data);
        fclose(fp);
        return NULL;
    }

    free(data);
    fclose(fp);
    return node;
}
