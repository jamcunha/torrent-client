#ifndef HTTP_H
#define HTTP_H

// For this project I don't think cookies are needed.

#include "dict.h"
#include "url.h"

#include <stdint.h>

#define HTTP_HOST "torrent-client 0.0.1"

typedef struct {
    uint16_t status_code;
    char*    status_msg;
    dict_t*  headers;
    size_t   content_length;
    uint8_t* body;
} http_response_t;

/**
 * @brief Send a HTTP GET request
 *
 * @param sockfd Socket file descriptor
 * @param url URL to send request
 * @param headers Headers to add in the request
 * @return 0 if no errors, else -1
 */
int http_send_get_request(int sockfd, url_t* url, dict_t* headers);

/**
 * @brief Receive a HTTP response
 *
 * @param sockfd Socket file descriptor
 * @return HTTP response
 */
http_response_t* http_recv_response(int sockfd);

/**
 * @brief Free HTTP Response
 *
 * @param res HTTP response
 */
void http_response_free(http_response_t* res);

#endif // !HTTP_H
