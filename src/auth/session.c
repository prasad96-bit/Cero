/*
 * Session management
 * Handles session creation, validation, and cleanup
 */

#include "session.h"
#include "../utils/db.h"
#include "../utils/log.h"
#include "../utils/string_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SESSION_DURATION_SECONDS (86400 * 7)  /* 7 days */
#define SESSION_INACTIVITY_SECONDS (86400 * 1)  /* 1 day */

/* Create new session for user */
int session_create(int user_id, const char *ip_address, const char *user_agent,
                   char *token_out, size_t token_size) {
    /* Generate random session token */
    char *token = generate_random_hex(64);
    if (!token) {
        LOG_ERROR("session", "Failed to generate session token");
        return -1;
    }

    time_t now = time(NULL);
    time_t expires_at = now + SESSION_DURATION_SECONDS;

    /* Insert session */
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO sessions (user_id, token, created_at, expires_at, "
                      "last_activity_at, ip_address, user_agent) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?)";

    if (db_prepare(sql, &stmt) != 0) {
        LOG_ERROR("session", "Failed to prepare session insert query");
        free(token);
        return -1;
    }

    db_bind_int(stmt, 1, user_id);
    db_bind_text(stmt, 2, token);
    db_bind_int64(stmt, 3, now);
    db_bind_int64(stmt, 4, expires_at);
    db_bind_int64(stmt, 5, now);
    db_bind_text(stmt, 6, ip_address ? ip_address : "");
    db_bind_text(stmt, 7, user_agent ? user_agent : "");

    int result = sqlite3_step(stmt);
    db_finalize(stmt);

    if (result != SQLITE_DONE) {
        LOG_ERROR("session", "Failed to create session: %s", db_error_message());
        free(token);
        return -1;
    }

    /* Copy token to output */
    safe_strncpy(token_out, token, token_size);
    free(token);

    LOG_INFO("session", "Created session for user %d", user_id);
    return 0;
}

/* Validate session token and load user context */
int session_validate(const char *token, http_request_t *req) {
    if (!token || !req) {
        return 0;
    }

    sqlite3_stmt *stmt;
    const char *sql = "SELECT s.id, s.user_id, s.expires_at, s.last_activity_at, "
                      "u.email, u.role, u.account_id "
                      "FROM sessions s "
                      "JOIN users u ON s.user_id = u.id "
                      "WHERE s.token = ? AND u.is_active = 1";

    if (db_prepare(sql, &stmt) != 0) {
        LOG_ERROR("session", "Failed to prepare session validation query");
        return 0;
    }

    db_bind_text(stmt, 1, token);

    int result = sqlite3_step(stmt);
    if (result != SQLITE_ROW) {
        db_finalize(stmt);
        return 0;
    }

    /* Get session data */
    int session_id = db_column_int(stmt, 0);
    int user_id = db_column_int(stmt, 1);
    time_t expires_at = db_column_int64(stmt, 2);
    time_t last_activity_at = db_column_int64(stmt, 3);
    const char *email = db_column_text(stmt, 4);
    const char *role = db_column_text(stmt, 5);
    int account_id = db_column_int(stmt, 6);

    /* Check if session expired */
    time_t now = time(NULL);
    if (now > expires_at) {
        LOG_INFO("session", "Session %d expired", session_id);
        db_finalize(stmt);
        return 0;
    }

    /* Check inactivity timeout */
    if (now - last_activity_at > SESSION_INACTIVITY_SECONDS) {
        LOG_INFO("session", "Session %d inactive timeout", session_id);
        db_finalize(stmt);
        return 0;
    }

    /* Populate request context */
    req->user_id = user_id;
    req->account_id = account_id;
    safe_strncpy(req->user_email, email, sizeof(req->user_email));
    safe_strncpy(req->user_role, role, sizeof(req->user_role));
    req->is_authenticated = 1;

    db_finalize(stmt);

    /* Update last activity */
    session_update_activity(token);

    LOG_DEBUG("session", "Session validated for user %d (%s)", user_id, email);
    return 1;
}

/* Update session last activity timestamp */
int session_update_activity(const char *token) {
    sqlite3_stmt *stmt;
    const char *sql = "UPDATE sessions SET last_activity_at = ? WHERE token = ?";

    if (db_prepare(sql, &stmt) != 0) {
        LOG_ERROR("session", "Failed to prepare activity update query");
        return -1;
    }

    time_t now = time(NULL);
    db_bind_int64(stmt, 1, now);
    db_bind_text(stmt, 2, token);

    int result = sqlite3_step(stmt);
    db_finalize(stmt);

    if (result != SQLITE_DONE) {
        LOG_ERROR("session", "Failed to update session activity");
        return -1;
    }

    return 0;
}

/* Delete session (logout) */
int session_delete(const char *token) {
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM sessions WHERE token = ?";

    if (db_prepare(sql, &stmt) != 0) {
        LOG_ERROR("session", "Failed to prepare session delete query");
        return -1;
    }

    db_bind_text(stmt, 1, token);

    int result = sqlite3_step(stmt);
    db_finalize(stmt);

    if (result != SQLITE_DONE) {
        LOG_ERROR("session", "Failed to delete session");
        return -1;
    }

    LOG_INFO("session", "Session deleted");
    return 0;
}

/* Delete expired sessions (cleanup) */
int session_cleanup_expired(void) {
    time_t now = time(NULL);

    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM sessions WHERE expires_at < ? OR last_activity_at < ?";

    if (db_prepare(sql, &stmt) != 0) {
        LOG_ERROR("session", "Failed to prepare session cleanup query");
        return -1;
    }

    time_t inactivity_cutoff = now - SESSION_INACTIVITY_SECONDS;

    db_bind_int64(stmt, 1, now);
    db_bind_int64(stmt, 2, inactivity_cutoff);

    int result = sqlite3_step(stmt);
    db_finalize(stmt);

    if (result != SQLITE_DONE) {
        LOG_ERROR("session", "Failed to cleanup sessions");
        return -1;
    }

    int deleted = sqlite3_changes(g_db);
    if (deleted > 0) {
        LOG_INFO("session", "Cleaned up %d expired sessions", deleted);
    }

    return deleted;
}

/* Get session by token */
int session_get_by_token(const char *token, session_t *session) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT id, user_id, token, created_at, expires_at, "
                      "last_activity_at, ip_address, user_agent "
                      "FROM sessions WHERE token = ?";

    if (db_prepare(sql, &stmt) != 0) {
        LOG_ERROR("session", "Failed to prepare session get query");
        return -1;
    }

    db_bind_text(stmt, 1, token);

    int result = sqlite3_step(stmt);
    if (result != SQLITE_ROW) {
        db_finalize(stmt);
        return -1;
    }

    session->id = db_column_int(stmt, 0);
    session->user_id = db_column_int(stmt, 1);
    safe_strncpy(session->token, db_column_text(stmt, 2), sizeof(session->token));
    session->created_at = db_column_int64(stmt, 3);
    session->expires_at = db_column_int64(stmt, 4);
    session->last_activity_at = db_column_int64(stmt, 5);
    safe_strncpy(session->ip_address, db_column_text(stmt, 6), sizeof(session->ip_address));
    safe_strncpy(session->user_agent, db_column_text(stmt, 7), sizeof(session->user_agent));

    db_finalize(stmt);
    return 0;
}
