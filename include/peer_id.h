#ifndef PEER_ID_H
#define PEER_ID_H

#include <stdint.h>

#define PEER_ID_SIZE 20

/**
 * @brief Generate a peer ID
 *
 * The peer ID is a 20-byte string that is used to identify the client
 * to the tracker and other peers.
 *
 * The generated peer ID is stored in the GLOBAL_PEER_ID variable.
 */
void generate_peer_id(void);

/**
 * @brief Get the peer ID
 *
 * @return const char* The peer ID
 */
const uint8_t* get_peer_id(void);

#endif // !PEER_ID_H
