#include "file.h"
#include "log.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

struct file {
    size_t size;
    char path[];
};

file_t *file_create(const char *path, size_t size) {
    if (path == NULL) {
        LOG_WARN("[file.c] Must provide a path to create a file");
        return NULL;
    }

    file_t *file = malloc(sizeof(file_t) + strlen(path) + 1);
    if (file == NULL) {
        LOG_ERROR("[file.c] Failed to allocate memory for file");
        return NULL;
    }

    LOG_DEBUG("[file.c] Creating file `%s` with size %zu", path, size);

    strcpy(file->path, path);
    file->size = size;

    // Prealloc file with zeros

    uint8_t data[4096] = {0};
    FILE *fp = fopen(path, "wb");
    if (fp == NULL) {
        LOG_ERROR("[file.c] Failed to open file `%s` in write mode", path);
        free(file);
        return NULL;
    }

    for (size_t i = 0; i < size; i += 4096) {
        size_t chunk_size = 4096;
        if (i + chunk_size > size) {
            chunk_size = size - i;
        }

        if (fwrite(data, 1, chunk_size, fp) != chunk_size) {
            LOG_ERROR("[file.c] Failed to write to file `%s`", path);
            free(file);
            fclose(fp);
            return NULL;
        }
    }

    fseek(fp, 0, SEEK_END);
    size_t written = ftell(fp);
    if (written != size) {
        LOG_ERROR("[file.c] Failed to write to file `%s`, wrote %zu bytes, expected %zu bytes", path, written, size);
        free(file);
        fclose(fp);
        return NULL;
    }

    fclose(fp);
    return file;
}

size_t get_file_size(const file_t *file) {
    if (file == NULL) {
        LOG_WARN("[file.c] Must provide a file to get its size");
        return 0;
    }

    return file->size;
}

const char *get_file_path(const file_t *file) {
    if (file == NULL) {
        LOG_WARN("[file.c] Must provide a file to get its path");
        return NULL;
    }

    return file->path;
}

int write_data_to_file(file_t *file, size_t offset, uint8_t *data, size_t len) {
    if (file == NULL || data == NULL) {
        LOG_WARN("[file.c] Must provide a file and data to write to the file");
        return -1;
    }

    if (offset + len > file->size) {
        LOG_ERROR("[file.c] Attempted to write data past the end of the file");
        return -1;
    }

    FILE *fp = fopen(file->path, "r+b");
    if (fp == NULL) {
        LOG_ERROR("[file.c] Failed to open file `%s` in read/write mode", file->path);
        return -1;
    }

    if (fseek(fp, offset, SEEK_SET) != 0) {
        LOG_ERROR("[file.c] Failed to seek to offset %zu in file `%s`", offset, file->path);
        fclose(fp);
        return -1;
    }

    size_t total_written = 0;
    while(total_written < len) {
        size_t written = fwrite(data + total_written, sizeof(uint8_t), len - total_written, fp);
        if (written == 0) {
            LOG_ERROR("[file.c] Failed to write data to file `%s`", file->path);
            fclose(fp);
            return -1;
        }
        total_written += written;
    }

    assert(total_written == len && "Wrote more data than expected");

    fclose(fp);
    return 0;
}

bool dir_exists(const char *path) {
    struct stat sb;
    return stat(path, &sb) == 0 && S_ISDIR(sb.st_mode);
}

int create_dir(const char *path) {
    if (path == NULL) {
        LOG_WARN("[file.c] Must provide a path to create a directory");
        return -1;
    }

    if (dir_exists(path)) {
        LOG_DEBUG("[file.c] Directory `%s` already exists", path);
        return 0;
    }

    LOG_DEBUG("[file.c] Creating directory `%s`", path);

    // rwxr-xr-x
    return mkdir(path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
}
