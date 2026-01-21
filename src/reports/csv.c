/*
 * CSV export utility
 * Generates CSV formatted data
 */

#include "csv.h"
#include "../utils/log.h"
#include <stdlib.h>
#include <string.h>

#define INITIAL_BUFFER_SIZE 4096

/* Create new CSV writer */
csv_writer_t *csv_writer_new(void) {
    csv_writer_t *writer = calloc(1, sizeof(csv_writer_t));
    if (!writer) {
        LOG_ERROR("csv", "Failed to allocate CSV writer");
        return NULL;
    }

    writer->buffer_capacity = INITIAL_BUFFER_SIZE;
    writer->buffer = malloc(writer->buffer_capacity);
    if (!writer->buffer) {
        LOG_ERROR("csv", "Failed to allocate CSV buffer");
        free(writer);
        return NULL;
    }

    writer->buffer[0] = '\0';
    writer->buffer_size = 0;

    return writer;
}

/* Escape CSV field (handles quotes, commas, newlines) */
char *csv_escape_field(const char *field) {
    if (!field) {
        return strdup("");
    }

    /* Check if field needs escaping */
    int needs_escape = 0;
    for (const char *p = field; *p; p++) {
        if (*p == ',' || *p == '"' || *p == '\n' || *p == '\r') {
            needs_escape = 1;
            break;
        }
    }

    if (!needs_escape) {
        return strdup(field);
    }

    /* Calculate escaped length */
    size_t len = strlen(field);
    size_t escaped_len = len + 2; /* Opening and closing quotes */

    /* Count quotes that need doubling */
    for (const char *p = field; *p; p++) {
        if (*p == '"') {
            escaped_len++;
        }
    }

    /* Build escaped field */
    char *escaped = malloc(escaped_len + 1);
    if (!escaped) {
        LOG_ERROR("csv", "Failed to allocate escaped field");
        return strdup(field);
    }

    char *out = escaped;
    *out++ = '"'; /* Opening quote */

    for (const char *p = field; *p; p++) {
        if (*p == '"') {
            *out++ = '"'; /* Double the quote */
            *out++ = '"';
        } else {
            *out++ = *p;
        }
    }

    *out++ = '"'; /* Closing quote */
    *out = '\0';

    return escaped;
}

/* Append string to CSV buffer */
static void csv_append(csv_writer_t *writer, const char *str) {
    size_t str_len = strlen(str);
    size_t needed = writer->buffer_size + str_len + 1;

    /* Expand buffer if needed */
    if (needed > writer->buffer_capacity) {
        writer->buffer_capacity = needed * 2;
        char *new_buffer = realloc(writer->buffer, writer->buffer_capacity);
        if (!new_buffer) {
            LOG_ERROR("csv", "Failed to expand CSV buffer");
            return;
        }
        writer->buffer = new_buffer;
    }

    /* Append string */
    strcat(writer->buffer, str);
    writer->buffer_size += str_len;
}

/* Add CSV header row */
void csv_add_header(csv_writer_t *writer, const char **columns, int column_count) {
    for (int i = 0; i < column_count; i++) {
        char *escaped = csv_escape_field(columns[i]);
        csv_append(writer, escaped);
        free(escaped);

        if (i < column_count - 1) {
            csv_append(writer, ",");
        }
    }
    csv_append(writer, "\n");
}

/* Add CSV data row */
void csv_add_row(csv_writer_t *writer, const char **values, int value_count) {
    for (int i = 0; i < value_count; i++) {
        char *escaped = csv_escape_field(values[i]);
        csv_append(writer, escaped);
        free(escaped);

        if (i < value_count - 1) {
            csv_append(writer, ",");
        }
    }
    csv_append(writer, "\n");
}

/* Get CSV content */
const char *csv_get_content(csv_writer_t *writer) {
    return writer->buffer;
}

/* Free CSV writer */
void csv_writer_free(csv_writer_t *writer) {
    if (!writer) return;

    free(writer->buffer);
    free(writer);
}
