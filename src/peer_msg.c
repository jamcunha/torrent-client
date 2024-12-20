#include "peer_msg.h"

#include "byte_str.h"
#include "log.h"

#include <assert.h>
#include <netinet/in.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

static const char* peer_msg_type_str(peer_msg_type_t type) {
    switch (type) {
    case PEER_MSG_CHOKE:
        return "CHOKE";
    case PEER_MSG_UNCHOKE:
        return "UNCHOKE";
    case PEER_MSG_INTERESTED:
        return "INTERESTED";
    case PEER_MSG_NOT_INTERESTED:
        return "NOT INTERESTED";
    case PEER_MSG_HAVE:
        return "HAVE";
    case PEER_MSG_BITFIELD:
        return "BITFIELD";
    case PEER_MSG_REQUEST:
        return "REQUEST";
    case PEER_MSG_PIECE:
        return "PIECE";
    case PEER_MSG_CANCEL:
        return "CANCEL";
    default:
        assert(0 && "Invalid peer message type");
    }
}

static uint32_t peer_msg_size(peer_msg_t* msg) {
    if (msg == NULL) {
        LOG_WARN("Must provide a message");
        return 0;
    }

    switch (msg->type) {
    case PEER_MSG_CHOKE:
    case PEER_MSG_UNCHOKE:
    case PEER_MSG_INTERESTED:
    case PEER_MSG_NOT_INTERESTED:
        return sizeof(peer_msg_type_t);
    case PEER_MSG_HAVE:
        return sizeof(peer_msg_type_t) + sizeof(uint32_t);
    case PEER_MSG_BITFIELD:
        return sizeof(peer_msg_type_t) + msg->payload.bitfield->len;
    case PEER_MSG_REQUEST:
        return sizeof(peer_msg_type_t) + sizeof(peer_request_msg_t);
    case PEER_MSG_PIECE:
        return sizeof(peer_msg_type_t) + 2 * sizeof(uint32_t)
               + msg->payload.piece.block->len;
    default:
        LOG_ERROR("Invalid message type");
        return -1;
    }
}

static int send_have_msg(int sockfd, peer_msg_t* msg) {
    if (msg == NULL) {
        LOG_WARN("Must provide a message");
        return -1;
    }

    if (msg->type != PEER_MSG_HAVE) {
        LOG_ERROR("Trying to send %s message as a HAVE message",
                  peer_msg_type_str(msg->type));
        return -1;
    }

    uint32_t idx = htonl(msg->payload.index);
    if (send(sockfd, &idx, sizeof(idx), 0) != sizeof(idx)) {
        LOG_ERROR("Failed to send HAVE message");
        return -1;
    }

    return 0;
}

static int send_bitfield_msg(int sockfd, peer_msg_t* msg) {
    if (msg == NULL) {
        LOG_WARN("Must provide a message");
        return -1;
    }

    if (msg->type != PEER_MSG_BITFIELD) {
        LOG_ERROR("Trying to send %s message as a BITFIELD message",
                  peer_msg_type_str(msg->type));
        return -1;
    }

    ssize_t sent = send(sockfd, msg->payload.bitfield->data,
                        msg->payload.bitfield->len, 0);
    if (sent < 0 || (size_t)sent != msg->payload.bitfield->len) {
        LOG_ERROR("Failed to send BITFIELD message");
        return -1;
    }

    return 0;
}

// CANCEL messages also use this function
static int send_request_msg(int sockfd, peer_msg_t* msg) {
    if (msg == NULL) {
        LOG_WARN("Must provide a message");
        return -1;
    }

    if (msg->type != PEER_MSG_REQUEST && msg->type != PEER_MSG_CANCEL) {
        LOG_ERROR("Trying to send %s message as a REQUEST/CANCEL "
                  "message",
                  peer_msg_type_str(msg->type));
        return -1;
    }

    uint32_t idx    = htonl(msg->payload.request.index);
    uint32_t begin  = htonl(msg->payload.request.begin);
    uint32_t length = htonl(msg->payload.request.length);

    if (send(sockfd, &idx, sizeof(idx), 0) != sizeof(idx)) {
        LOG_ERROR("Failed to send %s message", peer_msg_type_str(msg->type));
        return -1;
    }

    if (send(sockfd, &begin, sizeof(begin), 0) != sizeof(begin)) {
        LOG_ERROR("Failed to send %s message", peer_msg_type_str(msg->type));
        return -1;
    }

    if (send(sockfd, &length, sizeof(length), 0) != sizeof(length)) {
        LOG_ERROR("Failed to send %s message", peer_msg_type_str(msg->type));
        return -1;
    }

    return 0;
}

static int recv_piece_msg(int sockfd, peer_msg_t* msg, size_t len) {
    if (msg == NULL) {
        LOG_WARN("Must provide a message");
        return -1;
    }

    uint32_t idx;
    if (recv(sockfd, &idx, sizeof(uint32_t), 0) != sizeof(uint32_t)) {
        LOG_ERROR("Failed to receive piece message");
        return -1;
    }
    msg->payload.piece.index = ntohl(idx);

    uint32_t begin;
    if (recv(sockfd, &begin, sizeof(uint32_t), 0) != sizeof(uint32_t)) {
        LOG_ERROR("Failed to receive piece message");
        return -1;
    }
    msg->payload.piece.begin = ntohl(begin);

    uint32_t block_len = len - 2 * sizeof(uint32_t);

    uint8_t buf[block_len];
    for (uint32_t total_recv = 0; total_recv < block_len;) {
        ssize_t recv_len
            = recv(sockfd, buf + total_recv, block_len - total_recv, 0);
        if (recv_len <= 0) {
            LOG_ERROR("Failed to receive piece message");
            return -1;
        }
        total_recv += recv_len;
    }

    msg->payload.piece.block = byte_str_create(buf, len);
    return 0;
}

int peer_send_msg(int sockfd, peer_msg_t* msg) {
    if (sockfd < 0 || msg == NULL) {
        LOG_WARN("Must provide a valid socket and message");
        return -1;
    }

    uint32_t len = htonl(peer_msg_size(msg));

    if (send(sockfd, &len, sizeof(len), 0) != sizeof(len)) {
        LOG_ERROR("Failed to send message length");
        return -1;
    }

    if (send(sockfd, &msg->type, sizeof(msg->type), 0) != sizeof(msg->type)) {
        LOG_ERROR("Failed to send message type");
        return -1;
    }

    switch (msg->type) {
    case PEER_MSG_CHOKE:
    case PEER_MSG_UNCHOKE:
    case PEER_MSG_INTERESTED:
    case PEER_MSG_NOT_INTERESTED:
        assert(ntohl(len) == sizeof(peer_msg_type_t)
               && "Invalid message length");
        return 0;
    case PEER_MSG_HAVE:
        return send_have_msg(sockfd, msg);
    case PEER_MSG_BITFIELD:
        assert(msg->payload.bitfield != NULL && "Invalid message payload");
        return send_bitfield_msg(sockfd, msg);
    case PEER_MSG_REQUEST:
        return send_request_msg(sockfd, msg);
    case PEER_MSG_PIECE:
        // NOTE: For now, we don't send PIECE messages
        assert(0 && "PIECE message not implemented");
    case PEER_MSG_CANCEL:
        assert(0 && "CANCEL message not implemented");
    default:
        assert(0 && "Invalid message type");
    }
}

peer_msg_t* peer_recv_msg(int sockfd) {
    if (sockfd < 0) {
        LOG_WARN("Must provide a valid socket");
        return NULL;
    }

    uint32_t len;
    if (recv(sockfd, &len, sizeof(len), 0) != sizeof(len)) {
        LOG_ERROR("Failed to receive message length");
        return NULL;
    }
    len = ntohl(len);

    peer_msg_type_t type;
    if (recv(sockfd, &type, sizeof(type), 0) != sizeof(type)) {
        LOG_ERROR("Failed to receive message type");
        return NULL;
    }

    // TODO: validate len

    peer_msg_t* msg = malloc(sizeof(peer_msg_t));
    if (msg == NULL) {
        LOG_ERROR("Failed to allocate message");
        return NULL;
    }

    msg->type = type;

    uint32_t left = len - sizeof(type);
    switch (type) {
    case PEER_MSG_CHOKE:
    case PEER_MSG_UNCHOKE:
    case PEER_MSG_INTERESTED:
    case PEER_MSG_NOT_INTERESTED:
        if (left != 0) {
            LOG_ERROR("Invalid message length");
            peer_msg_free(msg);
            return NULL;
        }
        return msg;
    case PEER_MSG_HAVE:
        // NOTE: For now, we don't receive HAVE messages
        LOG_WARN("HAVE message not implemented");
        peer_msg_free(msg);
        return NULL;
    case PEER_MSG_BITFIELD: {
        uint8_t buf[left];
        if (recv(sockfd, buf, left, 0) != left) {
            LOG_ERROR("Failed to receive message");
            peer_msg_free(msg);
            return NULL;
        }

        msg->payload.bitfield = byte_str_create(buf, left);
        if (msg->payload.bitfield == NULL) {
            peer_msg_free(msg);
            return NULL;
        }
        return msg;
    }
    case PEER_MSG_REQUEST:
        // NOTE: For now, we don't receive REQUEST messages
        LOG_WARN("REQUEST message not implemented");
        peer_msg_free(msg);
        return NULL;
    case PEER_MSG_PIECE:
        if (recv_piece_msg(sockfd, msg, left) != 0) {
            peer_msg_free(msg);
            return NULL;
        }
        return msg;
    case PEER_MSG_CANCEL:
        // NOTE: For now, we don't receive CANCEL messages
        LOG_WARN("CANCEL message not implemented");
        peer_msg_free(msg);
        return NULL;
    default:
        LOG_ERROR("Invalid message type");
        peer_msg_free(msg);
        return NULL;
    }
}

void peer_msg_free(peer_msg_t* msg) {
    if (msg == NULL) {
        LOG_WARN("Trying to free NULL message");
        return;
    }

    switch (msg->type) {
    case PEER_MSG_CHOKE:
    case PEER_MSG_UNCHOKE:
    case PEER_MSG_INTERESTED:
    case PEER_MSG_NOT_INTERESTED:
    case PEER_MSG_HAVE:
    case PEER_MSG_REQUEST:
    case PEER_MSG_CANCEL:
        // Nothing to free
        break;
    case PEER_MSG_BITFIELD:
        free(msg->payload.bitfield);
        break;
    case PEER_MSG_PIECE:
        free(msg->payload.piece.block);
        break;
    default:
        assert(0 && "Invalid message type");
    }

    free(msg);
}
