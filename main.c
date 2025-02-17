#include "log.h"
#include "peer.h"
#include "torrent.h"
#include "tracker.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

const char* shift_args(int* argc, char*** argv) {
    const char* arg = NULL;
    if (*argc > 0) {
        arg = (*argv)[0];
        (*argc)--;
        (*argv)++;
    }
    return arg;
}

void helper(const char* program_name) {
    printf("Usage: %s -t <torrent file> [-o <output path>]\n", program_name);
    printf("Options:\n");
    printf("  -t <torrent file>  Torrent file to download\n");

    // NOTE: Should the path be shown instead of $XDG_DOWNLOAD_DIR?
    printf("  -o <output path>   Output path [default: $XDG_DOWNLOAD_DIR]\n");
    printf("  -h                 Show this help\n");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Missing arguments\n\n");
        helper(argv[0]);
        return 1;
    }

    const char* program_name = shift_args(&argc, &argv); // skip program name

    const char* torrent_file = NULL;
    const char* output_path  = NULL;

    while (argc > 0) {
        const char* arg = shift_args(&argc, &argv);
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
            LOG_ERROR("Failed to get XDG_DOWNLOAD_DIR, please provide an "
                      "output path\n");
            helper(program_name);
            return 1;
        }
    }

    LOG_INFO("Parsing torrent file: %s", torrent_file);

    torrent_t* torrent = torrent_create_from_file(torrent_file, output_path);
    if (torrent == NULL) {
        LOG_ERROR("Failed to create torrent");
        return 1;
    }

    tracker_req_t* req = tracker_request_create(torrent, 6881);
    if (req == NULL) {
        LOG_ERROR("Failed to create tracker request");
        torrent_free(torrent);
        return 1;
    }

    tracker_res_t* res = tracker_announce(req, torrent->announce);
    tracker_request_free(req);
    if (res == NULL) {
        LOG_ERROR("Failed to announce to tracker");
        torrent_free(torrent);
        return 1;
    }

    LOG_INFO("Tracker response:");
    LOG_INFO("  Failure reason: %s", res->failure_reason);
    LOG_INFO("  Warning message: %s", res->warning_message);
    LOG_INFO("  Interval: %d", res->interval);
    LOG_INFO("  Min interval: %d", res->min_interval);
    LOG_INFO("  Tracker ID: %s", res->tracker_id);
    LOG_INFO("  Complete: %d", res->complete);
    LOG_INFO("  Incomplete: %d", res->incomplete);

    // Unlink the peers list from the response
    list_t* peers = res->peers;
    res->peers    = NULL;

    // need to store more data (interval for example)

    tracker_response_free(res);

    LOG_INFO("  Peers:");
    for (const list_iterator_t* it = list_iterator_first(peers);
         it != NULL && will_log(LOG_LEVEL_INFO); it = list_iterator_next(it)) {
        peer_t* peer = list_iterator_get(it);
        char    ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &peer->addr.sin_addr, ip, INET_ADDRSTRLEN);
        LOG_INFO("    %s:%d", ip, ntohs(peer->addr.sin_port));
    }

    // NOTE: have a pool of peers instead of a single one

    peer_t* peer = NULL;

    for (const list_iterator_t* it = list_iterator_first(peers); it != NULL;
         it                        = list_iterator_next(it)) {
        peer = list_iterator_get(it);
        LOG_INFO("Connecting to peer %s:%d", inet_ntoa(peer->addr.sin_addr),
                 ntohs(peer->addr.sin_port));

        if (peer_connect(peer, torrent->info_hash) == 0) {
            LOG_INFO("Connected to peer %s:%d", inet_ntoa(peer->addr.sin_addr),
                     ntohs(peer->addr.sin_port));

            // TODO: check if it has all the pieces
            //       for now the download assumes the peer has all the pieces

            bool has_all_pieces = true;

            for (size_t i = 0; i < torrent->num_pieces; ++i) {
                if (!peer_has_piece(peer, i)) {
                    LOG_WARN("Peer %s:%d does not have piece %d",
                             inet_ntoa(peer->addr.sin_addr),
                             ntohs(peer->addr.sin_port), i);

                    has_all_pieces = false;
                    break;
                }
            }

            if (has_all_pieces) {
                break;
            }
        }
    }

    if (peer == NULL) {
        LOG_ERROR("Failed to connect to any peer");
        list_free(peers);
        torrent_free(torrent);
        return 1;
    }

    // NOTE: Instead of downloading pieces in ascending order, we should
    //       download the rarest pieces first
    //       (priority queue, piece object with a count of how many peers
    //       have it)?

    // NOTE: have a piece_t with:
    //          - index
    //          - peer_t* (peer working on the piece, NULL if no peer)
    //          (add more info)

    for (size_t i = 0; i < torrent->num_pieces; i++) {
        if (download_piece(peer, torrent, i) == -1) {
            LOG_ERROR("Failed to download piece");
            list_free(peers);
            torrent_free(torrent);
            return 1;
        }

        LOG_INFO("Piece %d/%d downloaded successfully", i + 1,
                 torrent->num_pieces);
    }

    // NOTE: maybe we should download by block instead of by piece

    list_free(peers);
    torrent_free(torrent);
    return 0;
}
