#ifndef AUTH_H
#define AUTH_H

#include "../core/request.h"
#include "../core/response.h"

/* Authenticate user with email and password */
/* Returns user_id on success, -1 on failure */
int auth_authenticate_user(const char *email, const char *password);

/* Hash password using bcrypt */
int auth_hash_password(const char *password, char *hash, size_t hash_size);

/* Verify password against hash */
int auth_verify_password(const char *password, const char *hash);

/* Create user account */
int auth_create_user(int account_id, const char *email, const char *password, const char *role);

/* Update user last login timestamp */
int auth_update_last_login(int user_id);

/* Route handlers */
http_response_t *handle_login_page(http_request_t *req);
http_response_t *handle_login_submit(http_request_t *req);
http_response_t *handle_logout(http_request_t *req);

#endif /* AUTH_H */
