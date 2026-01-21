#ifndef ENTITLEMENT_H
#define ENTITLEMENT_H

#include "subscription.h"

/* Feature identifiers */
typedef enum {
    FEATURE_BASIC_REPORTS,
    FEATURE_ADVANCED_REPORTS,
    FEATURE_EXTENDED_DATE_RANGE,
    FEATURE_CSV_EXPORT,
    FEATURE_REPORT_GROUPING,
    FEATURE_API_ACCESS,
    FEATURE_PRIORITY_SUPPORT
} feature_t;

/* Check if account has access to feature */
int entitlement_has_feature(int account_id, feature_t feature);

/* Get maximum date range (in days) for reports */
int entitlement_get_max_report_days(int account_id);

/* Check if account can export CSV */
int entitlement_can_export_csv(int account_id);

/* Check if account can use report grouping */
int entitlement_can_use_grouping(int account_id);

/* Get feature name as string */
const char *entitlement_feature_name(feature_t feature);

#endif /* ENTITLEMENT_H */
