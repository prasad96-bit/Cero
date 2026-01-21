/*
 * Subscription management
 * Handles subscription lifecycle and plan management
 */

#include "subscription.h"
#include "../utils/db.h"
#include "../utils/log.h"
#include "../utils/string_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Convert plan enum to string */
const char *subscription_plan_to_string(plan_t plan) {
    switch (plan) {
        case PLAN_FREE: return "free";
        case PLAN_PRO: return "pro";
        case PLAN_ENTERPRISE: return "enterprise";
        default: return "unknown";
    }
}

/* Convert status enum to string */
const char *subscription_status_to_string(subscription_status_t status) {
    switch (status) {
        case STATUS_ACTIVE: return "active";
        case STATUS_GRACE_PERIOD: return "grace_period";
        case STATUS_EXPIRED: return "expired";
        case STATUS_CANCELLED: return "cancelled";
        default: return "unknown";
    }
}

/* Convert string to plan enum */
plan_t subscription_string_to_plan(const char *str) {
    if (strcmp(str, "free") == 0) return PLAN_FREE;
    if (strcmp(str, "pro") == 0) return PLAN_PRO;
    if (strcmp(str, "enterprise") == 0) return PLAN_ENTERPRISE;
    return PLAN_FREE;
}

/* Convert string to status enum */
subscription_status_t subscription_string_to_status(const char *str) {
    if (strcmp(str, "active") == 0) return STATUS_ACTIVE;
    if (strcmp(str, "grace_period") == 0) return STATUS_GRACE_PERIOD;
    if (strcmp(str, "expired") == 0) return STATUS_EXPIRED;
    if (strcmp(str, "cancelled") == 0) return STATUS_CANCELLED;
    return STATUS_EXPIRED;
}

/* Get subscription for account */
int subscription_get_by_account(int account_id, subscription_t *sub) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT id, account_id, plan, status, valid_from, valid_until, "
                      "grace_until, provider, external_id, notes, created_at, updated_at "
                      "FROM subscriptions WHERE account_id = ?";

    if (db_prepare(sql, &stmt) != 0) {
        LOG_ERROR("subscription", "Failed to prepare subscription query");
        return -1;
    }

    db_bind_int(stmt, 1, account_id);

    int result = sqlite3_step(stmt);
    if (result != SQLITE_ROW) {
        db_finalize(stmt);
        LOG_WARN("subscription", "No subscription found for account %d", account_id);
        return -1;
    }

    sub->id = db_column_int(stmt, 0);
    sub->account_id = db_column_int(stmt, 1);
    sub->plan = subscription_string_to_plan(db_column_text(stmt, 2));
    sub->status = subscription_string_to_status(db_column_text(stmt, 3));
    sub->valid_from = db_column_int64(stmt, 4);
    sub->valid_until = db_column_int64(stmt, 5);
    sub->grace_until = db_column_int64(stmt, 6);
    safe_strncpy(sub->provider, db_column_text(stmt, 7), sizeof(sub->provider));
    safe_strncpy(sub->external_id, db_column_text(stmt, 8), sizeof(sub->external_id));
    safe_strncpy(sub->notes, db_column_text(stmt, 9), sizeof(sub->notes));
    sub->created_at = db_column_int64(stmt, 10);
    sub->updated_at = db_column_int64(stmt, 11);

    db_finalize(stmt);
    return 0;
}

/* Check if subscription is currently valid (including grace period) */
int subscription_is_valid(const subscription_t *sub) {
    time_t now = time(NULL);

    /* Check status */
    if (sub->status == STATUS_CANCELLED || sub->status == STATUS_EXPIRED) {
        /* Check grace period */
        if (sub->grace_until > 0 && now <= sub->grace_until) {
            return 1; /* In grace period */
        }
        return 0;
    }

    /* Check validity period */
    if (now < sub->valid_from || now > sub->valid_until) {
        return 0;
    }

    return 1;
}

/* Update subscription (logs event to billing_events) */
int subscription_update(int account_id, plan_t new_plan,
                       subscription_status_t new_status,
                       time_t valid_until, int admin_user_id,
                       const char *notes) {
    /* Get current subscription */
    subscription_t current_sub;
    int has_current = (subscription_get_by_account(account_id, &current_sub) == 0);

    /* Begin transaction */
    db_begin_transaction();

    /* Update or insert subscription */
    sqlite3_stmt *stmt;
    const char *sql;

    if (has_current) {
        sql = "UPDATE subscriptions SET plan = ?, status = ?, valid_until = ?, "
              "notes = ?, updated_at = ? WHERE account_id = ?";
    } else {
        sql = "INSERT INTO subscriptions (plan, status, valid_from, valid_until, "
              "grace_until, provider, external_id, notes, created_at, updated_at, account_id) "
              "VALUES (?, ?, ?, ?, 0, 'manual', '', ?, ?, ?, ?)";
    }

    if (db_prepare(sql, &stmt) != 0) {
        LOG_ERROR("subscription", "Failed to prepare subscription update");
        db_rollback_transaction();
        return -1;
    }

    time_t now = time(NULL);

    if (has_current) {
        db_bind_text(stmt, 1, subscription_plan_to_string(new_plan));
        db_bind_text(stmt, 2, subscription_status_to_string(new_status));
        db_bind_int64(stmt, 3, valid_until);
        db_bind_text(stmt, 4, notes ? notes : "");
        db_bind_int64(stmt, 5, now);
        db_bind_int(stmt, 6, account_id);
    } else {
        db_bind_text(stmt, 1, subscription_plan_to_string(new_plan));
        db_bind_text(stmt, 2, subscription_status_to_string(new_status));
        db_bind_int64(stmt, 3, now);
        db_bind_int64(stmt, 4, valid_until);
        db_bind_text(stmt, 5, notes ? notes : "");
        db_bind_int64(stmt, 6, now);
        db_bind_int64(stmt, 7, now);
        db_bind_int(stmt, 8, account_id);
    }

    int result = sqlite3_step(stmt);
    db_finalize(stmt);

    if (result != SQLITE_DONE) {
        LOG_ERROR("subscription", "Failed to update subscription: %s", db_error_message());
        db_rollback_transaction();
        return -1;
    }

    /* Log billing event */
    const char *event_sql = "INSERT INTO billing_events (account_id, event_type, "
                           "previous_plan, new_plan, previous_status, new_status, "
                           "admin_user_id, notes, occurred_at) "
                           "VALUES (?, 'subscription_update', ?, ?, ?, ?, ?, ?, ?)";

    if (db_prepare(event_sql, &stmt) != 0) {
        LOG_ERROR("subscription", "Failed to prepare billing event");
        db_rollback_transaction();
        return -1;
    }

    db_bind_int(stmt, 1, account_id);
    if (has_current) {
        db_bind_text(stmt, 2, subscription_plan_to_string(current_sub.plan));
        db_bind_text(stmt, 4, subscription_status_to_string(current_sub.status));
    } else {
        db_bind_text(stmt, 2, "none");
        db_bind_text(stmt, 4, "none");
    }
    db_bind_text(stmt, 3, subscription_plan_to_string(new_plan));
    db_bind_text(stmt, 5, subscription_status_to_string(new_status));
    db_bind_int(stmt, 6, admin_user_id);
    db_bind_text(stmt, 7, notes ? notes : "");
    db_bind_int64(stmt, 8, now);

    result = sqlite3_step(stmt);
    db_finalize(stmt);

    if (result != SQLITE_DONE) {
        LOG_ERROR("subscription", "Failed to log billing event");
        db_rollback_transaction();
        return -1;
    }

    db_commit_transaction();

    LOG_INFO("subscription", "Updated subscription for account %d: %s -> %s",
             account_id,
             has_current ? subscription_plan_to_string(current_sub.plan) : "none",
             subscription_plan_to_string(new_plan));

    return 0;
}

/* Create initial subscription for new account */
int subscription_create(int account_id, plan_t plan) {
    time_t now = time(NULL);
    time_t valid_until = now + (365 * 24 * 60 * 60); /* 1 year from now */

    return subscription_update(account_id, plan, STATUS_ACTIVE, valid_until, 0,
                              "Initial subscription");
}
