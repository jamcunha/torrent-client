#include "peer.h"

#include "file.h"
#include "list.h"
#include "log.h"
#include "peer_msg.h"

#include <assert.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PROTOCOL     "BitTorrent protocol"
#define PROTOCOL_LEN 19

#define HANDSHAKE_LEN (1 + PROTOCOL_LEN + 8 + SHA1_DIGEST_SIZE + PEER_ID_SIZE)

static int peer_connect(peer_t* peer) {
    if (peer == NULL) {
        LOG_WARN("[peer.c] Must provide a peer");
        return -1;
    }

    int sockfd = socket(peer->addr.sin_family, SOCK_STREAM, 0);
    if (sockfd < 0) {
        LOG_ERROR("[peer.c] Failed to create socket");
        return -1;
    }

    if (connect(sockfd, (struct sockaddr*)&peer->addr, sizeof(peer->addr))
        != 0) {
        LOG_ERROR("[peer.c] Failed to connect to peer");
        return -1;
    }

    return sockfd;
}

static int peer_send_handshake(int           sockfd,
                               const uint8_t info_hash[SHA1_DIGEST_SIZE]) {
    if (sockfd < 0 || info_hash == NULL) {
        LOG_WARN("[peer.c] Must provide a valid socket and info hash");
        return -1;
    }

    uint8_t handshake[HANDSHAKE_LEN];

    handshake[0] = PROTOCOL_LEN;
    memcpy(handshake + 1, PROTOCOL, PROTOCOL_LEN);
    memset(handshake + 1 + PROTOCOL_LEN, 0, 8);
    memcpy(handshake + 1 + PROTOCOL_LEN + 8, info_hash, SHA1_DIGEST_SIZE);
    memcpy(handshake + 1 + PROTOCOL_LEN + 8 + SHA1_DIGEST_SIZE, get_peer_id(),
           PEER_ID_SIZE);

    if (send(sockfd, handshake, HANDSHAKE_LEN, 0) != HANDSHAKE_LEN) {
        LOG_ERROR("[peer.c] Failed to send handshake");
        return -1;
    }

    LOG_INFO("Sent handshake to peer");
    return 0;
}

static int peer_recv_handshake(int           sockfd,
                               const uint8_t info_hash[SHA1_DIGEST_SIZE]) {
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

    if (memcmp(handshake + 1 + PROTOCOL_LEN + 8, info_hash, SHA1_DIGEST_SIZE)
        != 0) {
        LOG_ERROR("[peer.c] Invalid info hash");
        return -1;
    }

    if (will_log(LOG_LEVEL_INFO)) {
        uint8_t recv_peer_id[PEER_ID_SIZE + 1] = {0};
        memcpy(recv_peer_id,
               handshake + 1 + PROTOCOL_LEN + 8 + SHA1_DIGEST_SIZE,
               PEER_ID_SIZE);
        LOG_INFO("Received handshake from peer: %s", recv_peer_id);
    }

    return 0;
}

int peer_connection_create(torrent_t* torrent, peer_t* peer) {
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

    peer_msg_t* bitfield_msg = peer_recv_msg(sockfd);
    if (bitfield_msg == NULL) {
        return -1;
    }

    if (bitfield_msg->type != PEER_MSG_BITFIELD) {
        LOG_ERROR("[peer.c] Expected bitfield message");
        peer_msg_free(bitfield_msg);
        return -1;
    }

    // TODO: check needed pieces
    //       for now assume peer has all pieces
    peer_msg_free(bitfield_msg);

    // TODO: may need to return info from the bitfield message
    return sockfd;
}

int download_piece(torrent_t* torrent, peer_t* peer, int sockfd,
                   uint32_t index) {
    if (torrent == NULL || peer == NULL || sockfd < 0) {
        LOG_WARN("[peer.c] Must provide a torrent, peer, and socket");
        return -1;
    }

    LOG_WARN("[peer.c] ONLY SINGLE FILE TORRENTS SUPPORTED");
    if (list_size(torrent->files) != 1) {
        LOG_ERROR("[peer.c] Only single file torrents are supported");
        return -1;
    }

    peer_msg_t interested_msg = {.type = PEER_MSG_INTERESTED};

    if (peer_send_msg(sockfd, &interested_msg) != 0) {
        return -1;
    }

    while (peer->choked) {
        peer_msg_t* unchoke_msg = peer_recv_msg(sockfd);
        if (unchoke_msg == NULL) {
            return -1;
        }

        switch (unchoke_msg->type) {
        case PEER_MSG_UNCHOKE:
            peer->choked = false;
            break;
        case PEER_MSG_CHOKE:
            assert(0 && "Peer choked, TODO: handle this");
            break;
        case PEER_MSG_HAVE:
            // TODO: update needed pieces
            //      for now assume peer has all pieces
            break;
        case PEER_MSG_BITFIELD:
            assert(0 && "Should not receive bitfield message after handshake");
            break;
        default:
            // No need to handle other messages
            break;
        }
        peer_msg_free(unchoke_msg);
    }

    uint64_t piece_length = torrent->piece_length;

    // If this is the last piece, the length may be less than the piece length
    if (index == torrent->num_pieces - 1) {
        piece_length = torrent->total_down % torrent->piece_length;
    }

    uint8_t piece[piece_length];
    for (size_t i = 0; i < piece_length; i += BLOCK_SIZE) {
        uint32_t block_size = BLOCK_SIZE;
        if (i + BLOCK_SIZE > piece_length) {
            block_size = piece_length - i;
        }

        peer_msg_t request_msg = {
            .type = PEER_MSG_REQUEST,
            .payload.request = (peer_request_msg_t) {
                .index = index,
                .begin = i,
                .length = block_size,
            },
        };

        if (peer_send_msg(sockfd, &request_msg) != 0) {
            return -1;
        }

        peer_msg_t* piece_msg = peer_recv_msg(sockfd);
        if (piece_msg == NULL) {
            return -1;
        }

        switch (piece_msg->type) {
        case PEER_MSG_CHOKE:
            peer->choked = true;
            LOG_WARN("[peer.c] Peer choked while downloading piece");
            peer_msg_free(piece_msg);
            return -1;
        case PEER_MSG_UNCHOKE:
            assert(0 && "Should not receive unchoke message while unchoked");
            peer_msg_free(piece_msg);
            return -1;
        case PEER_MSG_HAVE:
            // TODO: update needed pieces
            //     for now assume peer has all pieces
            break;
        case PEER_MSG_BITFIELD:
            assert(0 && "Should not receive bitfield message after handshake");
            peer_msg_free(piece_msg);
            return -1;
        case PEER_MSG_PIECE:
            if (piece_msg->payload.piece.index != index
                || piece_msg->payload.piece.begin != i) {
                LOG_ERROR("[peer.c] Invalid piece message");
                peer_msg_free(piece_msg);
                return -1;
            }

            memcpy(piece + i, piece_msg->payload.piece.block->data, block_size);
        default:
            // No need to handle other messages
            break;
        }
        peer_msg_free(piece_msg);
    }

    uint8_t recv_hash[SHA1_DIGEST_SIZE];
    sha1(piece, piece_length, recv_hash);

    if (memcmp(recv_hash, torrent->pieces[index], SHA1_DIGEST_SIZE) != 0) {
        LOG_ERROR("[peer.c] Invalid piece hash");
        return -1;
    }

    // TODO: Find a way to add the piece to correct files in multi file torrents

    file_t* file = *(file_t**)list_at(torrent->files, 0);
    if (write_data_to_file(file, index * torrent->piece_length, piece,
                           piece_length)
        != 0) {
        return -1;
    }

    return 0;
}
