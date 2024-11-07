#include "torrent_file.h"
#include "log.h"

#include <stdio.h>

bencode_node_t *torrent_file_parse(const char *filename) {
    if (filename == NULL) {
        LOG_WARN("Must provide a filename");
        return NULL;
    }

    LOG_DEBUG("Opening file `%s`", filename);

    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        LOG_ERROR("Failed to open file `%s` in read mode", filename);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    LOG_DEBUG("Reading file `%s` of size %zu", filename, size);

    char *data = malloc(size + 1);
    if (data == NULL) {
        LOG_ERROR("Failed to allocate memory for buffer of size %zu", size);
        fclose(fp);
        return NULL;
    }

    size_t read = fread(data, 1, size, fp);
    if (read != size) {
        LOG_ERROR("Failed to read file `%s`", filename);
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

    LOG_DEBUG("Closing file `%s`", filename);

    free(data);
    fclose(fp);
    return node;
}
