#include "url.h"

#include "log.h"

#include <assert.h>
#include <ctype.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

url_t* url_parse(const char* url, const char** endptr) {
    if (url == NULL) {
        LOG_WARN("Must provide a URL");
        return NULL;
    }

    url_t* ret = malloc(sizeof(url_t));
    if (ret == NULL) {
        LOG_ERROR("Failed to allocate memory for URL");
        return NULL;
    }

    ret->user     = NULL;
    ret->password = NULL;
    ret->host     = NULL;
    ret->port     = 0;
    ret->path     = NULL;
    ret->queries  = NULL;

    // temp value to parse url fragment if exists
    char* fragment = NULL;

    if (endptr == NULL) {
        *endptr = url;
    }

    // Validate schema
    while (**endptr != '\0' && **endptr != ':') {
        (*endptr)++;
    }

    if (**endptr == '\0') {
        LOG_ERROR("URL has no scheme");
        return NULL;
    }

    assert(**endptr == ':');
    (*endptr)++;

    if (strncmp(url, "http", 4) == 0) {
        ret->scheme = URL_SCHEME_HTTP;
    } else if (strncmp(url, "https", 5) == 0) {
        ret->scheme = URL_SCHEME_HTTPS;
    } else if (strncmp(url, "udp", 3) == 0) {
        ret->scheme = URL_SCHEME_UDP;
    } else {
        LOG_ERROR("Invalid URL scheme: %s", url);
        free(ret);
        return NULL;
    }

    if (**endptr != '/' || *(*endptr + 1) != '/') {
        ret->path = strdup(*endptr);
    } else {
        // skip "//"
        (*endptr) += 2;

        if (**endptr != '\0') {
            ret->host = ret->user = (char*)*endptr;
        }
    }

    while (**endptr != '\0') {
        switch (**endptr) {
        case ':':
            if (ret->port != 0 || ret->path || ret->queries) {
                break;
            }

            for (uint8_t i = 1; (*endptr)[i] != '\0'; ++i) {
                char c = (*endptr)[i];

                if (c == '/') {
                    if (ret->host == ret->user) {
                        ret->user = NULL;
                    }

                    ret->host = strndup(ret->host, *endptr - ret->host);
                    *endptr += i;
                    break;
                } else if (!isdigit(c)) {
                    ret->port = 0;
                    break;
                } else {
                    ret->port = 10 * ret->port + (c - '0');
                }
            }

            if (ret->port == 0) {
                ret->user = strndup(ret->user, *endptr - ret->user);

                (*endptr)++;

                ret->password = (char*)*endptr;
                ret->host     = NULL;
            }

            break;
        case '@':
            if ((ret->host && ret->host != ret->user) || ret->path
                || ret->queries || fragment) {
                (*endptr)++;
                break;
            }

            if (ret->user == NULL) {
                LOG_ERROR("Invalid userinfo");
                url_free(ret);
                return NULL;
            }

            ret->password = strndup(ret->password, *endptr - ret->password);

            (*endptr)++;
            ret->host = (char*)*endptr;

            break;
        case '[':
            if (ret->host != ret->user || ret->path || ret->queries
                || fragment) {
                (*endptr)++;
                break;
            }

            while (**endptr != '\0' && **endptr != ']') {
                (*endptr)++;
            }

            break;
        case '/':
            // Part of the path
            if (ret->path || ret->queries) {
                (*endptr)++;
                break;
            }

            if (ret->host == NULL) {
                LOG_ERROR("Expected host before the path");
                LOG_WARN("DEBUG URL:\n");
                LOG_WARN("scheme: %d", ret->scheme);
                LOG_WARN("user: %s", ret->user);
                LOG_WARN("password: %s", ret->password);
                LOG_WARN("host: %s", ret->host);
                LOG_WARN("port: %d", ret->port);
                LOG_WARN("path: %s", ret->path);
                LOG_WARN("queries: %s", ret->queries);
                url_free(ret);
                return NULL;
            }

            if (ret->host == ret->user) {
                ret->user = NULL;
            }

            ret->host = strndup(ret->host, *endptr - ret->host);

            (*endptr)++;
            ret->path = (char*)*endptr;
            break;
        case '?':
            if (ret->path == NULL) {
                LOG_ERROR("Expected path before queries");
                url_free(ret);
                return NULL;
            }

            // Queries are parsed afterwards so no need to check for more '?'
            if (ret->queries) {
                (*endptr)++;
                break;
            }

            ret->path = strndup(ret->path, *endptr - ret->path);

            (*endptr)++;
            ret->queries = (char*)*endptr;

            break;
        case '#':
            if (ret->path == NULL) {
                LOG_ERROR("Expected path before fragments");
                url_free(ret);
                return NULL;
            }

            if (fragment) {
                (*endptr)++;
                break;
            }

            if (ret->queries == NULL) {
                ret->path = strndup(ret->path, *endptr - ret->path);
            } else {
                ret->queries = strndup(ret->queries, *endptr - ret->queries);
            }

            (*endptr)++;
            fragment = (char*)*endptr;
            break;
        default:
            (*endptr)++;
        }
    }

    if (ret->path && ret->queries == NULL && fragment == NULL) {
        ret->path = strndup(ret->path, *endptr - ret->path);
    }

    if (ret->queries && fragment == NULL) {
        ret->queries = strndup(ret->queries, *endptr - ret->queries);
    }

    if (ret->port == 0) {
        if (ret->scheme == URL_SCHEME_HTTP) {
            ret->port = 80;
        } else if (ret->scheme == URL_SCHEME_HTTPS) {
            ret->port = 443;
        }
    }

    return ret;
}

int url_connect(const url_t* url) {
    if (url == NULL) {
        LOG_WARN("Must provide a url");
        return -1;
    }

    struct addrinfo  hints;
    struct addrinfo *res, *rp = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype
        = url->scheme == URL_SCHEME_UDP ? SOCK_DGRAM : SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags    = 0;

    char port[6];
    snprintf(port, sizeof(port), "%hu", url->port);

    int s = getaddrinfo(url->host, port, &hints, &res);
    if (s != 0) {
        LOG_ERROR("Failed to get address info: %s", gai_strerror(s));
        return -1;
    }

    int sockfd;
    for (rp = res; rp != NULL; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1) {
            continue;
        }

        if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }

        close(sockfd);
        sockfd = -1;
    }

    freeaddrinfo(res);
    return sockfd;
}

void url_free(url_t* url) {
    if (url == NULL) {
        LOG_WARN("Trying to free NULL URL");
        return;
    }

    free(url->user);
    free(url->password);
    free(url->host);
    free(url->path);
    free(url->queries);
    free(url);
}
