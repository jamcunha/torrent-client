#include "peer.h"
#include "log.h"

#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PROTOCOL "BitTorrent protocol"
#define PROTOCOL_LEN 19

#define HANDSHAKE_LEN (1 + PROTOCOL_LEN + 8 + SHA1_DIGEST_SIZE + PEER_ID_SIZE)

static int peer_connect(peer_t *peer) {
    if (peer == NULL) {
        LOG_WARN("[peer.c] Must provide a peer");
        return -1;
    }

    int sockfd = socket(peer->addr.sin_family, SOCK_STREAM, 0);
    if (sockfd < 0) {
        LOG_ERROR("[peer.c] Failed to create socket");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&peer->addr, sizeof(peer->addr)) != 0) {
        LOG_ERROR("[peer.c] Failed to connect to peer");
        return -1;
    }

    return sockfd;
}

static int peer_send_handshake(int sockfd, const uint8_t info_hash[SHA1_DIGEST_SIZE]) {
    if (sockfd < 0 || info_hash == NULL) {
        LOG_WARN("[peer.c] Must provide a valid socket and info hash");
        return -1;
    }

    uint8_t handshake[HANDSHAKE_LEN];

    handshake[0] = PROTOCOL_LEN;
    memcpy(handshake + 1, PROTOCOL, PROTOCOL_LEN);
    memset(handshake + 1 + PROTOCOL_LEN, 0, 8);
    memcpy(handshake + 1 + PROTOCOL_LEN + 8, info_hash, SHA1_DIGEST_SIZE);
    memcpy(handshake + 1 + PROTOCOL_LEN + 8 + SHA1_DIGEST_SIZE, get_peer_id(), PEER_ID_SIZE);

    if (send(sockfd, handshake, HANDSHAKE_LEN, 0) != HANDSHAKE_LEN) {
        LOG_ERROR("[peer.c] Failed to send handshake");
        return -1;
    }

    LOG_INFO("Sent handshake to peer");
    return 0;
}

static int peer_recv_handshake(int sockfd, const uint8_t info_hash[SHA1_DIGEST_SIZE]) {
    if (sockfd < 0 || info_hash == NULL) {
        LOG_WARN("[peer.c] Must provide a valid socket and info hash");
        return -1;
    }

    uint8_t handshake[HANDSHAKE_LEN];

    if (recv(sockfd, handshake, HANDSHAKE_LEN, 0) != HANDSHAKE_LEN) {
        LOG_ERROR("[peer.c] Failed to receive handshake");
        return -1;
    }

    if (handshake[0] != PROTOCOL_LEN) {
        LOG_ERROR("[peer.c] Invalid protocol length");
        return -1;
    }

    if (memcmp(handshake + 1, PROTOCOL, PROTOCOL_LEN) != 0) {
        LOG_ERROR("[peer.c] Invalid protocol");
        return -1;
    }

    if (memcmp(handshake + 1 + PROTOCOL_LEN + 8, info_hash, SHA1_DIGEST_SIZE) != 0) {
        LOG_ERROR("[peer.c] Invalid info hash");
        return -1;
    }

    if (will_log(LOG_LEVEL_INFO)) {
        uint8_t recv_peer_id[PEER_ID_SIZE + 1] = {0};
        memcpy(recv_peer_id, handshake + 1 + PROTOCOL_LEN + 8 + SHA1_DIGEST_SIZE, PEER_ID_SIZE);
        LOG_INFO("Received handshake from peer %s", recv_peer_id);
    }

    return 0;
}

int peer_connection_create(torrent_t *torrent, peer_t *peer) {
    if (torrent == NULL || peer == NULL) {
        LOG_WARN("[peer.c] Must provide a torrent and peer");
        return -1;
    }

    int sockfd = peer_connect(peer);
    if (sockfd < 0) {
        return -1;
    }

    if (peer_send_handshake(sockfd, torrent->info_hash) != 0) {
        close(sockfd);
        return -1;
    }

    if (peer_recv_handshake(sockfd, torrent->info_hash) != 0) {
        close(sockfd);
        return -1;
    }

    close(sockfd);
    return 0;
}
