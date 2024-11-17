#ifndef URL_H
#define URL_H

#include <stdint.h>

typedef enum {
    URL_PROTOCOL_HTTP,
    URL_PROTOCOL_HTTPS,
    URL_PROTOCOL_UDP
} protocol_t;

typedef struct {
    protocol_t protocol;
    char *host;
    char *path;
    uint16_t port;
} url_t;

/**
 * @brief Parse a URL
 *
 * @param url The URL to parse
 * @return url_t* The parsed URL
 */
url_t *url_parse(const char *url);

/**
 * @brief Free the URL
 *
 * @param url The URL
 */
void url_free(url_t *url);

#endif // !URL_H
