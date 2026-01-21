#ifndef DB_H
#define DB_H

#include <sqlite3.h>

/* Global database connection */
extern sqlite3 *g_db;

/* Initialize database connection and schema */
int db_init(const char *db_path, const char *schema_path);

/* Close database connection */
void db_close(void);

/* Begin transaction */
int db_begin_transaction(void);

/* Commit transaction */
int db_commit_transaction(void);

/* Rollback transaction */
int db_rollback_transaction(void);

/* Execute SQL statement (no results expected) */
int db_exec(const char *sql);

/* Prepare SQL statement */
int db_prepare(const char *sql, sqlite3_stmt **stmt);

/* Finalize prepared statement */
void db_finalize(sqlite3_stmt *stmt);

/* Helper: bind text parameter */
int db_bind_text(sqlite3_stmt *stmt, int index, const char *value);

/* Helper: bind integer parameter */
int db_bind_int(sqlite3_stmt *stmt, int index, int value);

/* Helper: bind int64 parameter */
int db_bind_int64(sqlite3_stmt *stmt, int index, sqlite3_int64 value);

/* Helper: get column as text (returns pointer to internal SQLite memory) */
const char *db_column_text(sqlite3_stmt *stmt, int index);

/* Helper: get column as integer */
int db_column_int(sqlite3_stmt *stmt, int index);

/* Helper: get column as int64 */
sqlite3_int64 db_column_int64(sqlite3_stmt *stmt, int index);

/* Get last insert rowid */
sqlite3_int64 db_last_insert_rowid(void);

/* Get error message */
const char *db_error_message(void);

/* Backup database to file */
int db_backup(const char *backup_path);

#endif /* DB_H */
