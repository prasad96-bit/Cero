/*
 * HTTP request parsing
 * Handles parsing of incoming HTTP requests
 */

#include "request.h"
#include "../utils/string_utils.h"
#include "../utils/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* Parse HTTP request from buffer */
int request_parse(const char *buffer, size_t buffer_len, http_request_t *req) {
    memset(req, 0, sizeof(*req));

    /* Make a copy of the buffer we can modify */
    char *buf_copy = strndup(buffer, buffer_len);
    if (!buf_copy) {
        LOG_ERROR("request", "Failed to allocate memory for request parsing");
        return -1;
    }

    /* Parse request line: "GET /path HTTP/1.1" */
    char *line_end = strstr(buf_copy, "\r\n");
    if (!line_end) {
        LOG_ERROR("request", "Invalid HTTP request - no CRLF found");
        free(buf_copy);
        return -1;
    }

    *line_end = '\0';
    char *request_line = buf_copy;

    /* Parse method */
    char method_str[16];
    char path[1024];
    char version[16];

    if (sscanf(request_line, "%15s %1023s %15s", method_str, path, version) != 3) {
        LOG_ERROR("request", "Failed to parse request line");
        free(buf_copy);
        return -1;
    }

    /* Convert method string to enum */
    if (strcmp(method_str, "GET") == 0) req->method = HTTP_GET;
    else if (strcmp(method_str, "POST") == 0) req->method = HTTP_POST;
    else if (strcmp(method_str, "HEAD") == 0) req->method = HTTP_HEAD;
    else if (strcmp(method_str, "PUT") == 0) req->method = HTTP_PUT;
    else if (strcmp(method_str, "DELETE") == 0) req->method = HTTP_DELETE;
    else req->method = HTTP_UNKNOWN;

    /* Split path and query string */
    char *query = strchr(path, '?');
    if (query) {
        *query = '\0';
        safe_strncpy(req->query_string, query + 1, sizeof(req->query_string));
    } else {
        req->query_string[0] = '\0';
    }

    safe_strncpy(req->path, path, sizeof(req->path));
    safe_strncpy(req->http_version, version, sizeof(req->http_version));

    /* Parse headers */
    char *header_start = line_end + 2; /* Skip \r\n */
    while (1) {
        line_end = strstr(header_start, "\r\n");
        if (!line_end) break;

        *line_end = '\0';

        /* Empty line marks end of headers */
        if (header_start[0] == '\0') {
            header_start = line_end + 2;
            break;
        }

        if (req->header_count >= MAX_HEADERS) {
            LOG_WARN("request", "Too many headers, ignoring remaining");
            break;
        }

        /* Find colon separator */
        char *colon = strchr(header_start, ':');
        if (!colon) {
            header_start = line_end + 2;
            continue;
        }

        *colon = '\0';
        char *header_name = header_start;
        char *header_value = colon + 1;

        /* Trim whitespace from value */
        while (*header_value == ' ' || *header_value == '\t') {
            header_value++;
        }

        req->headers[req->header_count].name = strdup(header_name);
        req->headers[req->header_count].value = strdup(header_value);

        /* Parse cookies from Cookie header */
        if (strcasecmp(header_name, "Cookie") == 0) {
            char *cookie_copy = strdup(header_value);
            char *cookie = cookie_copy;

            while (cookie && req->cookie_count < MAX_HEADERS) {
                /* Skip whitespace */
                while (*cookie == ' ') cookie++;

                /* Find next cookie (separated by semicolon) */
                char *next = strchr(cookie, ';');
                if (next) {
                    *next = '\0';
                    next++;
                }

                /* Store cookie */
                if (*cookie) {
                    req->cookies[req->cookie_count] = strdup(cookie);
                    req->cookie_count++;
                }

                cookie = next;
            }

            free(cookie_copy);
        }

        req->header_count++;
        header_start = line_end + 2;
    }

    /* Parse body (for POST/PUT requests) */
    const char *body_start = strstr(buffer, "\r\n\r\n");
    if (body_start && (req->method == HTTP_POST || req->method == HTTP_PUT)) {
        body_start += 4;
        size_t body_len = buffer_len - (body_start - buffer);

        if (body_len > 0 && body_len <= MAX_BODY_SIZE) {
            req->body = malloc(body_len + 1);
            if (req->body) {
                memcpy(req->body, body_start, body_len);
                req->body[body_len] = '\0';
                req->body_length = body_len;
            }
        }
    }

    free(buf_copy);
    return 0;
}

/* Get header value by name (case-insensitive) */
const char *request_get_header(http_request_t *req, const char *name) {
    for (int i = 0; i < req->header_count; i++) {
        if (strcasecmp(req->headers[i].name, name) == 0) {
            return req->headers[i].value;
        }
    }
    return NULL;
}

/* Get cookie value by name */
const char *request_get_cookie(http_request_t *req, const char *name) {
    size_t name_len = strlen(name);

    for (int i = 0; i < req->cookie_count; i++) {
        /* Format is "name=value" */
        if (strncmp(req->cookies[i], name, name_len) == 0 &&
            req->cookies[i][name_len] == '=') {
            return req->cookies[i] + name_len + 1;
        }
    }

    return NULL;
}

/* Get query parameter by name */
char *request_get_query_param(http_request_t *req, const char *name) {
    if (req->query_string[0] == '\0') return NULL;

    char *query_copy = strdup(req->query_string);
    if (!query_copy) return NULL;

    char *result = NULL;
    char *saveptr;
    char *param = strtok_r(query_copy, "&", &saveptr);

    while (param) {
        char *equals = strchr(param, '=');
        if (equals) {
            *equals = '\0';
            if (strcmp(param, name) == 0) {
                result = strdup(equals + 1);
                url_decode(result);
                break;
            }
        }
        param = strtok_r(NULL, "&", &saveptr);
    }

    free(query_copy);
    return result;
}

/* Get POST parameter by name (application/x-www-form-urlencoded) */
char *request_get_post_param(http_request_t *req, const char *name) {
    if (!req->body || req->body_length == 0) return NULL;

    /* Check Content-Type */
    const char *content_type = request_get_header(req, "Content-Type");
    if (!content_type || strstr(content_type, "application/x-www-form-urlencoded") == NULL) {
        return NULL;
    }

    char *body_copy = strdup(req->body);
    if (!body_copy) return NULL;

    char *result = NULL;
    char *saveptr;
    char *param = strtok_r(body_copy, "&", &saveptr);

    while (param) {
        char *equals = strchr(param, '=');
        if (equals) {
            *equals = '\0';
            if (strcmp(param, name) == 0) {
                result = strdup(equals + 1);
                url_decode(result);
                break;
            }
        }
        param = strtok_r(NULL, "&", &saveptr);
    }

    free(body_copy);
    return result;
}

/* Free request resources */
void request_free(http_request_t *req) {
    /* Free headers */
    for (int i = 0; i < req->header_count; i++) {
        free(req->headers[i].name);
        free(req->headers[i].value);
    }

    /* Free cookies */
    for (int i = 0; i < req->cookie_count; i++) {
        free(req->cookies[i]);
    }

    /* Free body */
    if (req->body) {
        free(req->body);
        req->body = NULL;
    }
}

/* Get method string */
const char *request_method_string(http_method_t method) {
    switch (method) {
        case HTTP_GET: return "GET";
        case HTTP_POST: return "POST";
        case HTTP_HEAD: return "HEAD";
        case HTTP_PUT: return "PUT";
        case HTTP_DELETE: return "DELETE";
        default: return "UNKNOWN";
    }
}
