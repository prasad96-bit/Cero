/*
 * Core SaaS Platform - Main Entry Point
 * Designed for 50+ year longevity with minimal maintenance
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "utils/config.h"
#include "utils/log.h"
#include "utils/db.h"
#include "core/server.h"
#include "core/router.h"
#include "auth/session.h"

/* Global shutdown flag */
static volatile int g_shutdown = 0;

/* Signal handler for graceful shutdown */
static void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        LOG_INFO("main", "Received shutdown signal");
        g_shutdown = 1;
        server_stop();
    }
}

/* Setup signal handlers */
static void setup_signals(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    /* Ignore SIGPIPE (broken pipe when client disconnects) */
    signal(SIGPIPE, SIG_IGN);
}

/* Initialize all subsystems */
static int initialize_app(const char *config_file, const char *secrets_file,
                         const char *schema_file) {
    /* Load configuration */
    if (config_load(config_file, secrets_file) != 0) {
        fprintf(stderr, "Failed to load configuration\n");
        return -1;
    }

    /* Initialize logging */
    if (log_init(g_config.log_path, g_config.log_level) != 0) {
        fprintf(stderr, "Failed to initialize logging\n");
        return -1;
    }

    LOG_INFO("main", "Starting Core SaaS Platform");
    LOG_INFO("main", "Configuration loaded from %s", config_file);

    /* Initialize database */
    if (db_init(g_config.db_path, schema_file) != 0) {
        LOG_ERROR("main", "Failed to initialize database");
        return -1;
    }
    LOG_INFO("main", "Database initialized: %s", g_config.db_path);

    /* Initialize routing system */
    router_init();
    routes_register_all();
    LOG_INFO("main", "Routes registered");

    /* Clean up expired sessions on startup */
    int expired = session_cleanup_expired();
    LOG_INFO("main", "Cleaned up %d expired sessions", expired);

    return 0;
}

/* Cleanup all subsystems */
static void cleanup_app(void) {
    LOG_INFO("main", "Shutting down");

    db_close();
    log_close();
}

int main(int argc, char *argv[]) {
    const char *config_file = "config/config.txt";
    const char *secrets_file = "config/secrets.txt";
    const char *schema_file = "config/schema.sql";

    /* Parse command line arguments */
    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            printf("Core SaaS Platform\n");
            printf("Usage: %s [config_file] [secrets_file] [schema_file]\n", argv[0]);
            printf("\n");
            printf("Default configuration:\n");
            printf("  config_file:  config/config.txt\n");
            printf("  secrets_file: config/secrets.txt\n");
            printf("  schema_file:  config/schema.sql\n");
            return 0;
        }
        config_file = argv[1];
    }
    if (argc > 2) {
        secrets_file = argv[2];
    }
    if (argc > 3) {
        schema_file = argv[3];
    }

    /* Setup signal handlers */
    setup_signals();

    /* Initialize application */
    if (initialize_app(config_file, secrets_file, schema_file) != 0) {
        fprintf(stderr, "Failed to initialize application\n");
        cleanup_app();
        return 1;
    }

    /* Start HTTP server (blocks until shutdown) */
    LOG_INFO("main", "Starting HTTP server on %s:%d", g_config.host, g_config.port);
    if (server_start(g_config.host, g_config.port) != 0) {
        LOG_ERROR("main", "Failed to start server");
        cleanup_app();
        return 1;
    }

    /* Cleanup and exit */
    cleanup_app();
    LOG_INFO("main", "Shutdown complete");

    return 0;
}
