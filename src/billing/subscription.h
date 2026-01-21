#ifndef SUBSCRIPTION_H
#define SUBSCRIPTION_H

#include <time.h>

/* Subscription plans */
typedef enum {
    PLAN_FREE,
    PLAN_PRO,
    PLAN_ENTERPRISE
} plan_t;

/* Subscription status */
typedef enum {
    STATUS_ACTIVE,
    STATUS_GRACE_PERIOD,
    STATUS_EXPIRED,
    STATUS_CANCELLED
} subscription_status_t;

/* Subscription structure */
typedef struct {
    int id;
    int account_id;
    plan_t plan;
    subscription_status_t status;
    time_t valid_from;
    time_t valid_until;
    time_t grace_until;  /* 0 if no grace period */
    char provider[32];
    char external_id[128];
    char notes[512];
    time_t created_at;
    time_t updated_at;
} subscription_t;

/* Get subscription for account */
int subscription_get_by_account(int account_id, subscription_t *sub);

/* Check if subscription is currently valid (including grace period) */
int subscription_is_valid(const subscription_t *sub);

/* Update subscription (logs event to billing_events) */
int subscription_update(int account_id, plan_t new_plan,
                       subscription_status_t new_status,
                       time_t valid_until, int admin_user_id,
                       const char *notes);

/* Create initial subscription for new account */
int subscription_create(int account_id, plan_t plan);

/* Convert plan enum to string */
const char *subscription_plan_to_string(plan_t plan);

/* Convert status enum to string */
const char *subscription_status_to_string(subscription_status_t status);

/* Convert string to plan enum */
plan_t subscription_string_to_plan(const char *str);

/* Convert string to status enum */
subscription_status_t subscription_string_to_status(const char *str);

#endif /* SUBSCRIPTION_H */
