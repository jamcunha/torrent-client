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

int peer_connection_create(torrent_t *torrent, peer_t *peer);

#endif // PEER_H
