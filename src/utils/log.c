/*
 * Simple file-based logging system
 * Thread-safe with mutex locking
 */

#include "log.h"
#include "time_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>

static FILE *g_log_file = NULL;
static log_level_t g_min_level = LOG_INFO;
static pthread_mutex_t g_log_mutex = PTHREAD_MUTEX_INITIALIZER;
static char g_log_path[512] = {0};

static const char *level_strings[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR"
};

/* Initialize logging */
int log_init(const char *log_file_path, log_level_t min_level) {
    pthread_mutex_lock(&g_log_mutex);

    if (g_log_file != NULL) {
        fclose(g_log_file);
    }

    strncpy(g_log_path, log_file_path, sizeof(g_log_path) - 1);
    g_min_level = min_level;

    g_log_file = fopen(log_file_path, "a");
    if (g_log_file == NULL) {
        pthread_mutex_unlock(&g_log_mutex);
        return -1;
    }

    /* Unbuffered for immediate writes */
    setvbuf(g_log_file, NULL, _IONBF, 0);

    pthread_mutex_unlock(&g_log_mutex);
    return 0;
}

/* Close logging */
void log_close(void) {
    pthread_mutex_lock(&g_log_mutex);

    if (g_log_file != NULL) {
        fclose(g_log_file);
        g_log_file = NULL;
    }

    pthread_mutex_unlock(&g_log_mutex);
}

/* Log a message */
void log_message(log_level_t level, const char *module, const char *fmt, ...) {
    if (level < g_min_level) {
        return;
    }

    pthread_mutex_lock(&g_log_mutex);

    if (g_log_file == NULL) {
        pthread_mutex_unlock(&g_log_mutex);
        return;
    }

    /* Format timestamp */
    char timestamp[64];
    time_t now = get_current_timestamp();
    format_timestamp_iso8601(now, timestamp, sizeof(timestamp));

    /* Print log header */
    fprintf(g_log_file, "[%s] [%s] [%s] ",
            timestamp, level_strings[level], module);

    /* Print formatted message */
    va_list args;
    va_start(args, fmt);
    vfprintf(g_log_file, fmt, args);
    va_end(args);

    fprintf(g_log_file, "\n");

    pthread_mutex_unlock(&g_log_mutex);
}

/* Rotate log file */
int log_rotate(void) {
    pthread_mutex_lock(&g_log_mutex);

    if (g_log_file != NULL) {
        fclose(g_log_file);
    }

    /* Rename current log file with timestamp */
    char timestamp[32];
    time_t now = get_current_timestamp();
    struct tm *tm_info = gmtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d", tm_info);

    char new_path[1024];
    snprintf(new_path, sizeof(new_path), "%s.%s", g_log_path, timestamp);

    /* Note: This will overwrite if multiple rotations happen same day */
    rename(g_log_path, new_path);

    /* Open new log file */
    g_log_file = fopen(g_log_path, "a");
    if (g_log_file == NULL) {
        pthread_mutex_unlock(&g_log_mutex);
        return -1;
    }

    setvbuf(g_log_file, NULL, _IONBF, 0);

    pthread_mutex_unlock(&g_log_mutex);
    return 0;
}
