#ifndef RESPONSE_H
#define RESPONSE_H

#include <stddef.h>

#define MAX_RESPONSE_HEADERS 32

/* HTTP response structure */
typedef struct {
    int status_code;
    char status_message[64];

    char *headers[MAX_RESPONSE_HEADERS];
    int header_count;

    char *body;
    size_t body_length;
    size_t body_capacity;
} http_response_t;

/* Create new response */
http_response_t *response_new(void);

/* Set response status */
void response_set_status(http_response_t *resp, int status_code);

/* Add response header */
void response_add_header(http_response_t *resp, const char *name, const char *value);

/* Set cookie */
void response_set_cookie(http_response_t *resp, const char *name, const char *value,
                        int max_age, int http_only, int secure, const char *same_site);

/* Delete cookie */
void response_delete_cookie(http_response_t *resp, const char *name);

/* Set response body */
void response_set_body(http_response_t *resp, const char *body);

/* Append to response body */
void response_append_body(http_response_t *resp, const char *data);

/* Set content type */
void response_set_content_type(http_response_t *resp, const char *content_type);

/* Redirect to URL */
void response_redirect(http_response_t *resp, const char *url, int permanent);

/* Build response as HTTP string - caller must free result */
char *response_build(http_response_t *resp);

/* Free response */
void response_free(http_response_t *resp);

/* Common status codes */
#define HTTP_200_OK 200
#define HTTP_201_CREATED 201
#define HTTP_204_NO_CONTENT 204
#define HTTP_301_MOVED_PERMANENTLY 301
#define HTTP_302_FOUND 302
#define HTTP_303_SEE_OTHER 303
#define HTTP_304_NOT_MODIFIED 304
#define HTTP_400_BAD_REQUEST 400
#define HTTP_401_UNAUTHORIZED 401
#define HTTP_403_FORBIDDEN 403
#define HTTP_404_NOT_FOUND 404
#define HTTP_405_METHOD_NOT_ALLOWED 405
#define HTTP_429_TOO_MANY_REQUESTS 429
#define HTTP_500_INTERNAL_SERVER_ERROR 500
#define HTTP_503_SERVICE_UNAVAILABLE 503

#endif /* RESPONSE_H */
