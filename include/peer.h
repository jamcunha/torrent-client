#ifndef PEER_H
#define PEER_H

#include "byte_str.h"
#include "peer_id.h"
#include "torrent.h"

#include <netinet/in.h>
#include <stdbool.h>

#define BLOCK_SIZE (16 * 1024)

typedef struct {
    int                sockfd;
    uint8_t            id[PEER_ID_SIZE];
    struct sockaddr_in addr;
    byte_str_t*        bitfield;

    // NOTE: Maybe add a uint8_t state to keep track of more states
    bool choked;
    bool interested;

    // NOTE: Future reference: check Deluge's lazy bitfield implementation
} peer_t;

/**
 * @brief Create a new peer object
 *
 * @param ip The IP address, in network byte order
 * @param port The port, in network byte order
 * @param peer_id The peer ID, optional
 * @return peer_t* The peer
 */
peer_t* peer_create(uint32_t ip, uint16_t port,
                    const uint8_t peer_id[PEER_ID_SIZE]);

/**
 * @brief Connect to a peer
 *
 * @param peer The peer
 * @param info_hash The info hash of the torrent
 * @return true if the peer is connected, false otherwise
 */
int peer_connect(peer_t* peer, const uint8_t info_hash[SHA1_DIGEST_SIZE]);

/**
 * @brief Check if a peer has a piece
 *
 * Bitfield structure is as follows:
 * ---------------------------------   ---------------------------------
 * | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |   | 8 | 9 | 10| 11| 12| 13| 14| 15| ...
 * ---------------------------------   ---------------------------------
 * ^byte 0                             ^byte 1
 *
 * @param peer The peer
 * @param index The piece index
 * @return true if the peer has the piece, false otherwise
 */
bool peer_has_piece(peer_t* peer, uint32_t index);

/**
 * @brief Download a piece from a peer
 * @details For now, only single file torrents are supported
 * (still need to find how to send the correct data to the file)
 *
 * @param peer The peer
 * @param torrent The torrent
 * @param index The piece index
 * @return int 0 if successful, -1 otherwise
 */
int download_piece(peer_t* peer, torrent_t* torrent, uint32_t index);

/**
 * @brief Close the connection and free the peer
 *
 * @param peer The peer
 */
void peer_free(peer_t* peer);

#endif // PEER_H
