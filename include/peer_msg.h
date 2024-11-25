#ifndef PEER_MSG_H
#define PEER_MSG_H

#include "byte_str.h"

#include <stdint.h>

typedef enum __attribute__((packed)) {
    PEER_MSG_CHOKE,
    PEER_MSG_UNCHOKE,
    PEER_MSG_INTERESTED,
    PEER_MSG_NOT_INTERESTED,
    PEER_MSG_HAVE,
    PEER_MSG_BITFIELD,
    PEER_MSG_REQUEST,
    PEER_MSG_PIECE,
    PEER_MSG_CANCEL,
} peer_msg_type_t;

typedef struct {
    uint32_t index;
    uint32_t begin;
    uint32_t length;
} peer_request_msg_t;

typedef struct {
    uint32_t index;
    uint32_t begin;
    byte_str_t *block;
} peer_piece_msg_t;

typedef struct {
    peer_msg_type_t type;
    union {
        uint32_t index;             // HAVE
        byte_str_t *bitfield;       // BITFIELD
        peer_request_msg_t request; // REQUEST, CANCEL
        peer_piece_msg_t piece;     // PIECE
    } payload;
} peer_msg_t;

/**
 * @brief Send a message to a peer
 *
 * @param sockfd The socket file descriptor
 * @param msg The message to send
 * @return int 0 if successful, -1 otherwise
 */
int peer_send_msg(int sockfd, peer_msg_t *msg);

/**
 * @brief Receive a message from a peer
 *
 * @param sockfd The socket file descriptor
 * @return peer_msg_t* The message received, NULL otherwise
 */
peer_msg_t *peer_recv_msg(int sockfd);

#endif // !PEER_MSG_H
