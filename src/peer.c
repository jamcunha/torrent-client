#include "peer.h"

#include "byte_str.h"
#include "file.h"
#include "list.h"
#include "log.h"
#include "peer_id.h"
#include "peer_msg.h"
#include "sha1.h"

#include <assert.h>
#include <limits.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PROTOCOL     "BitTorrent protocol"
#define PROTOCOL_LEN 19

#define HANDSHAKE_LEN (1 + PROTOCOL_LEN + 8 + SHA1_DIGEST_SIZE + PEER_ID_SIZE)

static int peer_connect_socket(peer_t* peer) {
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

static int peer_recv_handshake(peer_t*       peer,
                               const uint8_t info_hash[SHA1_DIGEST_SIZE]) {
    if (peer == NULL || info_hash == NULL) {
        LOG_WARN("[peer.c] Must provide a valid peer and info hash");
        return -1;
    }

    uint8_t handshake[HANDSHAKE_LEN];

    if (recv(peer->sockfd, handshake, HANDSHAKE_LEN, 0) != HANDSHAKE_LEN) {
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

    // TODO: Find a way to check if the peer id given in creation is different
    //       from the received one
    memcpy(peer->id, handshake + 1 + PROTOCOL_LEN + 8 + SHA1_DIGEST_SIZE,
           PEER_ID_SIZE);

    LOG_INFO("Received handshake from peer: %s", peer->id);
    return 0;
}

peer_t* peer_create(uint32_t ip, uint16_t port,
                    const uint8_t peer_id[PEER_ID_SIZE]) {
    peer_t* peer = malloc(sizeof(peer_t));
    if (peer == NULL) {
        LOG_ERROR("[peer.c] Failed to allocate memory for peer");
        return NULL;
    }

    memset(peer, 0, sizeof(peer_t));

    peer->sockfd               = -1;
    peer->addr.sin_family      = AF_INET;
    peer->addr.sin_port        = port;
    peer->addr.sin_addr.s_addr = ip;
    peer->choked               = true;
    peer->bitfield             = NULL;

    // NOTE: if the id received from the handshake is the same one
    //       from the tracker, just remove this from the peer creation
    if (peer_id != NULL) {
        memcpy(peer->id, peer_id, PEER_ID_SIZE);
    }

    return peer;
}

int peer_connect(peer_t* peer, const uint8_t info_hash[SHA1_DIGEST_SIZE]) {
    if (peer == NULL || info_hash == NULL) {
        LOG_WARN("[peer.c] Must provide a peer and an info hash");
        return -1;
    }

    if (peer->sockfd != -1) {
        LOG_WARN("[peer.c] Peer is already connected");
        return -1;
    }

    peer->sockfd = peer_connect_socket(peer);
    if (peer->sockfd < 0) {
        return -1;
    }

    if (peer_send_handshake(peer->sockfd, info_hash) != 0) {
        return -1;
    }

    if (peer_recv_handshake(peer, info_hash) != 0) {
        return -1;
    }

    peer_msg_t* bitfield_msg = peer_recv_msg(peer->sockfd);
    if (bitfield_msg == NULL) {
        return -1;
    }

    if (bitfield_msg->type != PEER_MSG_BITFIELD) {
        LOG_ERROR("[peer.c] Expected bitfield message");
        peer_msg_free(bitfield_msg);
        return -1;
    }

    // copy bitfield
    peer->bitfield = byte_str_create(bitfield_msg->payload.bitfield->data,
                                     bitfield_msg->payload.bitfield->len);

    peer_msg_free(bitfield_msg);
    return 0;
}

bool peer_has_piece(peer_t* peer, uint32_t index) {
    if (peer == NULL) {
        LOG_WARN("[peer.c] Must provide a peer");
        return false;
    }

    if (peer->bitfield == NULL) {
        LOG_WARN("[peer.c] Peer has no bitfield");
        return false;
    }

    uint32_t byte = index / CHAR_BIT;
    uint8_t  bit  = index % CHAR_BIT;

    get_byte_result_t byte_result = byte_str_get_byte(peer->bitfield, byte);
    if (!byte_result.success) {
        LOG_ERROR("[peer.c] Failed to get byte from bitfield");
        return false;
    }

    return (byte_result.byte & (1 << (CHAR_BIT - bit - 1))) != 0;
}

// TODO: This function should be responsible for downloading a single block from
// a single peer
//       Complexity should be moved to torrent.c
int download_piece(peer_t* peer, torrent_t* torrent, uint32_t index) {
    if (torrent == NULL || peer == NULL) {
        LOG_WARN("[peer.c] Must provide a torrent and a peer");
        return -1;
    }

    if (peer->sockfd == -1) {
        LOG_WARN("[peer.c] Peer is not connected");
        return -1;
    }

    if (list_size(torrent->files) != 1) {
        LOG_ERROR("[peer.c] Only single file torrents are supported");
        return -1;
    }

    peer_msg_t interested_msg = {.type = PEER_MSG_INTERESTED};

    if (peer_send_msg(peer->sockfd, &interested_msg) != 0) {
        return -1;
    }

    while (peer->choked) {
        peer_msg_t* unchoke_msg = peer_recv_msg(peer->sockfd);
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
            .payload.request =
                (peer_request_msg_t){
                    .index = index,
                    .begin = i,
                    .length = block_size,
                },
        };

        if (peer_send_msg(peer->sockfd, &request_msg) != 0) {
            return -1;
        }

        peer_msg_t* piece_msg = peer_recv_msg(peer->sockfd);
        if (piece_msg == NULL) {
            return -1;
        }

        // test
        if (piece_msg->type == PEER_MSG_UNCHOKE) {
            peer_msg_free(piece_msg);
            continue;
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

void peer_free(peer_t* peer) {
    if (peer == NULL) {
        LOG_WARN("[peer.c] Trying to free NULL peer");
        return;
    }

    close(peer->sockfd);
    free(peer->bitfield);
    free(peer);
}
