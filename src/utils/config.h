#ifndef CONFIG_H
#define CONFIG_H

/* Application configuration structure */
typedef struct {
    /* Server settings */
    char host[256];
    int port;

    /* Database settings */
    char db_path[512];

    /* Logging settings */
    char log_path[512];
    int log_level;  /* 0=DEBUG, 1=INFO, 2=WARN, 3=ERROR */

    /* Session settings */
    int session_expiry_seconds;

    /* Rate limiting */
    int rate_limit_requests_per_minute;

    /* Secrets */
    char session_secret[128];
    char csrf_secret[128];
    char admin_password_hash[128];
} config_t;

/* Global configuration instance */
extern config_t g_config;

/* Load configuration from files */
int config_load(const char *config_file, const char *secrets_file);

/* Get configuration value as string */
const char *config_get_string(const char *key, const char *default_value);

/* Get configuration value as integer */
int config_get_int(const char *key, int default_value);

#endif /* CONFIG_H */
