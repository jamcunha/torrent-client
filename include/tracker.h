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

typedef enum {
    TRACKER_EVENT_EMPTY,
    TRACKER_EVENT_COMPLETED,
    TRACKER_EVENT_STARTED,
    TRACKER_EVENT_STOPPED
} tracker_event_t;

typedef struct {
    uint8_t            info_hash[SHA1_DIGEST_SIZE];
    char               peer_id[PEER_ID_SIZE];
    uint16_t           port;
    uint64_t           uploaded;
    uint64_t           downloaded;
    uint64_t           left;
    bool               compact;
    bool               no_peer_id;
    tracker_event_t    event;
    struct sockaddr_in ip;
    uint32_t           numwant;
    char*              key;
    char*              tracker_id;
} tracker_req_t;

typedef struct {
    char*    failure_reason;
    char*    warning_message;
    uint32_t interval;
    uint32_t min_interval;
    char*    tracker_id;
    uint32_t complete;
    uint32_t incomplete;
    list_t*  peers;
} tracker_res_t;

/**
 * @brief Announce to a tracker
 *
 * @param req The tracker request
 * @param announce_url The announce URL
 * @return The tracker response
 */
tracker_res_t* tracker_announce(tracker_req_t* req, const char* announce_url);

/**
 * @brief Create a tracker request
 *
 * @param torrent The torrent
 * @param port The port
 * @return The tracker request
 */
tracker_req_t* tracker_request_create(torrent_t* torrent, uint16_t port);

/**
 * @brief Free a tracker request
 *
 * @param req The tracker request
 */
void tracker_request_free(tracker_req_t* req);

/**
 * @brief Parse a tracker response
 *
 * @param bencode_str The bencoded string
 * @return The tracker response
 */
tracker_res_t* parse_tracker_response(char* bencode_str);

/**
 * @brief Free a tracker response
 *
 * @param res The tracker response
 */
void tracker_response_free(tracker_res_t* res);

#endif // !TRACKER_H
