/*
 * Time and date utilities
 * All times stored and processed as UTC
 */

#include "time_utils.h"
#include <stdio.h>
#include <string.h>

/* Get current Unix timestamp (UTC) */
time_t get_current_timestamp(void) {
    return time(NULL);
}

/* Format timestamp as ISO 8601 string (UTC) */
void format_timestamp_iso8601(time_t timestamp, char *buffer, size_t buffer_size) {
    struct tm *tm_info = gmtime(&timestamp);
    strftime(buffer, buffer_size, "%Y-%m-%dT%H:%M:%SZ", tm_info);
}

/* Format timestamp as HTTP date string (RFC 7231) */
void format_timestamp_http(time_t timestamp, char *buffer, size_t buffer_size) {
    struct tm *tm_info = gmtime(&timestamp);
    strftime(buffer, buffer_size, "%a, %d %b %Y %H:%M:%S GMT", tm_info);
}

/* Parse ISO 8601 date string to timestamp */
time_t parse_iso8601(const char *date_str) {
    struct tm tm_info;
    memset(&tm_info, 0, sizeof(tm_info));

    /* Parse YYYY-MM-DD format */
    if (sscanf(date_str, "%d-%d-%d",
               &tm_info.tm_year, &tm_info.tm_mon, &tm_info.tm_mday) == 3) {
        tm_info.tm_year -= 1900;  /* Years since 1900 */
        tm_info.tm_mon -= 1;      /* Months since January */
        return timegm(&tm_info);  /* Convert to UTC timestamp */
    }

    return 0;  /* Parse error */
}

/* Add days to timestamp */
time_t add_days(time_t timestamp, int days) {
    return timestamp + (days * 86400);
}

/* Add seconds to timestamp */
time_t add_seconds(time_t timestamp, int seconds) {
    return timestamp + seconds;
}

/* Get start of day (00:00:00 UTC) */
time_t start_of_day(time_t timestamp) {
    struct tm *tm_info = gmtime(&timestamp);
    tm_info->tm_hour = 0;
    tm_info->tm_min = 0;
    tm_info->tm_sec = 0;
    return timegm(tm_info);
}

/* Get end of day (23:59:59 UTC) */
time_t end_of_day(time_t timestamp) {
    struct tm *tm_info = gmtime(&timestamp);
    tm_info->tm_hour = 23;
    tm_info->tm_min = 59;
    tm_info->tm_sec = 59;
    return timegm(tm_info);
}
