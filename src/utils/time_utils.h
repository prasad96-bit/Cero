#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <time.h>

/* Get current Unix timestamp (UTC) */
time_t get_current_timestamp(void);

/* Format timestamp as ISO 8601 string (UTC) */
/* Returns formatted string in provided buffer */
void format_timestamp_iso8601(time_t timestamp, char *buffer, size_t buffer_size);

/* Format timestamp as HTTP date string (RFC 7231) */
void format_timestamp_http(time_t timestamp, char *buffer, size_t buffer_size);

/* Parse ISO 8601 date string to timestamp */
time_t parse_iso8601(const char *date_str);

/* Add days to timestamp */
time_t add_days(time_t timestamp, int days);

/* Add seconds to timestamp */
time_t add_seconds(time_t timestamp, int seconds);

/* Get start of day (00:00:00 UTC) for given timestamp */
time_t start_of_day(time_t timestamp);

/* Get end of day (23:59:59 UTC) for given timestamp */
time_t end_of_day(time_t timestamp);

#endif /* TIME_UTILS_H */
