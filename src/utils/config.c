/*
 * Configuration file parser
 * Supports simple KEY=VALUE format
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Global configuration instance */
config_t g_config;

/* Trim whitespace from string */
static void trim(char *str) {
    char *start = str;
    char *end;

    /* Trim leading */
    while (isspace((unsigned char)*start)) start++;

    if (*start == 0) {
        str[0] = '\0';
        return;
    }

    /* Trim trailing */
    end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';

    /* Move trimmed string to start */
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

/* Parse single configuration file */
static int parse_config_file(const char *filename, int is_secrets) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Failed to open config file: %s\n", filename);
        return -1;
    }

    char line[1024];
    int line_num = 0;

    while (fgets(line, sizeof(line), file)) {
        line_num++;

        /* Remove newline */
        line[strcspn(line, "\r\n")] = '\0';

        /* Trim whitespace */
        trim(line);

        /* Skip empty lines and comments */
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }

        /* Parse KEY=VALUE */
        char *equals = strchr(line, '=');
        if (equals == NULL) {
            fprintf(stderr, "Invalid config line %d: %s\n", line_num, line);
            continue;
        }

        *equals = '\0';
        char *key = line;
        char *value = equals + 1;

        trim(key);
        trim(value);

        /* Set configuration values */
        if (strcmp(key, "PORT") == 0) {
            g_config.port = atoi(value);
        } else if (strcmp(key, "HOST") == 0) {
            strncpy(g_config.host, value, sizeof(g_config.host) - 1);
        } else if (strcmp(key, "DB_PATH") == 0) {
            strncpy(g_config.db_path, value, sizeof(g_config.db_path) - 1);
        } else if (strcmp(key, "LOG_PATH") == 0) {
            strncpy(g_config.log_path, value, sizeof(g_config.log_path) - 1);
        } else if (strcmp(key, "LOG_LEVEL") == 0) {
            if (strcmp(value, "DEBUG") == 0) g_config.log_level = 0;
            else if (strcmp(value, "INFO") == 0) g_config.log_level = 1;
            else if (strcmp(value, "WARN") == 0) g_config.log_level = 2;
            else if (strcmp(value, "ERROR") == 0) g_config.log_level = 3;
        } else if (strcmp(key, "SESSION_EXPIRY_SECONDS") == 0) {
            g_config.session_expiry_seconds = atoi(value);
        } else if (strcmp(key, "RATE_LIMIT_REQUESTS_PER_MINUTE") == 0) {
            g_config.rate_limit_requests_per_minute = atoi(value);
        }
        /* Secrets */
        else if (is_secrets && strcmp(key, "SESSION_SECRET") == 0) {
            strncpy(g_config.session_secret, value, sizeof(g_config.session_secret) - 1);
        } else if (is_secrets && strcmp(key, "CSRF_SECRET") == 0) {
            strncpy(g_config.csrf_secret, value, sizeof(g_config.csrf_secret) - 1);
        } else if (is_secrets && strcmp(key, "ADMIN_PASSWORD_HASH") == 0) {
            strncpy(g_config.admin_password_hash, value, sizeof(g_config.admin_password_hash) - 1);
        }
    }

    fclose(file);
    return 0;
}

/* Load configuration from files */
int config_load(const char *config_file, const char *secrets_file) {
    /* Set defaults */
    memset(&g_config, 0, sizeof(g_config));
    strncpy(g_config.host, "0.0.0.0", sizeof(g_config.host) - 1);
    g_config.port = 8080;
    strncpy(g_config.db_path, "data/app.db", sizeof(g_config.db_path) - 1);
    strncpy(g_config.log_path, "logs/app.log", sizeof(g_config.log_path) - 1);
    g_config.log_level = 1;  /* INFO */
    g_config.session_expiry_seconds = 2592000;  /* 30 days */
    g_config.rate_limit_requests_per_minute = 60;

    /* Load main config */
    if (parse_config_file(config_file, 0) != 0) {
        return -1;
    }

    /* Load secrets */
    if (parse_config_file(secrets_file, 1) != 0) {
        fprintf(stderr, "Warning: Failed to load secrets file\n");
        /* Continue with defaults */
    }

    return 0;
}

/* Get configuration value as string */
const char *config_get_string(const char *key, const char *default_value) {
    if (strcmp(key, "HOST") == 0) return g_config.host;
    if (strcmp(key, "DB_PATH") == 0) return g_config.db_path;
    if (strcmp(key, "LOG_PATH") == 0) return g_config.log_path;
    return default_value;
}

/* Get configuration value as integer */
int config_get_int(const char *key, int default_value) {
    if (strcmp(key, "PORT") == 0) return g_config.port;
    if (strcmp(key, "LOG_LEVEL") == 0) return g_config.log_level;
    if (strcmp(key, "SESSION_EXPIRY_SECONDS") == 0) return g_config.session_expiry_seconds;
    if (strcmp(key, "RATE_LIMIT_REQUESTS_PER_MINUTE") == 0) return g_config.rate_limit_requests_per_minute;
    return default_value;
}
