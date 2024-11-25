#include "log.h"
#include "peer.h"
#include "torrent.h"
#include "torrent_file.h"
#include "tracker.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

const char *shift_args(int *argc, char ***argv) {
    const char *arg = NULL;
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

    const char *program_name = shift_args(&argc, &argv); // skip program name

    const char *torrent_file = NULL;
    const char *output_path = NULL;

    while (argc > 0) {
        const char *arg = shift_args(&argc, &argv);
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

    tracker_req_t *req = tracker_request_create(torrent, 6881);
    if (req == NULL) {
        LOG_ERROR("Failed to create tracker request");
        torrent_free(torrent);
        return 1;
    }

    tracker_res_t *res = tracker_announce(req, torrent->announce);
    if (res == NULL) {
        LOG_ERROR("Failed to announce to tracker");
        tracker_request_free(req);
        torrent_free(torrent);
        return 1;
    }
    tracker_request_free(req);

    LOG_INFO("Tracker response:");
    LOG_INFO("  Failure reason: %s", res->failure_reason);
    LOG_INFO("  Warning message: %s", res->warning_message);
    LOG_INFO("  Interval: %d", res->interval);
    LOG_INFO("  Min interval: %d", res->min_interval);
    LOG_INFO("  Tracker ID: %s", res->tracker_id);
    LOG_INFO("  Complete: %d", res->complete);
    LOG_INFO("  Incomplete: %d", res->incomplete);

    LOG_INFO("  Peers:");
    if (will_log(LOG_LEVEL_INFO)) {
        for (const list_iterator_t *it = list_iterator_first(res->peers); it != NULL; it = list_iterator_next(it)) {
            peer_t *peer = list_iterator_get(it);
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &peer->addr.sin_addr, ip, INET_ADDRSTRLEN);
            LOG_INFO("    %s:%d", ip, ntohs(peer->addr.sin_port));
        }
    }

    peer_t *peer = list_at(res->peers, 0);

    int sockfd;
    if ((sockfd = peer_connection_create(torrent, peer)) <= 0) {
        LOG_ERROR("Failed to connect to peer");
        tracker_response_free(res);
        torrent_free(torrent);
        return 1;
    }

    LOG_INFO("Connected to peer %s:%d", inet_ntoa(peer->addr.sin_addr), ntohs(peer->addr.sin_port));

    if (download_piece(torrent, peer, sockfd, 0) == -1) {
        LOG_ERROR("Failed to download piece");
        close(sockfd);
        tracker_response_free(res);
        torrent_free(torrent);
        return 1;
    }

    LOG_INFO("Piece %d/%d downloaded successfully", 1, torrent->num_pieces);

    close(sockfd);
    tracker_response_free(res);
    torrent_free(torrent);
    return 0;
}
