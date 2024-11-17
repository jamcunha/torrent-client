#include "torrent.h"
#include "bencode.h"
#include "dict.h"
#include "file.h"
#include "list.h"
#include "log.h"
#include "sha1.h"

#include <stdlib.h>
#include <string.h>

static void torrent_file_free(file_t **file_ptr) {
    if (file_ptr == NULL) {
        LOG_WARN("[torrent.c] Must provide a file pointer");
        return;
    }

    file_free(*file_ptr);
    free(file_ptr);
}

static void torrent_free_pieces(torrent_t *torrent) {
    if (torrent == NULL) {
        LOG_WARN("[torrent.c] Must provide a torrent");
        return;
    }

    if (torrent->pieces != NULL) {
        for (size_t i = 0; i < torrent->num_pieces; ++i) {
            free(torrent->pieces[i]);
        }
        free(torrent->pieces);
    }

    torrent->pieces = NULL;
    torrent->num_pieces = 0;
}

static uint8_t **torrent_create_pieces_array(torrent_t *torrent, byte_str_t *pieces_str) {
    if (torrent == NULL) {
        LOG_WARN("[torrent.c] Must provide a torrent");
        return NULL;
    }

    if (pieces_str == NULL) {
        LOG_WARN("[torrent.c] Must provide a pieces byte string");
        return NULL;
    }

    if (pieces_str->len % SHA1_DIGEST_SIZE != 0) {
        LOG_WARN("[torrent.c] Invalid pieces byte string, len %zu is not a multiple of %d", pieces_str->len, SHA1_DIGEST_SIZE);
        return NULL;
    }

    torrent->num_pieces = pieces_str->len / SHA1_DIGEST_SIZE;
    torrent->pieces_left = torrent->num_pieces;

    uint8_t **pieces = malloc(torrent->num_pieces * sizeof(uint8_t *));
    if (pieces == NULL) {
        LOG_ERROR("[torrent.c] Failed to allocate memory for pieces");
        return NULL;
    }

    for (size_t i = 0; i < torrent->num_pieces; ++i) {
        pieces[i] = malloc(SHA1_DIGEST_SIZE);
        if (pieces[i] == NULL) {
            LOG_ERROR("[torrent.c] Failed to allocate memory for piece %zu", i);
            for (size_t j = 0; j < i; ++j) {
                free(pieces[j]);
            }
            free(pieces);
            return NULL;
        }
        memcpy(pieces[i], pieces_str->data + i * SHA1_DIGEST_SIZE, SHA1_DIGEST_SIZE);
    }

    return pieces;
}

static int torrent_get_files(torrent_t *torrent, bencode_node_t *files_node, const char *output_path) {
    if (torrent == NULL) {
        LOG_WARN("[torrent.c] Must provide a torrent");
        return -1;
    }

    if (files_node == NULL) {
        LOG_WARN("[torrent.c] Must provide a files node");
        return -1;
    }

    if (output_path == NULL) {
        LOG_WARN("[torrent.c] Must provide an output path");
        return -1;
    }

    // create the output directory if it does not exist
    if (!dir_exists(output_path)) {
        if (create_dir(output_path)) {
            LOG_ERROR("[torrent.c] Failed to create output directory");
            return -1;
        }
    } else {
        // TODO: should we return if the directory already exists?
        LOG_ERROR("[torrent.c] Output directory already exists");
        return -1;
    }

    list_t *files = files_node->value.l;
    for (const list_iterator_t *it = list_iterator_first(files); it != NULL; it = list_iterator_next(it)) {
        bencode_node_t *file_node = (bencode_node_t *)list_iterator_get(it);
        if (file_node == NULL) {
            LOG_WARN("[torrent.c] Missing file node in files list");
            return -1;
        }

        dict_t *file_dict = file_node->value.d;
        bencode_node_t *length_node = (bencode_node_t *)dict_get(file_dict, "length");
        if (length_node == NULL) {
            LOG_WARN("[torrent.c] Missing length key in file dictionary");
            return -1;
        }

        int64_t file_length = length_node->value.i;

        bencode_node_t *path_node = (bencode_node_t *)dict_get(file_dict, "path");
        if (path_node == NULL) {
            LOG_WARN("[torrent.c] Missing path key in file dictionary");
            return -1;
        }

        char path[512];
        strcpy(path, output_path);
        size_t path_len = strlen(output_path);

        list_t *path_list = path_node->value.l;
        size_t i = 0; // used to create subdirectories
        for (const list_iterator_t *it = list_iterator_first(path_list); it != NULL; it = list_iterator_next(it)) {
            bencode_node_t *path_part_node = (bencode_node_t *)list_iterator_get(it);
            if (path_part_node == NULL) {
                LOG_WARN("[torrent.c] Missing path part node in path list");
                return -1;
            }

            char *path_part = (char *)path_part_node->value.s->data;
            size_t part_len = strlen(path_part);
            if (path_len + part_len + 1 > 512) {
                LOG_WARN("[torrent.c] Path is too long: %s", path);
                return -1;
            }

            if (path_len > 0) {
                path[path_len++] = '/';
            }

            strcpy(path + path_len, path_part);
            path_len += part_len;

            if (i < list_size(path_list) - 1) {
                if (create_dir(path)) {
                    LOG_ERROR("[torrent.c] Failed to create subdirectory `%s`", path);
                    return -1;
                }
            }

            ++i;
        }

        LOG_DEBUG("[torrent.c] Adding file `%s` with length %zu", path, (size_t)file_length);

        file_t *file = file_create(path, (size_t)file_length);
        if (file == NULL) {
            return -1;
        }

        if (list_push(torrent->files, &file, sizeof(file_t *))) {
            file_free(file);
            return -1;
        }
    }

    return 0;
}

static int torrent_get_info(torrent_t *torrent, dict_t *info, const char *output_path) {
    if (torrent == NULL) {
        LOG_WARN("[torrent.c] Must provide a torrent");
        return -1;
    }

    if (info == NULL) {
        LOG_WARN("[torrent.c] Must provide an info dictionary");
        return -1;
    }

    if (output_path == NULL) {
        LOG_WARN("[torrent.c] Must provide an output path");
        return -1;
    }

    byte_str_t *pieces_str = (byte_str_t *)((bencode_node_t *)dict_get(info, "pieces"))->value.s;
    if (pieces_str == NULL) {
        LOG_WARN("[torrent.c] Missing pieces key in torrent info");
        return -1;
    }
    torrent->pieces = torrent_create_pieces_array(torrent, pieces_str);

    LOG_DEBUG("[torrent.c] Torrent has %zu pieces", torrent->num_pieces);

    int64_t piece_length = (int64_t)((bencode_node_t *)dict_get(info, "piece length"))->value.i;
    if (piece_length == 0) {
        LOG_WARN("[torrent.c] Missing piece length key in torrent info");
        torrent_free_pieces(torrent);
        return -1;
    }
    torrent->piece_length = (size_t)piece_length;

    LOG_DEBUG("[torrent.c] Torrent has piece length of %zu bytes", torrent->piece_length);

    bencode_node_t *name_node = (bencode_node_t *)dict_get(info, "name");
    if (name_node == NULL) {
        LOG_WARN("[torrent.c] Missing name key in torrent info");
        torrent_free_pieces(torrent);
        return -1;
    }
    char *name = (char *)name_node->value.s->data;

    char *path = calloc(strlen(output_path) + strlen(name) + 2, sizeof(char));
    if (path == NULL) {
        LOG_ERROR("[torrent.c] Failed to allocate memory for file path");
        torrent_free_pieces(torrent);
        return -1;
    }

    if (output_path[strlen(output_path) - 1] == '/') {
        sprintf(path, "%s%s", output_path, name);
    } else {
        sprintf(path, "%s/%s", output_path, name);
    }

    bencode_node_t *length_node = (bencode_node_t *)dict_get(info, "length");
    if (length_node != NULL) {
        LOG_DEBUG("[torrent.c] Torrent has a single file with name `%s`", name);

        int64_t file_length = length_node->value.i;

        file_t *file = file_create(path, (size_t)file_length);
        if (file == NULL) {
            torrent_free_pieces(torrent);
            free(path);
            return -1;
        }

        // Free path since it is copied into the file object
        free(path);

        if (list_push(torrent->files, &file, sizeof(file_t *))) {
            torrent_free_pieces(torrent);
            file_free(file);
            return -1;
        }

        return 0;
    }

    LOG_DEBUG("[torrent.c] Torrent has multiple files");

    bencode_node_t *files_node = (bencode_node_t *)dict_get(info, "files");
    if (files_node == NULL) {
        LOG_WARN("[torrent.c] Missing files key in torrent info");
        torrent_free_pieces(torrent);
        return -1;
    }

    if (torrent_get_files(torrent, files_node, path)) {
        torrent_free_pieces(torrent);
        free(path);
        return -1;
    }

    free(path);
    return 0;
}

torrent_t *torrent_create(bencode_node_t *node, const char *output_path) {
    if (node == NULL) {
        LOG_WARN("[torrent.c] Must provide a bencode node");
        return NULL;
    }

    if (output_path == NULL) {
        LOG_WARN("[torrent.c] Must provide an output path");
        return NULL;
    }

    torrent_t *torrent = malloc(sizeof(torrent_t));
    if (torrent == NULL) {
        LOG_ERROR("[torrent.c] Failed to allocate memory for torrent");
        return NULL;
    }

    torrent->max_peers = TORRENT_DEFAULT_MAX_PEERS;
    torrent->total_down = 0;
    
    LOG_DEBUG("[torrent.c] Getting announce key from torrent file");

    bencode_node_t *announce_node = (bencode_node_t *)dict_get(node->value.d, "announce");
    if (announce_node == NULL) {
        LOG_WARN("[torrent.c] Missing announce key in torrent file");
        free(torrent);
        return NULL;
    }

    char *announce = (char *)announce_node->value.s->data;
    if (announce == NULL) {
        LOG_WARN("[torrent.c] Missing announce key in torrent file");
        free(torrent);
        return NULL;
    }

    torrent->announce = calloc(strlen(announce) + 1, sizeof(char));
    if (torrent->announce == NULL) {
        LOG_ERROR("[torrent.c] Failed to allocate memory for announce");
        free(torrent);
        return NULL;
    }
    strcpy(torrent->announce, announce);

    LOG_DEBUG("[torrent.c] Getting creation date key from torrent file");

    bencode_node_t *creation_date_node = (bencode_node_t *)dict_get(node->value.d, "creation date");
    if (creation_date_node != NULL) {
        torrent->creation_date = (uint32_t)creation_date_node->value.i;
    } else {
        LOG_DEBUG("[torrent.c] Missing creation date key in torrent file");
        torrent->creation_date = 0;
    }

    LOG_DEBUG("[torrent.c] Getting comment key from torrent file");

    bencode_node_t *comment_node = (bencode_node_t *)dict_get(node->value.d, "comment");
    if (comment_node != NULL) {
        char *comment = (char *)comment_node->value.s->data;
        torrent->comment = calloc(strlen(comment) + 1, sizeof(char));
        if (torrent->comment == NULL) {
            LOG_ERROR("[torrent.c] Failed to allocate memory for comment");
            free(torrent->announce);
            free(torrent);
            return NULL;
        }

        strcpy(torrent->comment, comment);
    } else {
        LOG_DEBUG("[torrent.c] Missing comment key in torrent file");
        torrent->comment = NULL;
    }

    LOG_DEBUG("[torrent.c] Getting created by key from torrent file");

    bencode_node_t *created_by_node = (bencode_node_t *)dict_get(node->value.d, "created by");
    if (created_by_node != NULL) {
        char *created_by = (char *)created_by_node->value.s->data;
        torrent->created_by = calloc(strlen(created_by) + 1, sizeof(char));
        if (torrent->created_by == NULL) {
            LOG_ERROR("[torrent.c] Failed to allocate memory for created by");
            free(torrent->comment);
            free(torrent->announce);
            free(torrent);
            return NULL;
        }

        strcpy(torrent->created_by, created_by);
    } else {
        LOG_DEBUG("[torrent.c] Missing created by key in torrent file");
        torrent->created_by = NULL;
    }

    LOG_DEBUG("[torrent.c] Getting info key from torrent file");

    bencode_node_t *info = (bencode_node_t *)dict_get(node->value.d, "info");
    if (info == NULL) {
        LOG_WARN("[torrent.c] Missing info key in torrent file");
        free(torrent->created_by);
        free(torrent->comment);
        free(torrent->announce);
        free(torrent);
        return NULL;
    }

    memcpy(torrent->info_hash, info->digest, SHA1_DIGEST_SIZE);

    torrent->files = list_create((void (*)(void *))torrent_file_free);
    if (torrent->files == NULL) {
        LOG_ERROR("[torrent.c] Failed to create files list");
        free(torrent->created_by);
        free(torrent->comment);
        free(torrent->announce);
        free(torrent);
        return NULL;
    }

    if (torrent_get_info(torrent, info->value.d, output_path)) {
        list_free(torrent->files);
        free(torrent->created_by);
        free(torrent->comment);
        free(torrent->announce);
        free(torrent);
        return NULL;
    }

    return torrent;
}

void torrent_free(torrent_t *torrent) {
    if (torrent == NULL) {
        LOG_WARN("[torrent.c] Trying to free NULL torrent");
        return;
    }

    LOG_DEBUG("[torrent.c] Freeing torrent object");

    free(torrent->announce);
    free(torrent->comment);
    free(torrent->created_by);

    torrent_free_pieces(torrent);
    list_free(torrent->files);
    free(torrent);
}
