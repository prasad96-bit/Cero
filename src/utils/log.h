#ifndef LOG_H
#define LOG_H

#include <stdio.h>

/* Log levels */
typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO  = 1,
    LOG_WARN  = 2,
    LOG_ERROR = 3
} log_level_t;

/* Initialize logging system */
int log_init(const char *log_file_path, log_level_t min_level);

/* Close logging system */
void log_close(void);

/* Log a message */
void log_message(log_level_t level, const char *module, const char *fmt, ...);

/* Convenience macros */
#define LOG_DEBUG(module, ...) log_message(LOG_DEBUG, module, __VA_ARGS__)
#define LOG_INFO(module, ...)  log_message(LOG_INFO,  module, __VA_ARGS__)
#define LOG_WARN(module, ...)  log_message(LOG_WARN,  module, __VA_ARGS__)
#define LOG_ERROR(module, ...) log_message(LOG_ERROR, module, __VA_ARGS__)

/* Rotate log file (call daily) */
int log_rotate(void);

#endif /* LOG_H */
