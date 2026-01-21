/*
 * Reports generation
 * Generates usage and activity reports
 */

#include "reports.h"
#include "csv.h"
#include "../billing/entitlement.h"
#include "../utils/db.h"
#include "../utils/log.h"
#include "../utils/string_utils.h"
#include "../utils/time_utils.h"
#include "../templates/template.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Generate report data */
int report_generate(int account_id, const report_params_t *params,
                   report_row_t **rows, int *row_count) {
    /* This is a placeholder implementation */
    /* In a real system, this would query actual usage data */

    /* Calculate number of days in range */
    time_t diff = params->end_date - params->start_date;
    int days = (int)(diff / (24 * 60 * 60));

    if (days <= 0) {
        LOG_WARN("reports", "Invalid date range");
        return -1;
    }

    /* Allocate row array */
    report_row_t *result_rows = calloc(days, sizeof(report_row_t));
    if (!result_rows) {
        LOG_ERROR("reports", "Failed to allocate report rows");
        return -1;
    }

    /* Generate placeholder data for each day */
    time_t current_date = params->start_date;
    for (int i = 0; i < days; i++) {
        format_timestamp_iso8601(current_date, result_rows[i].date, sizeof(result_rows[i].date));
        result_rows[i].user_count = 1 + (i % 5);
        result_rows[i].session_count = 5 + (i % 10);
        result_rows[i].account_count = 1;

        current_date += (24 * 60 * 60); /* Add 1 day */
    }

    *rows = result_rows;
    *row_count = days;

    LOG_INFO("reports", "Generated report with %d rows for account %d", days, account_id);
    return 0;
}

/* Validate report parameters against entitlements */
int report_validate_params(int account_id, const report_params_t *params,
                          char *error_message, size_t error_size) {
    /* Check date range */
    time_t diff = params->end_date - params->start_date;
    int days = (int)(diff / (24 * 60 * 60));

    int max_days = entitlement_get_max_report_days(account_id);
    if (days > max_days) {
        snprintf(error_message, error_size,
                "Date range exceeds maximum of %d days for your plan", max_days);
        return -1;
    }

    /* Check CSV export entitlement */
    if (params->export_csv && !entitlement_can_export_csv(account_id)) {
        snprintf(error_message, error_size,
                "CSV export not available on your plan");
        return -1;
    }

    /* Check grouping entitlement */
    if (params->grouping != GROUP_NONE && !entitlement_can_use_grouping(account_id)) {
        snprintf(error_message, error_size,
                "Report grouping not available on your plan");
        return -1;
    }

    return 0;
}

/* Route handler: Reports page */
http_response_t *handle_reports_page(http_request_t *req) {
    http_response_t *resp = response_new();
    response_set_content_type(resp, "text/html");

    int max_days = entitlement_get_max_report_days(req->account_id);
    int can_export = entitlement_can_export_csv(req->account_id);
    int can_group = entitlement_can_use_grouping(req->account_id);

    char body[4096];
    snprintf(body, sizeof(body),
            "<html><head><title>Reports</title></head><body>"
            "<h1>Reports</h1>"
            "<p>Account: %s (ID: %d)</p>"
            "<p>Maximum report range: %d days</p>"
            "<p>CSV Export: %s</p>"
            "<p>Grouping: %s</p>"
            "<h2>Generate Report</h2>"
            "<form method=\"POST\" action=\"/reports/generate\">"
            "<p><label>Start Date: <input type=\"date\" name=\"start_date\" required></label></p>"
            "<p><label>End Date: <input type=\"date\" name=\"end_date\" required></label></p>"
            "%s"
            "%s"
            "<p><button type=\"submit\">Generate Report</button></p>"
            "</form>"
            "<p><a href=\"/\">Home</a> | <a href=\"/dashboard\">Dashboard</a></p>"
            "</body></html>",
            req->user_email, req->account_id, max_days,
            can_export ? "Enabled" : "Disabled",
            can_group ? "Enabled" : "Disabled",
            can_export ? "<p><label><input type=\"checkbox\" name=\"export_csv\" value=\"1\"> Export as CSV</label></p>" : "",
            can_group ? "<p><label>Grouping: <select name=\"grouping\">"
                       "<option value=\"none\">None</option>"
                       "<option value=\"day\">By Day</option>"
                       "<option value=\"week\">By Week</option>"
                       "<option value=\"month\">By Month</option>"
                       "</select></label></p>" : "");

    response_set_body(resp, body);
    return resp;
}

/* Route handler: Generate report */
http_response_t *handle_reports_generate(http_request_t *req) {
    http_response_t *resp = response_new();

    /* Get POST parameters */
    char *start_date_str = request_get_post_param(req, "start_date");
    char *end_date_str = request_get_post_param(req, "end_date");
    char *export_csv_str = request_get_post_param(req, "export_csv");
    char *grouping_str = request_get_post_param(req, "grouping");

    if (!start_date_str || !end_date_str) {
        response_set_status(resp, 400);
        response_set_content_type(resp, "text/html");
        response_set_body(resp, "<h1>Bad Request</h1><p>Missing date parameters</p>");
        goto cleanup;
    }

    /* Parse dates */
    report_params_t params;
    params.start_date = parse_iso8601(start_date_str);
    params.end_date = parse_iso8601(end_date_str);
    params.export_csv = (export_csv_str && strcmp(export_csv_str, "1") == 0);

    if (grouping_str) {
        if (strcmp(grouping_str, "day") == 0) params.grouping = GROUP_BY_DAY;
        else if (strcmp(grouping_str, "week") == 0) params.grouping = GROUP_BY_WEEK;
        else if (strcmp(grouping_str, "month") == 0) params.grouping = GROUP_BY_MONTH;
        else params.grouping = GROUP_NONE;
    } else {
        params.grouping = GROUP_NONE;
    }

    /* Validate parameters */
    char error_msg[256];
    if (report_validate_params(req->account_id, &params, error_msg, sizeof(error_msg)) != 0) {
        response_set_status(resp, 403);
        response_set_content_type(resp, "text/html");
        char error_body[512];
        snprintf(error_body, sizeof(error_body),
                "<h1>Access Denied</h1><p>%s</p><p><a href=\"/reports\">Back</a></p>",
                error_msg);
        response_set_body(resp, error_body);
        goto cleanup;
    }

    /* Generate report */
    report_row_t *rows;
    int row_count;

    if (report_generate(req->account_id, &params, &rows, &row_count) != 0) {
        response_set_status(resp, 500);
        response_set_content_type(resp, "text/html");
        response_set_body(resp, "<h1>Error</h1><p>Failed to generate report</p>");
        goto cleanup;
    }

    /* Build HTML table */
    response_set_content_type(resp, "text/html");
    response_set_body(resp, "<html><head><title>Report Results</title></head><body>");
    response_append_body(resp, "<h1>Report Results</h1>");

    char table_header[512];
    snprintf(table_header, sizeof(table_header),
            "<table border=\"1\"><tr><th>Date</th><th>Users</th><th>Sessions</th><th>Accounts</th></tr>");
    response_append_body(resp, table_header);

    for (int i = 0; i < row_count; i++) {
        char row_html[256];
        snprintf(row_html, sizeof(row_html),
                "<tr><td>%s</td><td>%d</td><td>%d</td><td>%d</td></tr>",
                rows[i].date, rows[i].user_count, rows[i].session_count, rows[i].account_count);
        response_append_body(resp, row_html);
    }

    response_append_body(resp, "</table>");
    response_append_body(resp, "<p><a href=\"/reports\">Back to Reports</a></p>");
    response_append_body(resp, "</body></html>");

    report_free_rows(rows, row_count);

cleanup:
    free(start_date_str);
    free(end_date_str);
    free(export_csv_str);
    free(grouping_str);

    return resp;
}

/* Route handler: Export report as CSV */
http_response_t *handle_reports_export_csv(http_request_t *req) {
    http_response_t *resp = response_new();

    /* Check CSV export entitlement */
    if (!entitlement_can_export_csv(req->account_id)) {
        response_set_status(resp, 403);
        response_set_content_type(resp, "text/html");
        response_set_body(resp, "<h1>Access Denied</h1><p>CSV export not available on your plan</p>");
        return resp;
    }

    /* Simple placeholder CSV */
    response_set_content_type(resp, "text/csv");
    response_add_header(resp, "Content-Disposition", "attachment; filename=\"report.csv\"");
    response_set_body(resp, "Date,Users,Sessions,Accounts\n2024-01-01,5,10,1\n2024-01-02,3,8,1\n");

    return resp;
}

/* Free report rows */
void report_free_rows(report_row_t *rows, int row_count) {
    (void)row_count; /* Unused */
    free(rows);
}
