/*
 * Admin billing operations
 * Manual billing workflows and administrative functions
 */

#include "admin.h"
#include "subscription.h"
#include "../utils/db.h"
#include "../utils/log.h"
#include "../utils/string_utils.h"
#include "../templates/template.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Log billing event (append-only) */
int billing_log_event(int account_id, const char *event_type,
                     const char *previous_plan, const char *new_plan,
                     const char *previous_status, const char *new_status,
                     int amount_cents, const char *currency,
                     const char *payment_method, const char *external_reference,
                     int admin_user_id, const char *notes) {
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO billing_events (account_id, event_type, "
                      "previous_plan, new_plan, previous_status, new_status, "
                      "amount_cents, currency, payment_method, external_reference, "
                      "admin_user_id, notes, occurred_at) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    if (db_prepare(sql, &stmt) != 0) {
        LOG_ERROR("billing", "Failed to prepare billing event insert");
        return -1;
    }

    time_t now = time(NULL);

    db_bind_int(stmt, 1, account_id);
    db_bind_text(stmt, 2, event_type);
    db_bind_text(stmt, 3, previous_plan ? previous_plan : "");
    db_bind_text(stmt, 4, new_plan ? new_plan : "");
    db_bind_text(stmt, 5, previous_status ? previous_status : "");
    db_bind_text(stmt, 6, new_status ? new_status : "");
    db_bind_int(stmt, 7, amount_cents);
    db_bind_text(stmt, 8, currency ? currency : "USD");
    db_bind_text(stmt, 9, payment_method ? payment_method : "");
    db_bind_text(stmt, 10, external_reference ? external_reference : "");
    db_bind_int(stmt, 11, admin_user_id);
    db_bind_text(stmt, 12, notes ? notes : "");
    db_bind_int64(stmt, 13, now);

    int result = sqlite3_step(stmt);
    db_finalize(stmt);

    if (result != SQLITE_DONE) {
        LOG_ERROR("billing", "Failed to log billing event: %s", db_error_message());
        return -1;
    }

    LOG_INFO("billing", "Logged event: %s for account %d", event_type, account_id);
    return 0;
}

/* Mark subscription as paid (admin action) */
int billing_mark_as_paid(int account_id, plan_t plan, int duration_days,
                        int amount_cents, const char *payment_method,
                        const char *external_reference, int admin_user_id,
                        const char *notes) {
    /* Calculate new expiration date */
    time_t now = time(NULL);
    time_t valid_until = now + (duration_days * 24 * 60 * 60);

    /* Update subscription */
    int result = subscription_update(account_id, plan, STATUS_ACTIVE,
                                    valid_until, admin_user_id, notes);

    if (result != 0) {
        LOG_ERROR("billing", "Failed to update subscription for payment");
        return -1;
    }

    /* Log payment event */
    billing_log_event(account_id, "payment_received",
                     NULL, subscription_plan_to_string(plan),
                     NULL, subscription_status_to_string(STATUS_ACTIVE),
                     amount_cents, "USD",
                     payment_method, external_reference,
                     admin_user_id, notes);

    LOG_INFO("billing", "Marked account %d as paid: %s for %d days ($%.2f)",
             account_id, subscription_plan_to_string(plan),
             duration_days, amount_cents / 100.0);

    return 0;
}

/* Get billing events for account */
int billing_get_events_for_account(int account_id, billing_event_t **events, int *count) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT id, account_id, event_type, previous_plan, new_plan, "
                      "previous_status, new_status, amount_cents, currency, "
                      "payment_method, external_reference, admin_user_id, notes, occurred_at "
                      "FROM billing_events WHERE account_id = ? ORDER BY occurred_at DESC";

    if (db_prepare(sql, &stmt) != 0) {
        LOG_ERROR("billing", "Failed to prepare events query");
        return -1;
    }

    db_bind_int(stmt, 1, account_id);

    /* Count results first */
    int event_count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        event_count++;
    }

    if (event_count == 0) {
        db_finalize(stmt);
        *events = NULL;
        *count = 0;
        return 0;
    }

    /* Reset and allocate */
    sqlite3_reset(stmt);
    billing_event_t *result_events = calloc(event_count, sizeof(billing_event_t));
    if (!result_events) {
        db_finalize(stmt);
        return -1;
    }

    /* Fetch events */
    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < event_count) {
        result_events[i].id = db_column_int(stmt, 0);
        result_events[i].account_id = db_column_int(stmt, 1);
        safe_strncpy(result_events[i].event_type, db_column_text(stmt, 2),
                    sizeof(result_events[i].event_type));
        safe_strncpy(result_events[i].previous_plan, db_column_text(stmt, 3),
                    sizeof(result_events[i].previous_plan));
        safe_strncpy(result_events[i].new_plan, db_column_text(stmt, 4),
                    sizeof(result_events[i].new_plan));
        safe_strncpy(result_events[i].previous_status, db_column_text(stmt, 5),
                    sizeof(result_events[i].previous_status));
        safe_strncpy(result_events[i].new_status, db_column_text(stmt, 6),
                    sizeof(result_events[i].new_status));
        result_events[i].amount_cents = db_column_int(stmt, 7);
        safe_strncpy(result_events[i].currency, db_column_text(stmt, 8),
                    sizeof(result_events[i].currency));
        safe_strncpy(result_events[i].payment_method, db_column_text(stmt, 9),
                    sizeof(result_events[i].payment_method));
        safe_strncpy(result_events[i].external_reference, db_column_text(stmt, 10),
                    sizeof(result_events[i].external_reference));
        result_events[i].admin_user_id = db_column_int(stmt, 11);
        safe_strncpy(result_events[i].notes, db_column_text(stmt, 12),
                    sizeof(result_events[i].notes));
        result_events[i].occurred_at = db_column_int64(stmt, 13);
        i++;
    }

    db_finalize(stmt);

    *events = result_events;
    *count = i;

    return 0;
}

/* Route handler: Admin billing page */
http_response_t *handle_admin_billing_page(http_request_t *req) {
    http_response_t *resp = response_new();
    response_set_content_type(resp, "text/html");

    /* Simple admin billing interface */
    char body[4096];
    snprintf(body, sizeof(body),
            "<html><head><title>Admin Billing</title></head><body>"
            "<h1>Admin Billing</h1>"
            "<p>Logged in as: %s (Admin)</p>"
            "<h2>Mark Account as Paid</h2>"
            "<form method=\"POST\" action=\"/admin/billing/mark-paid\">"
            "<p><label>Account ID: <input type=\"number\" name=\"account_id\" required></label></p>"
            "<p><label>Plan: <select name=\"plan\" required>"
            "<option value=\"free\">Free</option>"
            "<option value=\"pro\">Pro</option>"
            "<option value=\"enterprise\">Enterprise</option>"
            "</select></label></p>"
            "<p><label>Duration (days): <input type=\"number\" name=\"duration\" value=\"30\" required></label></p>"
            "<p><label>Amount ($): <input type=\"number\" step=\"0.01\" name=\"amount\" required></label></p>"
            "<p><label>Payment Method: <input type=\"text\" name=\"payment_method\" value=\"manual\"></label></p>"
            "<p><label>Reference: <input type=\"text\" name=\"reference\"></label></p>"
            "<p><label>Notes: <textarea name=\"notes\"></textarea></label></p>"
            "<p><button type=\"submit\">Mark as Paid</button></p>"
            "</form>"
            "<p><a href=\"/\">Home</a> | <a href=\"/dashboard\">Dashboard</a></p>"
            "</body></html>",
            req->user_email);

    response_set_body(resp, body);
    return resp;
}

/* Route handler: Mark account as paid */
http_response_t *handle_admin_mark_paid(http_request_t *req) {
    http_response_t *resp = response_new();

    /* Get POST parameters */
    char *account_id_str = request_get_post_param(req, "account_id");
    char *plan_str = request_get_post_param(req, "plan");
    char *duration_str = request_get_post_param(req, "duration");
    char *amount_str = request_get_post_param(req, "amount");
    char *payment_method = request_get_post_param(req, "payment_method");
    char *reference = request_get_post_param(req, "reference");
    char *notes = request_get_post_param(req, "notes");

    if (!account_id_str || !plan_str || !duration_str || !amount_str) {
        response_set_status(resp, 400);
        response_set_content_type(resp, "text/html");
        response_set_body(resp, "<h1>Bad Request</h1><p>Missing required fields</p>");
        goto cleanup;
    }

    int account_id = atoi(account_id_str);
    int duration = atoi(duration_str);
    float amount_dollars = atof(amount_str);
    int amount_cents = (int)(amount_dollars * 100);

    plan_t plan = subscription_string_to_plan(plan_str);

    /* Mark as paid */
    int result = billing_mark_as_paid(account_id, plan, duration, amount_cents,
                                     payment_method ? payment_method : "manual",
                                     reference ? reference : "",
                                     req->user_id,
                                     notes ? notes : "");

    if (result != 0) {
        response_set_status(resp, 500);
        response_set_content_type(resp, "text/html");
        response_set_body(resp, "<h1>Error</h1><p>Failed to process payment</p>");
    } else {
        response_set_content_type(resp, "text/html");
        char success_body[1024];
        snprintf(success_body, sizeof(success_body),
                "<html><head><title>Success</title></head><body>"
                "<h1>Payment Processed</h1>"
                "<p>Account %d marked as paid: %s for %d days ($%.2f)</p>"
                "<p><a href=\"/admin/billing\">Back to Admin Billing</a></p>"
                "</body></html>",
                account_id, plan_str, duration, amount_dollars);
        response_set_body(resp, success_body);
    }

cleanup:
    free(account_id_str);
    free(plan_str);
    free(duration_str);
    free(amount_str);
    free(payment_method);
    free(reference);
    free(notes);

    return resp;
}

/* Route handler: Search accounts (admin) */
http_response_t *handle_admin_search_accounts(http_request_t *req) {
    (void)req; /* Unused for now */

    http_response_t *resp = response_new();
    response_set_content_type(resp, "text/html");

    /* Simple search result */
    response_set_body(resp,
        "<html><head><title>Search Accounts</title></head><body>"
        "<h1>Search Accounts</h1>"
        "<p>Search functionality would go here</p>"
        "<p><a href=\"/admin/billing\">Back to Admin Billing</a></p>"
        "</body></html>");

    return resp;
}
