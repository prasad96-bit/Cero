#ifndef RATELIMIT_H
#define RATELIMIT_H

/* Rate limit result */
typedef enum {
    RATELIMIT_OK,
    RATELIMIT_EXCEEDED,
    RATELIMIT_ERROR
} ratelimit_result_t;

/* Check rate limit for IP address */
ratelimit_result_t ratelimit_check_ip(const char *ip_address);

/* Check rate limit for user */
ratelimit_result_t ratelimit_check_user(int user_id);

/* Clean up old rate limit entries (call periodically) */
int ratelimit_cleanup(void);

/* Reset rate limit for identifier (admin tool) */
int ratelimit_reset(const char *identifier);

#endif /* RATELIMIT_H */
