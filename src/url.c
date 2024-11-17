#include "url.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>

url_t *url_parse(const char *url) {
    if (url == NULL) {
        LOG_WARN("Must provide a URL");
        return NULL;
    }

    url_t *parsed = malloc(sizeof(url_t));
    if (parsed == NULL) {
        LOG_ERROR("Failed to allocate memory for URL");
        return NULL;
    }

    if (strncmp(url, "http://", 7) == 0) {
        parsed->protocol = URL_PROTOCOL_HTTP;
        url += 7;
    } else if (strncmp(url, "https://", 8) == 0) {
        parsed->protocol = URL_PROTOCOL_HTTPS;
        url += 8;
    } else if (strncmp(url, "udp://", 6) == 0) {
        parsed->protocol = URL_PROTOCOL_UDP;
        url += 6;
    } else {
        LOG_WARN("Invalid URL protocol");
        free(parsed);
        return NULL;
    }

    const char *hostnanme_end = url;
    while (*hostnanme_end != '\0' && *hostnanme_end != '/' && *hostnanme_end != ':') {
        hostnanme_end++;
    }

    parsed->host = malloc(hostnanme_end - url + 1);
    if (parsed->host == NULL) {
        LOG_ERROR("Failed to allocate memory for hostname");
        free(parsed);
        return NULL;
    }

    memcpy(parsed->host, url, hostnanme_end - url);

    url = hostnanme_end;

    if (*url == ':') {
        url++;
        parsed->port = (uint16_t)strtol(url, (char **)&url, 10);
    } else {
        if (parsed->protocol == URL_PROTOCOL_HTTP) {
            parsed->port = 80;
        } else if (parsed->protocol == URL_PROTOCOL_HTTPS) {
            parsed->port = 443;
        } else if (parsed->protocol == URL_PROTOCOL_UDP) {
            // No default port
            parsed->port = 0;
        }
    }

    const char *path_end = url;
    while (*path_end != '\0') {
        path_end++;
    }

    parsed->path = malloc(path_end - url + 1);
    if (parsed->path == NULL) {
        LOG_ERROR("Failed to allocate memory for path");
        free(parsed->host);
        free(parsed);
        return NULL;
    }
    memcpy(parsed->path, url, path_end - url);

    return parsed;
}

void url_free(url_t *url) {
    if (url == NULL) {
        LOG_WARN("Trying to free NULL URL");
        return;
    }

    free(url->host);
    free(url->path);
    free(url);
}
