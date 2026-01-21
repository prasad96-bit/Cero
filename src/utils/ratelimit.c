/*
 * Rate limiting system
 * Simple in-memory rate limiting by IP address and user ID
 */

#include "ratelimit.h"
#include "db.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_REQUESTS_PER_MINUTE 60
#define RATE_LIMIT_WINDOW 60  /* 60 seconds */

/* Check rate limit for IP address */
ratelimit_result_t ratelimit_check_ip(const char *ip_address) {
    if (!ip_address) {
        return RATELIMIT_ERROR;
    }

    time_t now = time(NULL);
    time_t window_start = now - RATE_LIMIT_WINDOW;

    /* Get request count for this IP in the time window */
    sqlite3_stmt *stmt;
    const char *sql = "SELECT COUNT(*) FROM rate_limits "
                      "WHERE identifier = ? AND timestamp > ?";

    if (db_prepare(sql, &stmt) != 0) {
        LOG_ERROR("ratelimit", "Failed to prepare rate limit check query");
        return RATELIMIT_ERROR;
    }

    db_bind_text(stmt, 1, ip_address);
    db_bind_int64(stmt, 2, window_start);

    int result = sqlite3_step(stmt);
    if (result != SQLITE_ROW) {
        db_finalize(stmt);
        LOG_ERROR("ratelimit", "Failed to check rate limit");
        return RATELIMIT_ERROR;
    }

    int request_count = db_column_int(stmt, 0);
    db_finalize(stmt);

    /* Check if limit exceeded */
    if (request_count >= MAX_REQUESTS_PER_MINUTE) {
        LOG_WARN("ratelimit", "Rate limit exceeded for IP: %s (%d requests)",
                 ip_address, request_count);
        return RATELIMIT_EXCEEDED;
    }

    /* Record this request */
    const char *insert_sql = "INSERT INTO rate_limits (identifier, timestamp) VALUES (?, ?)";
    if (db_prepare(insert_sql, &stmt) != 0) {
        LOG_ERROR("ratelimit", "Failed to prepare rate limit insert query");
        return RATELIMIT_ERROR;
    }

    db_bind_text(stmt, 1, ip_address);
    db_bind_int64(stmt, 2, now);

    result = sqlite3_step(stmt);
    db_finalize(stmt);

    if (result != SQLITE_DONE) {
        LOG_ERROR("ratelimit", "Failed to record rate limit");
        return RATELIMIT_ERROR;
    }

    return RATELIMIT_OK;
}

/* Check rate limit for user */
ratelimit_result_t ratelimit_check_user(int user_id) {
    char identifier[64];
    snprintf(identifier, sizeof(identifier), "user:%d", user_id);

    time_t now = time(NULL);
    time_t window_start = now - RATE_LIMIT_WINDOW;

    /* Get request count for this user in the time window */
    sqlite3_stmt *stmt;
    const char *sql = "SELECT COUNT(*) FROM rate_limits "
                      "WHERE identifier = ? AND timestamp > ?";

    if (db_prepare(sql, &stmt) != 0) {
        LOG_ERROR("ratelimit", "Failed to prepare rate limit check query");
        return RATELIMIT_ERROR;
    }

    db_bind_text(stmt, 1, identifier);
    db_bind_int64(stmt, 2, window_start);

    int result = sqlite3_step(stmt);
    if (result != SQLITE_ROW) {
        db_finalize(stmt);
        LOG_ERROR("ratelimit", "Failed to check rate limit");
        return RATELIMIT_ERROR;
    }

    int request_count = db_column_int(stmt, 0);
    db_finalize(stmt);

    /* Check if limit exceeded */
    if (request_count >= MAX_REQUESTS_PER_MINUTE) {
        LOG_WARN("ratelimit", "Rate limit exceeded for user: %d (%d requests)",
                 user_id, request_count);
        return RATELIMIT_EXCEEDED;
    }

    /* Record this request */
    const char *insert_sql = "INSERT INTO rate_limits (identifier, timestamp) VALUES (?, ?)";
    if (db_prepare(insert_sql, &stmt) != 0) {
        LOG_ERROR("ratelimit", "Failed to prepare rate limit insert query");
        return RATELIMIT_ERROR;
    }

    db_bind_text(stmt, 1, identifier);
    db_bind_int64(stmt, 2, now);

    result = sqlite3_step(stmt);
    db_finalize(stmt);

    if (result != SQLITE_DONE) {
        LOG_ERROR("ratelimit", "Failed to record rate limit");
        return RATELIMIT_ERROR;
    }

    return RATELIMIT_OK;
}

/* Clean up old rate limit entries (call periodically) */
int ratelimit_cleanup(void) {
    time_t now = time(NULL);
    time_t cutoff = now - RATE_LIMIT_WINDOW;

    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM rate_limits WHERE timestamp < ?";

    if (db_prepare(sql, &stmt) != 0) {
        LOG_ERROR("ratelimit", "Failed to prepare cleanup query");
        return -1;
    }

    db_bind_int64(stmt, 1, cutoff);

    int result = sqlite3_step(stmt);
    db_finalize(stmt);

    if (result != SQLITE_DONE) {
        LOG_ERROR("ratelimit", "Failed to cleanup rate limits");
        return -1;
    }

    int deleted = sqlite3_changes(g_db);
    if (deleted > 0) {
        LOG_DEBUG("ratelimit", "Cleaned up %d old rate limit entries", deleted);
    }

    return deleted;
}

/* Reset rate limit for identifier (admin tool) */
int ratelimit_reset(const char *identifier) {
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM rate_limits WHERE identifier = ?";

    if (db_prepare(sql, &stmt) != 0) {
        LOG_ERROR("ratelimit", "Failed to prepare reset query");
        return -1;
    }

    db_bind_text(stmt, 1, identifier);

    int result = sqlite3_step(stmt);
    db_finalize(stmt);

    if (result != SQLITE_DONE) {
        LOG_ERROR("ratelimit", "Failed to reset rate limit");
        return -1;
    }

    LOG_INFO("ratelimit", "Reset rate limit for: %s", identifier);
    return 0;
}
