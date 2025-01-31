#include "http.h"

#include "dict.h"
#include "log.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#define HTTP_VERSION     "1.1"
#define HTTP_BUFFER_SIZE 4096

static uint8_t* read_line(uint8_t** endptr) {
    uint8_t* line = *endptr;

    while (**endptr != '\0' && **endptr != '\r') {
        (*endptr)++;
    }

    if (**endptr == '\0' || *(*endptr + 1) != '\n') {
        LOG_ERROR("Expected \\n after \\r, got 0x%02x", **endptr);
        return NULL;
    }

    *endptr += 2; // point to byte after '\n'
    return line;
}

int http_send_get_request(int sockfd, url_t* url, dict_t* headers) {
    if (sockfd < 0) {
        LOG_WARN("Invalid sockfd %d", sockfd);
        return -1;
    }

    if (url == NULL) {
        LOG_WARN("URL is a required param");
        return -1;
    }

    dprintf(sockfd, "GET /");

    if (url->path) {
        dprintf(sockfd, "%s", url->path);
    }

    if (url->queries) {
        dprintf(sockfd, "?%s", url->queries);
    }

    // TODO: macro for http version
    dprintf(sockfd, " HTTP/%s\r\n", HTTP_VERSION);

    dprintf(sockfd, "Host: %s", url->host);
    if (url->port != 0) {
        dprintf(sockfd, ":%hu", url->port);
    }
    dprintf(sockfd, "\r\n");

    dprintf(sockfd, "User-Agent: %s\r\n", HTTP_HOST);
    dprintf(sockfd, "Accept: */*\r\n");

    if (headers) {
        for (const dict_iterator_t* it = dict_iterator_first(headers);
             it != NULL; it            = dict_iterator_next(headers, it)) {
            const char* key   = dict_iterator_key(it);
            const char* value = (char*)dict_iterator_value(it);

            // This code wrongly assumes that every header
            // is a valid header.
            // In the future, safeguards should be added.
            dprintf(sockfd, "%s: %s\r\n", key, value);
        }
    }

    dprintf(sockfd, "\r\n");
    return 0;
}

http_response_t* http_recv_response(int sockfd) {
    if (sockfd < 0) {
        LOG_WARN("Invalid sockfd: %d", sockfd);
        return NULL;
    }

    uint8_t buffer[HTTP_BUFFER_SIZE + 1];
    ssize_t bytes_recv = recv(sockfd, buffer, HTTP_BUFFER_SIZE, 0);

    if (bytes_recv == -1) {
        LOG_ERROR("Failed to receive data from socket");
        return NULL;
    }

    if (bytes_recv == HTTP_BUFFER_SIZE) {
        LOG_ERROR("TODO: response may be larger than buffer");
        return NULL;
    }

    buffer[bytes_recv] = '\0';

    LOG_DEBUG("Buffer: %s\n", (char*)buffer);

    http_response_t* res = (http_response_t*)malloc(sizeof(http_response_t));
    res->status_msg      = NULL;
    res->body            = NULL;

    res->headers = dict_create(15, NULL);
    if (res->headers == NULL) {
        free(res);
        return NULL;
    }

    uint8_t* endptr = buffer;
    uint8_t* line   = read_line(&endptr);
    if (line == NULL) {
        http_response_free(res);
        return NULL;
    }

    // Getting HTTP version

    uint8_t* spc = (uint8_t*)strchr((char*)line, ' ');
    if (spc == NULL || strncmp((char*)line, "HTTP/", 5) != 0) {
        if (spc == NULL) {
            *endptr = '\0';
        } else {
            *spc = '\0';
        }

        LOG_ERROR("Recevied wrong protocol: %s", line);
        http_response_free(res);
        return NULL;
    }

    if (strncmp((char*)line + 5, HTTP_VERSION, strlen(HTTP_VERSION)) != 0) {
        *spc = '\0';

        LOG_ERROR("Expected version %s, got %s", HTTP_VERSION, line + 5);
        http_response_free(res);
        return NULL;
    }

    // Getting Status

    uint8_t* status = spc + 1;
    spc             = (uint8_t*)strchr((char*)status, ' ');

    if (spc == NULL) {
        *endptr = '\0';
        LOG_ERROR("Couldn't find status code: %s", line);
        http_response_free(res);
        return NULL;
    }

    *spc             = '\0';
    res->status_code = (uint16_t)atoi((char*)status);
    *spc             = ' ';

    // Getting Status Message

    res->status_msg = strndup((char*)spc + 1, endptr - spc - 1);

    while ((line = read_line(&endptr))) {
        if (*line == '\r' && *(line + 1) == '\n') {
            break;
        }

        uint8_t* colon = (uint8_t*)strchr((char*)line, ':');
        if (colon == NULL) {
            LOG_ERROR("Did not found ':' in header");
            http_response_free(res);
            return NULL;
        }

        *colon         = '\0';
        uint8_t* value = colon + 1;
        while (value != endptr && *value == ' ') {
            value++;
        }

        if (dict_add(res->headers, (char*)line, value, endptr - value) == -1) {
            http_response_free(res);
            return NULL;
        }

        *colon = ':';
    }

    // Getting Content Length

    char* cl = (char*)dict_get(res->headers, "Content-Length");
    if (cl == NULL) {
        LOG_ERROR("Content-Length header not found");
        http_response_free(res);
        return NULL;
    }

    if (*cl == '-') {
        LOG_ERROR("Content-Length cannot be negative");
        http_response_free(res);
        return NULL;
    }

    if (sscanf(cl, "%zu", &res->content_length) != 1) {
        LOG_ERROR("Invalid Content-Length: %s", cl);
        http_response_free(res);
        return NULL;
    }

    // Getting Body

    res->body = calloc(res->content_length + 1, sizeof(uint8_t));

    // If buffer has all body content just copy it
    if (res->content_length + (endptr - buffer) <= (size_t)bytes_recv) {
        memcpy(res->body, endptr, res->content_length);
        return res;
    }

    // Copy part of body already in the buffer
    size_t body_size = bytes_recv - (endptr - buffer);
    memcpy(res->body, endptr, body_size);

    // Get remaining of the body
    while (body_size < res->content_length) {
        bytes_recv = recv(sockfd, res->body + body_size,
                          res->content_length - body_size, 0);

        if (bytes_recv == 0) {
            LOG_ERROR("No more data to read. Read %zu/%zu bytes", body_size,
                      res->content_length - body_size);
            http_response_free(res);
            return NULL;
        }

        if (bytes_recv == -1) {
            LOG_ERROR("Failed to receive data from socket");
            http_response_free(res);
            return NULL;
        }

        body_size += bytes_recv;
    }

    return res;
}

void http_response_free(http_response_t* res) {
    if (res == NULL) {
        LOG_WARN("Trying to free a NULL response");
        return;
    }

    free(res->status_msg);
    free(res->body);
    dict_free(res->headers);

    free(res);
}
