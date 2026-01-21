#ifndef CSV_H
#define CSV_H

#include <stdio.h>

/* CSV writer structure */
typedef struct {
    char *buffer;
    size_t buffer_size;
    size_t buffer_capacity;
} csv_writer_t;

/* Create new CSV writer */
csv_writer_t *csv_writer_new(void);

/* Add CSV header row */
void csv_add_header(csv_writer_t *writer, const char **columns, int column_count);

/* Add CSV data row */
void csv_add_row(csv_writer_t *writer, const char **values, int value_count);

/* Escape CSV field (handles quotes, commas, newlines) */
char *csv_escape_field(const char *field);

/* Get CSV content */
const char *csv_get_content(csv_writer_t *writer);

/* Free CSV writer */
void csv_writer_free(csv_writer_t *writer);

#endif /* CSV_H */
