#ifndef SERVER_H
#define SERVER_H

/* Server configuration */
typedef struct {
    const char *host;
    int port;
    int backlog;  /* Listen backlog */
} server_config_t;

/* Initialize and start HTTP server */
int server_start(const char *host, int port);

/* Stop HTTP server */
void server_stop(void);

/* Handle single client connection */
void server_handle_client(int client_socket);

#endif /* SERVER_H */
