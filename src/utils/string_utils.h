#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <stddef.h>

/* Safe string copy - always null terminates */
void safe_strncpy(char *dest, const char *src, size_t dest_size);

/* URL decode in place */
void url_decode(char *str);

/* URL encode - caller must free result */
char *url_encode(const char *str);

/* HTML entity escape - caller must free result */
char *html_escape(const char *str);

/* Case-insensitive string comparison */
int strcasecmp_portable(const char *s1, const char *s2);

/* Trim whitespace from both ends - modifies string in place */
void str_trim(char *str);

/* Split string by delimiter - returns number of parts */
int str_split(char *str, char delimiter, char **parts, int max_parts);

/* Check if string starts with prefix */
int str_starts_with(const char *str, const char *prefix);

/* Check if string ends with suffix */
int str_ends_with(const char *str, const char *suffix);

/* Generate random hex string - caller must free result */
char *generate_random_hex(size_t length);

#endif /* STRING_UTILS_H */
