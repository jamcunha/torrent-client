#include "peer_id.h"

#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static const char* client_id = "TC";
static const char* version   = "0001";

static uint8_t peer_id[PEER_ID_SIZE];

void generate_peer_id(void) {
    int offset
        = snprintf((char*)peer_id, PEER_ID_SIZE, "-%s%s-", client_id, version);

    // To get some thread safety (not secure but good enough for this)
    int    pid = getpid();
    time_t t   = time(NULL);
    for (int i = offset; i < PEER_ID_SIZE; ++i) {
        peer_id[i] = (uint8_t)((pid + (t >> i)) & 0xFF);
    }

    LOG_DEBUG("Generated peer id: %s", peer_id);
}

const uint8_t* get_peer_id(void) {
    return peer_id;
}
