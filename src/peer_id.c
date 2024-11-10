#include "peer_id.h"
#include "log.h"
#include "unistd.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static const char *client_id = "TC";
static const char *version = "0001";

char *generate_peer_id(void) {
    char *peer_id = malloc(21);
    if (peer_id == NULL) {
        return NULL;
    }

    int offset = snprintf(peer_id, 21, "-%s%s-", client_id, version);

    // To get some thread safety (not secure but good enough for this)
    int pid = getpid();
    time_t t = time(NULL);
    for (int i = offset; i < 20; ++i) {
        peer_id[i] = (char)((pid + (t >> i)) & 0xFF);
    }

    // Null terminate the string
    peer_id[20] = '\0';

    LOG_INFO("Generated peer id: %s", peer_id);

    return peer_id;
}
