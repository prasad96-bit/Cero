#ifndef REQUEST_H
#define REQUEST_H

#include <stddef.h>

#define MAX_HEADERS 32
#define MAX_HEADER_SIZE 8192
#define MAX_BODY_SIZE 1048576  /* 1MB */

/* HTTP methods */
typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_HEAD,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_UNKNOWN
} http_method_t;

/* HTTP header */
typedef struct {
    char *name;
    char *value;
} http_header_t;

/* HTTP request */
typedef struct {
    http_method_t method;
    char path[1024];
    char query_string[2048];
    char http_version[16];

    http_header_t headers[MAX_HEADERS];
    int header_count;

    char *body;
    size_t body_length;

    /* Client info */
    char client_ip[64];
    int client_port;

    /* Parsed data */
    char *cookies[MAX_HEADERS];  /* cookie_name=value pairs */
    int cookie_count;

    /* Session and user context (populated by middleware) */
    int user_id;
    int account_id;
    char user_email[256];
    char user_role[16];
    int is_authenticated;
} http_request_t;

/* Parse HTTP request from buffer */
int request_parse(const char *buffer, size_t buffer_len, http_request_t *req);

/* Get header value by name (case-insensitive) */
const char *request_get_header(http_request_t *req, const char *name);

/* Get cookie value by name */
const char *request_get_cookie(http_request_t *req, const char *name);

/* Get query parameter by name */
char *request_get_query_param(http_request_t *req, const char *name);

/* Get POST parameter by name (application/x-www-form-urlencoded) */
char *request_get_post_param(http_request_t *req, const char *name);

/* Free request resources */
void request_free(http_request_t *req);

/* Get method string */
const char *request_method_string(http_method_t method);

#endif /* REQUEST_H */
