#ifndef SESSION_H
#define SESSION_H

#include "../core/request.h"
#include <time.h>

/* Session structure */
typedef struct {
    int id;
    int user_id;
    char token[65];  /* 64 hex chars + null terminator */
    time_t created_at;
    time_t expires_at;
    time_t last_activity_at;
    char ip_address[64];
    char user_agent[256];
} session_t;

/* Create new session for user */
int session_create(int user_id, const char *ip_address, const char *user_agent,
                   char *token_out, size_t token_size);

/* Validate session token and load user context */
/* Returns 1 if valid, 0 if invalid or expired */
int session_validate(const char *token, http_request_t *req);

/* Update session last activity timestamp */
int session_update_activity(const char *token);

/* Delete session (logout) */
int session_delete(const char *token);

/* Delete expired sessions (cleanup) */
int session_cleanup_expired(void);

/* Get session by token */
int session_get_by_token(const char *token, session_t *session);

#endif /* SESSION_H */
