#ifndef TORRENT_H
#define TORRENT_H

#include "bencode.h"
#include "list.h"
#include "sha1.h"

#include <stdint.h>

#define TORRENT_DEFAULT_MAX_PEERS 50

typedef struct {
    char* announce;

    // Optional
    // list_t *announce_list;
    uint32_t creation_date;
    char*    comment;
    char*    created_by;
    // char *encoding;

    // Info
    uint8_t   info_hash[SHA1_DIGEST_SIZE];
    list_t*   files;
    uint8_t** pieces;

    // NOTE: maybe add a byte_str to check each piece state
    //       instead of only a counter
    size_t num_pieces;

    uint64_t piece_length;
    // bool private;

    uint32_t max_peers;
    size_t   pieces_left;

    // Should this be here?
    uint64_t total_down;
} torrent_t;

/**
 * @brief Create a new torrent object
 *
 * @param node The bencode node
 * @param output_path The output path
 * @return torrent_t* The torrent
 */
torrent_t* torrent_create(bencode_node_t* node, const char* output_path);

torrent_t* torrent_create_from_file(const char* filename,
                                    const char* output_path);

/**
 * @brief Free the torrent object
 *
 * @param torrent The torrent
 */
void torrent_free(torrent_t* torrent);

#endif // !TORRENT_H
