#include "peer_id.h"
#include "log.h"
#include "unistd.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static const char *client_id = "TC";
static const char *version = "0001";

static char peer_id[PEER_ID_SIZE];

void generate_peer_id(void) {
    int offset = snprintf(peer_id, PEER_ID_SIZE, "-%s%s-", client_id, version);

    // To get some thread safety (not secure but good enough for this)
    int pid = getpid();
    time_t t = time(NULL);
    for (int i = offset; i < PEER_ID_SIZE; ++i) {
        peer_id[i] = (char)((pid + (t >> i)) & 0xFF);
    }

    LOG_DEBUG("[peer_id.c] Generated peer id: %s", peer_id);
}

const char *get_peer_id(void) {
    return peer_id;
}
