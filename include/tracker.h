#ifndef TRACKER_H
#define TRACKER_H

#include "list.h"
#include "peer_id.h"
#include "sha1.h"
#include "torrent.h"

#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/socket.h>

#define KB 1024
#define BLOCK_SIZE (16 * KB)

typedef enum {
    TRACKER_EVENT_EMPTY,
    TRACKER_EVENT_COMPLETED,
    TRACKER_EVENT_STARTED,
    TRACKER_EVENT_STOPPED
} tracker_event_t;

typedef struct {
    uint8_t info_hash[SHA1_DIGEST_SIZE];
    char peer_id[PEER_ID_SIZE];
    uint16_t port;
    uint64_t uploaded;
    uint64_t downloaded;
    uint64_t left;
    bool compact;
    bool no_peer_id;
    tracker_event_t event;
    struct sockaddr_in ip;
    uint32_t numwant;
    char *key;
    char *tracker_id;
} tracker_req_t;

typedef struct {
    char *failure_reason;
    char *warning_message;
    uint32_t interval;
    uint32_t min_interval;
    char *tracker_id;
    uint32_t complete;
    uint32_t incomplete;
    list_t *peers;
} tracker_res_t;

typedef struct {
    char id[PEER_ID_SIZE];
    struct sockaddr_in addr;
} peer_t;

tracker_res_t *tracker_announce(torrent_t *torrent);

void tracker_res_free(tracker_res_t *res);

#endif // !TRACKER_H
