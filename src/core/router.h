#ifndef ROUTER_H
#define ROUTER_H

#include "request.h"
#include "response.h"

/* Route handler function type */
typedef http_response_t *(*route_handler_t)(http_request_t *req);

/* Route definition */
typedef struct {
    http_method_t method;
    const char *path;
    route_handler_t handler;
    int requires_auth;
    int requires_admin;
} route_t;

/* Initialize routing system */
void router_init(void);

/* Register a route */
void router_add_route(http_method_t method, const char *path, route_handler_t handler,
                     int requires_auth, int requires_admin);

/* Find and execute route handler */
http_response_t *router_handle_request(http_request_t *req);

/* Register all application routes */
void routes_register_all(void);

#endif /* ROUTER_H */
