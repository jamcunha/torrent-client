#include "tracker.h"
#include "bencode.h"
#include "dict.h"
#include "list.h"
#include "log.h"
#include "url.h"

#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static int tracker_connect(url_t *url) {
    if (url == NULL) {
        LOG_WARN("[tracker.c] Must provide a URL");
        return -1;
    }

    struct addrinfo hints;
    struct addrinfo *head;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = url->protocol == URL_PROTOCOL_UDP ? SOCK_DGRAM : SOCK_STREAM;
    hints.ai_protocol = url->protocol == URL_PROTOCOL_UDP ? IPPROTO_UDP : IPPROTO_TCP;

    char port[6];
    snprintf(port, sizeof(port), "%hu", url->port);

    int s = getaddrinfo(url->host, port, &hints, &head);
    if (s != 0) {
        LOG_ERROR("[tracker.c] Failed to get address info for tracker: %s", gai_strerror(s));
        return -1;
    }

    int sockfd;
    struct addrinfo *tracker_addr;
    for (tracker_addr = head; tracker_addr != NULL; tracker_addr = tracker_addr->ai_next) {
        if ((sockfd = socket(tracker_addr->ai_family, tracker_addr->ai_socktype, tracker_addr->ai_protocol)) == -1) {
            LOG_ERROR("[tracker.c] Failed to create socket for tracker");
            continue;
        }

        if (connect(sockfd, tracker_addr->ai_addr, tracker_addr->ai_addrlen) == -1) {
            LOG_ERROR("[tracker.c] Failed to connect to tracker");
            close(sockfd);
            continue;
        }

        break;
    }

    if (tracker_addr == NULL) {
        LOG_ERROR("[tracker.c] Failed to connect to any tracker");
        freeaddrinfo(head);
        return -1;
    }
    freeaddrinfo(head);

    LOG_INFO("Successfully connected to tracker: %s", url->host);
    return sockfd;
}

inline static bool is_valid_url_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
           c == '.' || c == '-' || c == '_' || c == '~';
}

static int url_encode_str(char *buffer, size_t size, const char *str, size_t str_len) {
    int written = 0;
    for (size_t i = 0; i < str_len; ++i) {
        if (is_valid_url_char(str[i])) {
            written += snprintf(buffer + written, size - written, "%c", str[i]);
        } else {
            written += snprintf(buffer + written, size - written, "%%%02X", (uint8_t)str[i]);
        }
    }

    return written;
}

static char *build_http_request(url_t *url, tracker_req_t *req) {
    if (url == NULL || req == NULL) {
        LOG_WARN("[tracker.c] Must provide a URL and a tracker request");
        return NULL;
    }

    int written = 0;
    char buffer[4096];

    written += snprintf(buffer, sizeof(buffer), "GET %s", url->path);

    written += snprintf(buffer + written, sizeof(buffer) - written, "?info_hash=");
    written += url_encode_str(buffer + written, sizeof(buffer) - written, (char *)req->info_hash, SHA1_DIGEST_SIZE);

    written += snprintf(buffer + written, sizeof(buffer) - written, "&peer_id=");
    written += url_encode_str(buffer + written, sizeof(buffer) - written, req->peer_id, PEER_ID_SIZE);

    written += snprintf(buffer + written, sizeof(buffer) - written, "&port=%hu", req->port);

    written += snprintf(buffer + written, sizeof(buffer) - written, "&uploaded=%lu", req->uploaded);

    written += snprintf(buffer + written, sizeof(buffer) - written, "&downloaded=%lu", req->downloaded);

    written += snprintf(buffer + written, sizeof(buffer) - written, "&left=%lu", req->left);

    if (req->compact) {
        written += snprintf(buffer + written, sizeof(buffer) - written, "&compact=1");
    }

    if (req->no_peer_id) {
        written += snprintf(buffer + written, sizeof(buffer) - written, "&no_peer_id=1");
    }

    switch (req->event) {
        case TRACKER_EVENT_COMPLETED:
            written += snprintf(buffer + written, sizeof(buffer) - written, "&event=completed");
            break;
        case TRACKER_EVENT_STARTED:
            written += snprintf(buffer + written, sizeof(buffer) - written, "&event=started");
            break;
        case TRACKER_EVENT_STOPPED:
            written += snprintf(buffer + written, sizeof(buffer) - written, "&event=stopped");
            break;
        default:
            break;
    }

    if (req->numwant > 0) {
        written += snprintf(buffer + written, sizeof(buffer) - written, "&numwant=%u", req->numwant);
    }

    if (req->key != NULL) {
        written += snprintf(buffer + written, sizeof(buffer) - written, "&key=%s", req->key);
    }

    if (req->tracker_id != NULL) {
        written += snprintf(buffer + written, sizeof(buffer) - written, "&trackerid=%s", req->tracker_id);
    }

    written += snprintf(buffer + written, sizeof(buffer) - written, " HTTP/1.1\r\n");
    written += snprintf(buffer + written, sizeof(buffer) - written, "Host: %s\r\n\r\n", url->host);

    if ((size_t)written >= sizeof(buffer)) {
        LOG_ERROR("[tracker.c] HTTP request buffer too small");
        return NULL;
    }
    buffer[written] = '\0';

    char *http_req = malloc(written + 1);
    if (http_req == NULL) {
        LOG_ERROR("[tracker.c] Failed to allocate memory for HTTP request");
        return NULL;
    }

    strcpy(http_req, buffer);
    return http_req;
}

static int tracker_send(int sockfd, const char *http_req) {
    if (sockfd == -1 || http_req == NULL) {
        LOG_WARN("[tracker.c] Must provide a socket file descriptor and an HTTP request");
        return -1;
    }

    if (send(sockfd, http_req, strlen(http_req), 0) == -1) {
        LOG_ERROR("[tracker.c] Failed to send HTTP request to tracker");
        return -1;
    }

    return 0;
}

static char *extract_body_from_res(char *http_res) {
    if (http_res == NULL) {
        LOG_WARN("[tracker.c] Must provide an HTTP response");
        return NULL;
    }

    char *body = strstr(http_res, "\r\n\r\n");
    if (body == NULL) {
        LOG_ERROR("[tracker.c] Failed to extract body from HTTP response");
        return NULL;
    }

    // +4 to skip the \r\n\r\n
    return body + 4;
}

static char *tracker_recv(int sockfd) {
    if (sockfd == -1) {
        LOG_WARN("[tracker.c] Must provide a socket file descriptor");
        return NULL;
    }

    char buffer[4096];
    ssize_t total_recv = 0;
    ssize_t bytes_recv = 0;

    do {
        bytes_recv = recv(sockfd, buffer, sizeof(buffer), 0);
        if (bytes_recv == -1) {
            LOG_ERROR("[tracker.c] Failed to receive data from tracker");
            return NULL;
        }

        total_recv += bytes_recv;

        if (bytes_recv < 4096) {
            break;
        }
    } while (bytes_recv > 0);

    if (total_recv == 0) {
        LOG_ERROR("[tracker.c] Failed to receive data from tracker");
        return NULL;
    }

    if (total_recv > 4096) {
        LOG_ERROR("[tracker.c] Tracker response larger than buffer: %ld", total_recv);
        return NULL;
    }

    buffer[total_recv] = '\0';

    LOG_DEBUG("[tracker.c] Buffer: %s", buffer);

    return strdup(extract_body_from_res(buffer));
}

static list_t *parse_peer_list_compact(byte_str_t *peers_str) {
    if (peers_str == NULL) {
        LOG_WARN("[tracker.c] Must provide a byte string");
        return NULL;
    }

    assert(peers_str->len % 6 == 0 && "Invalid compact peer list");

    list_t *peers = list_create(NULL);
    if (peers == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < peers_str->len; i += 6) {
        peer_t peer;
        memset(&peer, 0, sizeof(peer));

        // In compact mode, the first 4 bytes are the IP address
        // and the last 2 bytes are the port

        uint32_t ip;
        memcpy(&ip, peers_str->data + i, sizeof(uint32_t));
        ip = ntohl(ip);

        uint16_t port;
        memcpy(&port, peers_str->data + i + sizeof(uint32_t), sizeof(uint16_t));
        port = ntohs(port);

        peer.addr.sin_family = AF_INET;
        peer.addr.sin_addr.s_addr = htonl(ip);
        peer.addr.sin_port = htons(port);

        if (list_push(peers, &peer, sizeof(peer))) {
            list_free(peers);
            return NULL;
        }

        LOG_DEBUG("[tracker.c] Peer addr: %s:%hu", inet_ntoa(peer.addr.sin_addr), ntohs(peer.addr.sin_port));
    }

    return peers;
}

// WARN: Not tested
static list_t* parse_peer_list_dict(list_t *peers_list) {
    if (peers_list == NULL) {
        LOG_WARN("[tracker.c] Must provide a dictionary");
        return NULL;
    }

    list_t *peers = list_create(NULL);
    if (peers == NULL) {
        return NULL;
    }

    for (const list_iterator_t *it = list_iterator_first(peers_list); it != NULL; it = list_iterator_next(it)) {
        bencode_node_t *peer_node = list_iterator_get(it);
        if (peer_node->type != BENCODE_DICT) {
            LOG_ERROR("[tracker.c] Invalid peer node type in tracker response");
            list_free(peers);
            return NULL;
        }
        dict_t *peer_dict = peer_node->value.d;

        peer_t peer;
        memset(&peer, 0, sizeof(peer));

        bencode_node_t *peer_id_node = dict_get(peer_dict, "peer id");
        if (peer_id_node == NULL) {
            LOG_ERROR("[tracker.c] Missing peer ID in peer dictionary");
            list_free(peers);
            return NULL;
        }

        if (peer_id_node->type != BENCODE_STR) {
            LOG_ERROR("[tracker.c] Invalid peer ID type in peer dictionary");
            list_free(peers);
            return NULL;
        }

        if (peer_id_node->value.s->len != PEER_ID_SIZE) {
            LOG_ERROR("[tracker.c] Invalid peer ID length in peer dictionary");
            list_free(peers);
            return NULL;
        }
        memcpy(peer.id, peer_id_node->value.s->data, PEER_ID_SIZE);

        bencode_node_t *ip_node = dict_get(peer_dict, "ip");
        if (ip_node == NULL) {
            LOG_ERROR("[tracker.c] Missing IP in peer dictionary");
            list_free(peers);
            return NULL;
        }

        if (ip_node->type != BENCODE_INT) {
            LOG_ERROR("[tracker.c] Invalid IP type in peer dictionary");
            list_free(peers);
            return NULL;
        }

        peer.addr.sin_family = AF_INET;
        peer.addr.sin_addr.s_addr = htonl((uint32_t)ip_node->value.i & 0xFFFFFFFF);

        bencode_node_t *port_node = dict_get(peer_dict, "port");
        if (port_node == NULL) {
            LOG_ERROR("[tracker.c] Missing port in peer dictionary");
            list_free(peers);
            return NULL;
        }

        if (port_node->type != BENCODE_INT) {
            LOG_ERROR("[tracker.c] Invalid port type in peer dictionary");
            list_free(peers);
            return NULL;
        }

        peer.addr.sin_port = htons((uint16_t)port_node->value.i & 0xFFFF);

        if (list_push(peers, &peer, sizeof(peer))) {
            list_free(peers);
            return NULL;
        }
    }

    return peers;
}

tracker_res_t *tracker_announce(torrent_t *torrent) {
    if (torrent == NULL) {
        LOG_WARN("[tracker.c] Must provide a torrent");
        return NULL;
    }

    url_t *url = url_parse(torrent->announce);
    if (url == NULL) {
        return NULL;
    }

    if (url->protocol == URL_PROTOCOL_UDP) {
        LOG_ERROR("[tracker.c] UDP tracker protocol not supported yet");
        url_free(url);
        return NULL;
    }

    int sockfd = tracker_connect(url);
    if (sockfd == -1) {
        url_free(url);
        return NULL;
    }

    tracker_req_t *req = tracker_request_create(torrent, 6881);
    if (req == NULL) {
        url_free(url);
        close(sockfd);
        return NULL;
    }

    char *http_req = build_http_request(url, req);
    if (http_req == NULL) {
        url_free(url);
        tracker_request_free(req);
        close(sockfd);
        return NULL;
    }
    url_free(url);
    tracker_request_free(req);

    LOG_DEBUG("[tracker.c] HTTP request: %s", http_req);

    if (tracker_send(sockfd, http_req) == -1) {
        free(http_req);
        close(sockfd);
        return NULL;
    }
    free(http_req);

    char *body = tracker_recv(sockfd);
    if (body == NULL) {
        close(sockfd);
        return NULL;
    }

    LOG_DEBUG("[tracker.c] Tracker response body: %s", body);

    tracker_res_t *res = parse_tracker_response(body);
    if (res == NULL) {
        free(body);
        close(sockfd);
        return NULL;
    }

    free(body);
    close(sockfd);
    return res;
}

tracker_req_t *tracker_request_create(torrent_t *torrent, uint16_t port) {
    if (torrent == NULL) {
        LOG_WARN("[tracker.c] Must provide a torrent");
        return NULL;
    }

    tracker_req_t *req = malloc(sizeof(tracker_req_t));
    if (req == NULL) {
        LOG_ERROR("[tracker.c] Failed to allocate memory for tracker request");
        return NULL;
    }

    memcpy(req->info_hash, torrent->info_hash, SHA1_DIGEST_SIZE);

    generate_peer_id();
    memcpy(req->peer_id, get_peer_id(), PEER_ID_SIZE);

    req->port = port;

    // TODO: Initialize this properly
    req->uploaded = 0;
    req->downloaded = torrent->total_down;
    req->left = torrent->pieces_left * BLOCK_SIZE;

    req->compact = true;
    req->no_peer_id = false;
    req->event = TRACKER_EVENT_EMPTY;

    // IP is not supported yet

    // NOTE: For now just use the default (50)
    req->numwant = torrent->max_peers;

    req->key = NULL;
    req->tracker_id = NULL;

    return req;
}

void tracker_request_free(tracker_req_t *req) {
    if (req == NULL) {
        LOG_WARN("[tracker.c] Trying to free NULL tracker request");
        return;
    }

    free(req->key);
    free(req->tracker_id);
    free(req);
}

tracker_res_t *parse_tracker_response(char *bencode_str) {
    const char *endptr;
    bencode_node_t *node = bencode_parse(bencode_str, &endptr);
    if (node == NULL) {
        LOG_ERROR("[tracker.c] Failed to parse tracker response");
        return NULL;
    }

    assert(node->type == BENCODE_DICT && "Expected a dictionary node");
    dict_t *dict = node->value.d;

    tracker_res_t *res = malloc(sizeof(tracker_res_t));
    if (res == NULL) {
        LOG_ERROR("[tracker.c] Failed to allocate memory for tracker response");
        bencode_free(node);
        return NULL;
    }

    bencode_node_t *failure_reason_node = dict_get(dict, "failure reason");
    if (failure_reason_node != NULL) {
        res->failure_reason = strdup((char *)failure_reason_node->value.s->data);
        if (res->failure_reason == NULL) {
            LOG_ERROR("[tracker.c] Failed to allocate memory for failure reason");
            bencode_free(node);
            free(res);
            return NULL;
        }
    } else {
        res->failure_reason = NULL;
    }

    bencode_node_t *warning_message_node = dict_get(dict, "warning message");
    if (warning_message_node != NULL) {
        res->warning_message = strdup((char *)warning_message_node->value.s->data);
        if (res->warning_message == NULL) {
            LOG_ERROR("[tracker.c] Failed to allocate memory for warning message");
            bencode_free(node);
            free(res->failure_reason);
            free(res);
            return NULL;
        }
    } else {
        res->warning_message = NULL;
    }

    bencode_node_t *interval_node = dict_get(dict, "interval");
    if (interval_node != NULL) {
        res->interval = interval_node->value.i;
    } else {
        LOG_ERROR("[tracker.c] Missing interval in tracker response");
        bencode_free(node);
        free(res->failure_reason);
        free(res->warning_message);
        free(res);
        return NULL;
    }

    bencode_node_t *min_interval_node = dict_get(dict, "min interval");
    if (min_interval_node != NULL) {
        res->min_interval = min_interval_node->value.i;
    } else {
        res->min_interval = 0;
    }

    bencode_node_t *tracker_id_node = dict_get(dict, "tracker id");
    if (tracker_id_node != NULL) {
        res->tracker_id = strdup((char *)tracker_id_node->value.s->data);
        if (res->tracker_id == NULL) {
            LOG_ERROR("[tracker.c] Failed to allocate memory for tracker ID");
            bencode_free(node);
            free(res->failure_reason);
            free(res->warning_message);
            free(res);
            return NULL;
        }
    } else {
        res->tracker_id = NULL;
    }

    bencode_node_t *complete_node = dict_get(dict, "complete");
    if (complete_node != NULL) {
        res->complete = complete_node->value.i;
    } else {
        LOG_ERROR("[tracker.c] Missing complete in tracker response");
        bencode_free(node);
        free(res->failure_reason);
        free(res->warning_message);
        free(res->tracker_id);
        free(res);
        return NULL;
    }

    bencode_node_t *incomplete_node = dict_get(dict, "incomplete");
    if (incomplete_node != NULL) {
        res->incomplete = incomplete_node->value.i;
    } else {
        LOG_ERROR("[tracker.c] Missing incomplete in tracker response");
        bencode_free(node);
        free(res->failure_reason);
        free(res->warning_message);
        free(res->tracker_id);
        free(res);
        return NULL;
    }

    bencode_node_t *peers_node = dict_get(dict, "peers");
    if (peers_node != NULL) {
        if (peers_node->type == BENCODE_LIST) {
            res->peers = parse_peer_list_dict(peers_node->value.l);
        } else if (peers_node->type == BENCODE_STR) {
            res->peers = parse_peer_list_compact(peers_node->value.s);
        } else {
            LOG_ERROR("[tracker.c] Invalid peers type in tracker response");
            bencode_free(node);
            free(res->failure_reason);
            free(res->warning_message);
            free(res->tracker_id);
            free(res);
            return NULL;
        }
    } else {
        LOG_ERROR("[tracker.c] Missing peers in tracker response");
        bencode_free(node);
        free(res->failure_reason);
        free(res->warning_message);
        free(res->tracker_id);
        free(res);
        return NULL;
    }

    bencode_free(node);
    return res;
}


void tracker_response_free(tracker_res_t *res) {
    if (res == NULL) {
        LOG_WARN("[tracker.c] Trying to free NULL tracker response");
        return;
    }

    free(res->failure_reason);
    free(res->warning_message);
    free(res->tracker_id);

    if (res->peers != NULL) {
        list_free(res->peers);
    }

    free(res);
}
