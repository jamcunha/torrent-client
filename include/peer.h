#ifndef PEER_H
#define PEER_H

#include "peer_id.h"
#include "torrent.h"

#include <netinet/in.h>

#define KB 1024
#define BLOCK_SIZE (16 * KB)

typedef struct {
    char id[PEER_ID_SIZE];
    struct sockaddr_in addr;
} peer_t;

/**
 * @brief Connect to a peer
 *
 * @param torrent The torrent
 * @param peer The peer
 * @return int The socket file descriptor if successful, -1 otherwise
 */
int peer_connection_create(torrent_t *torrent, peer_t *peer);

/**
 * @brief Download a piece from a peer
 * @details For now, only single file torrents are supported
 * (getting the size of the last piece is not implemented for multi-file torrents)
 *
 * @param torrent The torrent
 * @param peer The peer
 * @param sockfd The socket file descriptor
 * @param index The piece index
 * @return int 0 if successful, -1 otherwise
 */
int download_piece(torrent_t *torrent, peer_t *peer, int sockfd, uint32_t index);

#endif // PEER_H
