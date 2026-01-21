/*
 * HTTP routing system
 * Routes HTTP requests to appropriate handlers
 */

#include "router.h"
#include "../utils/log.h"
#include "../auth/auth.h"
#include "../billing/admin.h"
#include "../reports/reports.h"
#include <string.h>
#include <stdlib.h>

#define MAX_ROUTES 100

static route_t routes[MAX_ROUTES];
static int route_count = 0;

/* Forward declarations for route handlers */
http_response_t *handle_home_page(http_request_t *req);
http_response_t *handle_dashboard(http_request_t *req);
http_response_t *handle_billing_page(http_request_t *req);

/* Initialize routing system */
void router_init(void) {
    route_count = 0;
    LOG_INFO("router", "Router initialized");
}

/* Register a route */
void router_add_route(http_method_t method, const char *path, route_handler_t handler,
                     int requires_auth, int requires_admin) {
    if (route_count >= MAX_ROUTES) {
        LOG_ERROR("router", "Too many routes");
        return;
    }

    routes[route_count].method = method;
    routes[route_count].path = path;
    routes[route_count].handler = handler;
    routes[route_count].requires_auth = requires_auth;
    routes[route_count].requires_admin = requires_admin;
    route_count++;

    LOG_DEBUG("router", "Added route: %s %s (auth=%d, admin=%d)",
              method == HTTP_GET ? "GET" : "POST", path,
              requires_auth, requires_admin);
}

/* Find and execute route handler */
http_response_t *router_handle_request(http_request_t *req) {
    LOG_DEBUG("router", "Handling request: %s %s",
              request_method_string(req->method), req->path);

    /* Find matching route */
    for (int i = 0; i < route_count; i++) {
        if (routes[i].method == req->method &&
            strcmp(routes[i].path, req->path) == 0) {

            LOG_DEBUG("router", "Route matched: %s", routes[i].path);

            /* Check authentication */
            if (routes[i].requires_auth && !req->is_authenticated) {
                LOG_INFO("router", "Authentication required for %s", req->path);
                http_response_t *resp = response_new();
                response_redirect(resp, "/login", 0);
                return resp;
            }

            /* Check admin access */
            if (routes[i].requires_admin &&
                strcmp(req->user_role, "admin") != 0) {
                LOG_WARN("router", "Admin access denied for user %s", req->user_email);
                http_response_t *resp = response_new();
                response_set_status(resp, 403);
                response_set_content_type(resp, "text/html");
                response_set_body(resp, "<h1>403 Forbidden</h1><p>Admin access required</p>");
                return resp;
            }

            /* Call handler */
            return routes[i].handler(req);
        }
    }

    /* No route found - 404 */
    LOG_INFO("router", "No route found for %s %s",
             request_method_string(req->method), req->path);

    http_response_t *resp = response_new();
    response_set_status(resp, 404);
    response_set_content_type(resp, "text/html");
    response_set_body(resp, "<h1>404 Not Found</h1><p>The requested page does not exist.</p>");
    return resp;
}

/* Placeholder route handlers (will be implemented with templates) */

http_response_t *handle_home_page(http_request_t *req) {
    http_response_t *resp = response_new();
    response_set_content_type(resp, "text/html");

    if (req->is_authenticated) {
        char body[512];
        snprintf(body, sizeof(body),
                "<html><head><title>Home</title></head><body>"
                "<h1>Welcome, %s!</h1>"
                "<p><a href=\"/dashboard\">Dashboard</a> | "
                "<a href=\"/billing\">Billing</a> | "
                "<a href=\"/reports\">Reports</a> | "
                "<a href=\"/logout\">Logout</a></p>"
                "</body></html>",
                req->user_email);
        response_set_body(resp, body);
    } else {
        response_set_body(resp,
                "<html><head><title>Home</title></head><body>"
                "<h1>Welcome to Cero</h1>"
                "<p><a href=\"/login\">Login</a></p>"
                "</body></html>");
    }

    return resp;
}

http_response_t *handle_dashboard(http_request_t *req) {
    http_response_t *resp = response_new();
    response_set_content_type(resp, "text/html");

    char body[1024];
    snprintf(body, sizeof(body),
            "<html><head><title>Dashboard</title></head><body>"
            "<h1>Dashboard</h1>"
            "<p>Welcome, %s (Account ID: %d)</p>"
            "<p>Role: %s</p>"
            "<p><a href=\"/\">Home</a> | "
            "<a href=\"/billing\">Billing</a> | "
            "<a href=\"/reports\">Reports</a> | "
            "<a href=\"/logout\">Logout</a></p>"
            "</body></html>",
            req->user_email, req->account_id, req->user_role);

    response_set_body(resp, body);
    return resp;
}

http_response_t *handle_billing_page(http_request_t *req) {
    http_response_t *resp = response_new();
    response_set_content_type(resp, "text/html");

    char body[1024];
    snprintf(body, sizeof(body),
            "<html><head><title>Billing</title></head><body>"
            "<h1>Billing</h1>"
            "<p>Account ID: %d</p>"
            "<p>Email: %s</p>"
            "<p>Subscription information will be displayed here.</p>"
            "<p><a href=\"/\">Home</a> | "
            "<a href=\"/dashboard\">Dashboard</a> | "
            "<a href=\"/logout\">Logout</a></p>"
            "</body></html>",
            req->account_id, req->user_email);

    response_set_body(resp, body);
    return resp;
}

/* Register all application routes */
void routes_register_all(void) {
    LOG_INFO("router", "Registering all routes");

    /* Public routes */
    router_add_route(HTTP_GET, "/", handle_home_page, 0, 0);
    router_add_route(HTTP_GET, "/login", handle_login_page, 0, 0);
    router_add_route(HTTP_POST, "/login", handle_login_submit, 0, 0);
    router_add_route(HTTP_GET, "/logout", handle_logout, 0, 0);

    /* Authenticated routes */
    router_add_route(HTTP_GET, "/dashboard", handle_dashboard, 1, 0);
    router_add_route(HTTP_GET, "/billing", handle_billing_page, 1, 0);
    router_add_route(HTTP_GET, "/reports", handle_reports_page, 1, 0);
    router_add_route(HTTP_POST, "/reports/generate", handle_reports_generate, 1, 0);
    router_add_route(HTTP_GET, "/reports/export", handle_reports_export_csv, 1, 0);

    /* Admin routes */
    router_add_route(HTTP_GET, "/admin/billing", handle_admin_billing_page, 1, 1);
    router_add_route(HTTP_POST, "/admin/billing/mark-paid", handle_admin_mark_paid, 1, 1);
    router_add_route(HTTP_POST, "/admin/search", handle_admin_search_accounts, 1, 1);

    LOG_INFO("router", "Registered %d routes", route_count);
}
