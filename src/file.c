#include "file.h"
#include "log.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FILE_CHUNK_SIZE 1024

struct file {
    size_t size;
    uint8_t *data;
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
    file->path[strlen(path)] = '\0';
    file->size = size;

    file->data = calloc(FILE_CHUNK_SIZE, sizeof(uint8_t));
    if (file->data == NULL) {
        LOG_ERROR("[file.c] Failed to allocate memory for file data");
        free(file);
        return NULL;
    }

    // Prealloc file with zeros

    FILE *fp = fopen(path, "wb");
    if (fp == NULL) {
        LOG_ERROR("[file.c] Failed to open file `%s` in write mode", path);
        free(file->data);
        free(file);
        return NULL;
    }

    for (size_t i = 0; i < size; i += FILE_CHUNK_SIZE) {
        size_t chunk_size = FILE_CHUNK_SIZE;
        if (i + chunk_size > size) {
            chunk_size = size - i;
        }

        if (fwrite(file->data, sizeof(uint8_t), chunk_size, fp) != chunk_size) {
            LOG_ERROR("[file.c] Failed to write to file `%s`", path);
            free(file->data);
            free(file);
            fclose(fp);
            return NULL;
        }
    }

    fseek(fp, 0, SEEK_END);
    size_t written = ftell(fp);
    if (written != size) {
        LOG_ERROR("[file.c] Failed to write to file `%s`, wrote %zu bytes, expected %zu bytes", path, written, size);
        free(file->data);
        free(file);
        fclose(fp);
        return NULL;
    }

    fclose(fp);
    return file;
}


void file_free(file_t *file) {
    if (file == NULL) {
        LOG_WARN("[file.c] Trying to free NULL file");
        return;
    }

    free(file->data);
    free(file);
}

#ifdef _WIN32
    #include <direct.h>
    #include <windows.h>

    #define mkdir _mkdir
#else
    #include <sys/stat.h>
    #include <sys/types.h>
#endif

bool dir_exists(const char *path) {
#ifdef _WIN32
    DWORD attrib = GetFileAttributes(path);
    return (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat sb;
    return stat(path, &sb) == 0 && S_ISDIR(sb.st_mode);
#endif
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

#ifdef _WIN32
    return _mkdir(path);
#else
    // rwxr-xr-x
    return mkdir(path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
#endif
}
