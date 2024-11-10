#include "log.h"
#include "torrent.h"
#include "torrent_file.h"

#include <stdio.h>
#include <string.h>

char *shift_args(int *argc, char ***argv) {
    char *arg = NULL;
    if (*argc > 0) {
        arg = (*argv)[0];
        (*argc)--;
        (*argv)++;
    }
    return arg;
}

void helper(const char *program_name) {
    printf("Usage: %s -t <torrent file> [-o <output path>]\n", program_name);
    printf("Options:\n");
    printf("  -t <torrent file>  Torrent file to download\n");
    printf("  -o <output path>   Output path [default: $XDG_DOWNLOAD_DIR]\n");
    printf("  -h                 Show this help\n");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Missing arguments\n\n");
        helper(argv[0]);
        return 1;
    }

    char *program_name = shift_args(&argc, &argv); // skip program name

    char *torrent_file = NULL;
    char *output_path = NULL;

    while (argc > 0) {
        char *arg = shift_args(&argc, &argv);
        if (arg == NULL) {
            break;
        }

        if (strcmp(arg, "-t") == 0) {
            torrent_file = shift_args(&argc, &argv);
        } else if (strcmp(arg, "-o") == 0) {
            output_path = shift_args(&argc, &argv);
        } else {
            printf("Unknown argument: %s\n", arg);
            helper(program_name);
            return 1;
        }
    }

    if (torrent_file == NULL) {
        printf("Missing torrent file\n\n");
        helper(program_name);
        return 1;
    }

    if (output_path == NULL) {
        output_path = getenv("XDG_DOWNLOAD_DIR");
        LOG_INFO("Using default output path: %s", output_path);
        if (output_path == NULL) {
            LOG_ERROR("Failed to get XDG_DOWNLOAD_DIR, please provide an output path\n");
            helper(program_name);
            return 1;
        }
    }

    LOG_INFO("Parsing torrent file: %s", torrent_file);

    bencode_node_t *node = torrent_file_parse(torrent_file);
    if (node == NULL) {
        LOG_ERROR("Failed to parse torrent file");
        return 1;
    }

    torrent_t *torrent = torrent_create(node, output_path);
    if (torrent == NULL) {
        LOG_ERROR("Failed to create torrent");
        bencode_free(node);
        return 1;
    }
    bencode_free(node);

    torrent_free(torrent);
    return 0;
}
