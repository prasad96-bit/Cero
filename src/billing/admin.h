#ifndef BILLING_ADMIN_H
#define BILLING_ADMIN_H

#include "../core/request.h"
#include "../core/response.h"
#include "subscription.h"

/* Billing event structure */
typedef struct {
    int id;
    int account_id;
    char event_type[64];
    char previous_plan[32];
    char new_plan[32];
    char previous_status[32];
    char new_status[32];
    int amount_cents;
    char currency[8];
    char payment_method[64];
    char external_reference[128];
    int admin_user_id;
    char notes[512];
    time_t occurred_at;
} billing_event_t;

/* Log billing event (append-only) */
int billing_log_event(int account_id, const char *event_type,
                     const char *previous_plan, const char *new_plan,
                     const char *previous_status, const char *new_status,
                     int amount_cents, const char *currency,
                     const char *payment_method, const char *external_reference,
                     int admin_user_id, const char *notes);

/* Mark subscription as paid (admin action) */
int billing_mark_as_paid(int account_id, plan_t plan, int duration_days,
                        int amount_cents, const char *payment_method,
                        const char *external_reference, int admin_user_id,
                        const char *notes);

/* Get billing events for account */
int billing_get_events_for_account(int account_id, billing_event_t **events, int *count);

/* Route handlers */
http_response_t *handle_admin_billing_page(http_request_t *req);
http_response_t *handle_admin_mark_paid(http_request_t *req);
http_response_t *handle_admin_search_accounts(http_request_t *req);

#endif /* BILLING_ADMIN_H */
