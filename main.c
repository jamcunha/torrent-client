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
    printf("  -o <output path>   Output path (TODO)\n");
    printf("  -h                 Show this help\n");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        helper(argv[0]);
        return 1;
    }

    char *program_name = shift_args(&argc, &argv); // skip program name

    char *torrent_file = NULL;
    char *output_path = NULL;

    // WARN: only for not getting an unsed variable warning
    output_path = output_path ? output_path : "test";

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
        printf("Missing torrent file\n");
        helper(program_name);
        return 1;
    }

    printf("Torrent file: %s\n", torrent_file);

    return 0;
}
