/*
 * Authentication system
 * Handles user login, password hashing, and authentication
 */

#define _GNU_SOURCE
#include "auth.h"
#include "session.h"
#include "../utils/db.h"
#include "../utils/log.h"
#include "../utils/string_utils.h"
#include "../templates/template.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <crypt.h>
#include <time.h>

/* Hash password using crypt (SHA-512) */
int auth_hash_password(const char *password, char *hash, size_t hash_size) {
    /* Generate salt for SHA-512 ($6$ prefix) */
    char *random_hex = generate_random_hex(16);
    if (!random_hex) {
        LOG_ERROR("auth", "Failed to generate salt");
        return -1;
    }

    char salt[64];
    snprintf(salt, sizeof(salt), "$6$%s", random_hex);
    free(random_hex);

    /* Hash password */
    char *hashed = crypt(password, salt);
    if (!hashed) {
        LOG_ERROR("auth", "Failed to hash password");
        return -1;
    }

    /* Copy result */
    safe_strncpy(hash, hashed, hash_size);
    return 0;
}

/* Verify password against hash */
int auth_verify_password(const char *password, const char *hash) {
    char *result = crypt(password, hash);
    if (!result) {
        LOG_ERROR("auth", "Failed to verify password");
        return 0;
    }

    return strcmp(result, hash) == 0 ? 1 : 0;
}

/* Authenticate user with email and password */
int auth_authenticate_user(const char *email, const char *password) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT id, password_hash, is_active FROM users WHERE email = ?";

    if (db_prepare(sql, &stmt) != 0) {
        LOG_ERROR("auth", "Failed to prepare authentication query");
        return -1;
    }

    db_bind_text(stmt, 1, email);

    int result = sqlite3_step(stmt);
    if (result != SQLITE_ROW) {
        LOG_INFO("auth", "User not found: %s", email);
        db_finalize(stmt);
        return -1;
    }

    int user_id = db_column_int(stmt, 0);
    const char *password_hash = db_column_text(stmt, 1);
    int is_active = db_column_int(stmt, 2);

    /* Check if user is active */
    if (!is_active) {
        LOG_WARN("auth", "Inactive user attempted login: %s", email);
        db_finalize(stmt);
        return -1;
    }

    /* Verify password */
    int password_valid = auth_verify_password(password, password_hash);
    db_finalize(stmt);

    if (!password_valid) {
        LOG_WARN("auth", "Invalid password for user: %s", email);
        return -1;
    }

    LOG_INFO("auth", "User authenticated successfully: %s (ID: %d)", email, user_id);
    return user_id;
}

/* Create user account */
int auth_create_user(int account_id, const char *email, const char *password, const char *role) {
    char password_hash[256];

    /* Hash password */
    if (auth_hash_password(password, password_hash, sizeof(password_hash)) != 0) {
        LOG_ERROR("auth", "Failed to hash password for new user");
        return -1;
    }

    /* Insert user */
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO users (account_id, email, password_hash, role, is_active, created_at) "
                      "VALUES (?, ?, ?, ?, 1, ?)";

    if (db_prepare(sql, &stmt) != 0) {
        LOG_ERROR("auth", "Failed to prepare user creation query");
        return -1;
    }

    time_t now = time(NULL);
    db_bind_int(stmt, 1, account_id);
    db_bind_text(stmt, 2, email);
    db_bind_text(stmt, 3, password_hash);
    db_bind_text(stmt, 4, role);
    db_bind_int64(stmt, 5, now);

    int result = sqlite3_step(stmt);
    db_finalize(stmt);

    if (result != SQLITE_DONE) {
        LOG_ERROR("auth", "Failed to create user: %s", db_error_message());
        return -1;
    }

    int user_id = (int)db_last_insert_rowid();
    LOG_INFO("auth", "Created user: %s (ID: %d)", email, user_id);

    return user_id;
}

/* Update user last login timestamp */
int auth_update_last_login(int user_id) {
    sqlite3_stmt *stmt;
    const char *sql = "UPDATE users SET last_login_at = ? WHERE id = ?";

    if (db_prepare(sql, &stmt) != 0) {
        LOG_ERROR("auth", "Failed to prepare login update query");
        return -1;
    }

    time_t now = time(NULL);
    db_bind_int64(stmt, 1, now);
    db_bind_int(stmt, 2, user_id);

    int result = sqlite3_step(stmt);
    db_finalize(stmt);

    if (result != SQLITE_DONE) {
        LOG_ERROR("auth", "Failed to update last login");
        return -1;
    }

    return 0;
}

/* Route handler: Login page */
http_response_t *handle_login_page(http_request_t *req) {
    http_response_t *resp = response_new();
    response_set_content_type(resp, "text/html");

    /* If already authenticated, redirect to dashboard */
    if (req->is_authenticated) {
        response_redirect(resp, "/dashboard", 0);
        return resp;
    }

    /* Render login form */
    template_ctx_t *ctx = template_ctx_new();
    template_set(ctx, "title", "Login");

    char *html = template_render_file("login.html", ctx);
    if (!html) {
        /* Fallback to simple HTML */
        response_set_body(resp,
            "<html><head><title>Login</title></head><body>"
            "<h1>Login</h1>"
            "<form method=\"POST\" action=\"/login\">"
            "<p><label>Email: <input type=\"email\" name=\"email\" required></label></p>"
            "<p><label>Password: <input type=\"password\" name=\"password\" required></label></p>"
            "<p><button type=\"submit\">Login</button></p>"
            "</form>"
            "</body></html>");
    } else {
        response_set_body(resp, html);
        free(html);
    }

    template_ctx_free(ctx);
    return resp;
}

/* Route handler: Login form submission */
http_response_t *handle_login_submit(http_request_t *req) {
    http_response_t *resp = response_new();

    /* Get POST parameters */
    char *email = request_get_post_param(req, "email");
    char *password = request_get_post_param(req, "password");

    if (!email || !password) {
        LOG_WARN("auth", "Missing email or password in login request");
        response_set_status(resp, 400);
        response_set_content_type(resp, "text/html");
        response_set_body(resp, "<h1>Bad Request</h1><p><a href=\"/login\">Try again</a></p>");
        free(email);
        free(password);
        return resp;
    }

    /* Authenticate user */
    int user_id = auth_authenticate_user(email, password);
    free(password); /* Clear password from memory */

    if (user_id < 0) {
        LOG_INFO("auth", "Failed login attempt for: %s", email);
        response_set_content_type(resp, "text/html");
        response_set_body(resp,
            "<html><head><title>Login Failed</title></head><body>"
            "<h1>Login Failed</h1>"
            "<p>Invalid email or password.</p>"
            "<p><a href=\"/login\">Try again</a></p>"
            "</body></html>");
        free(email);
        return resp;
    }

    /* Update last login */
    auth_update_last_login(user_id);

    /* Create session */
    char session_token[65];
    int session_result = session_create(user_id, req->client_ip,
                                       request_get_header(req, "User-Agent"),
                                       session_token, sizeof(session_token));

    free(email);

    if (session_result != 0) {
        LOG_ERROR("auth", "Failed to create session for user %d", user_id);
        response_set_status(resp, 500);
        response_set_content_type(resp, "text/html");
        response_set_body(resp, "<h1>Error</h1><p>Failed to create session</p>");
        return resp;
    }

    /* Set session cookie */
    response_set_cookie(resp, "session_token", session_token,
                       86400 * 7, /* 7 days */
                       1, /* HttpOnly */
                       0, /* Not Secure (use 1 in production with HTTPS) */
                       "Strict");

    /* Redirect to dashboard */
    response_redirect(resp, "/dashboard", 0);

    LOG_INFO("auth", "User %d logged in successfully", user_id);
    return resp;
}

/* Route handler: Logout */
http_response_t *handle_logout(http_request_t *req) {
    http_response_t *resp = response_new();

    /* Get session token from cookie */
    const char *token = request_get_cookie(req, "session_token");

    if (token) {
        /* Delete session from database */
        session_delete(token);
        LOG_INFO("auth", "User logged out");
    }

    /* Delete session cookie */
    response_delete_cookie(resp, "session_token");

    /* Redirect to home */
    response_redirect(resp, "/", 0);

    return resp;
}
