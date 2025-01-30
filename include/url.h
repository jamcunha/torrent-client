#ifndef URL_H
#define URL_H

#include <stdint.h>

// Schemes used in Bittorrent client
typedef enum {
    URL_SCHEME_HTTP,
    URL_SCHEME_HTTPS,
    URL_SCHEME_UDP
} scheme_t;

typedef struct {
    scheme_t scheme;
    char*    user;
    char*    password;
    char*    host;
    uint16_t port;
    char*    path;
    char*    queries; // TODO: Better queries support
} url_t;

/**
 * @brief Parse a URL
 *
 * Syntax:
 * scheme : // user : password @ host : port / path ? query # fragment
 *          (           authority           )
 *             (   userinfo   )
 *
 * @param url The URL to parse
 * @param endptr Pointer to the character after parsed string
 * @return url_t* The parsed URL
 */
url_t* url_parse(const char* url, const char** endptr);

/**
 * @brief Connects to a given url
 *
 * @param url URL pointer
 * @return socket file descriptor
 */
int url_connect(const url_t* url);

/**
 * @brief Free the URL
 *
 * @param url The URL
 */
void url_free(url_t* url);

#endif // !URL_H
