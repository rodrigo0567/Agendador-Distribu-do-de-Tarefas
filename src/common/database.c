#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "database.h"
#include "../include/tslog.h"

#define DB_FILE "scheduler.db"

sqlite3 *db = NULL;
tslog_t *db_logger = NULL;

int database_init(tslog_t *logger) {
    int rc;
    db_logger = logger;
    
    rc = sqlite3_open(DB_FILE, &db);
    if (rc) {
        tslog_error(db_logger, "Não pode abrir database: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    tslog_info(db_logger, "Database SQLite aberto: %s", DB_FILE);
    
    // Criar tabelas se não existirem
    const char *sql = 
        "CREATE TABLE IF NOT EXISTS jobs ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "job_id INTEGER NOT NULL,"
        "script TEXT NOT NULL,"
        "priority INTEGER NOT NULL,"
        "timeout INTEGER NOT NULL,"
        "status INTEGER NOT NULL,"
        "submitted_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "started_at DATETIME,"
        "completed_at DATETIME,"
        "result_text TEXT,"
        "execution_time REAL,"
        "success INTEGER"
        ");"
        
        "CREATE TABLE IF NOT EXISTS workers ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "worker_id INTEGER NOT NULL,"
        "hostname TEXT NOT NULL,"
        "registered_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "last_heartbeat DATETIME,"
        "is_active INTEGER DEFAULT 1"
        ");";
    
    char *err_msg = NULL;
    rc = sqlite3_exec(db, sql, NULL, 0, &err_msg);
    
    if (rc != SQLITE_OK) {
        tslog_error(db_logger, "Erro criando tabelas: %s", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return -1;
    }
    
    tslog_info(db_logger, "Tabelas do database criadas/verificadas");
    return 0;
}

void database_close() {
    if (db) {
        sqlite3_close(db);
        tslog_info(db_logger, "Database fechado");
    }
}

int database_save_job(const job_t *job) {
    if (!db || !job) return -1;
    
    const char *sql = "INSERT INTO jobs (job_id, script, priority, timeout, status, submitted_at) "
                     "VALUES (?, ?, ?, ?, ?, datetime('now'));";
    
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        tslog_error(db_logger, "Erro preparando SQL: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, job->job_id);
    sqlite3_bind_text(stmt, 2, job->script, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, job->priority);
    sqlite3_bind_int(stmt, 4, job->timeout);
    sqlite3_bind_int(stmt, 5, job->status);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        tslog_error(db_logger, "Erro salvando job: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    tslog_debug(db_logger, "Job %d salvo no database", job->job_id);
    return 0;
}

int database_update_job_result(int job_id, int success, const char *result, double exec_time) {
    if (!db) return -1;
    
    const char *sql = "UPDATE jobs SET completed_at = datetime('now'), result_text = ?, "
                     "execution_time = ?, success = ?, status = ? WHERE job_id = ?;";
    
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        tslog_error(db_logger, "Erro preparando SQL: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, result, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 2, exec_time);
    sqlite3_bind_int(stmt, 3, success);
    sqlite3_bind_int(stmt, 4, success ? JOB_COMPLETED : JOB_FAILED);
    sqlite3_bind_int(stmt, 5, job_id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        tslog_error(db_logger, "Erro atualizando job: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    tslog_debug(db_logger, "Resultado do job %d atualizado no database", job_id);
    return 0;
}

int database_get_job_stats(int *total, int *completed, int *failed, double *avg_time) {
    if (!db) return -1;
    
    const char *sql = "SELECT COUNT(*), "
                     "SUM(CASE WHEN success = 1 THEN 1 ELSE 0 END), "
                     "SUM(CASE WHEN success = 0 THEN 1 ELSE 0 END), "
                     "AVG(execution_time) FROM jobs WHERE success IS NOT NULL;";
    
    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK) {
        tslog_error(db_logger, "Erro preparando SQL: %s", sqlite3_errmsg(db));
        return -1;
    }
    
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        *total = sqlite3_column_int(stmt, 0);
        *completed = sqlite3_column_int(stmt, 1);
        *failed = sqlite3_column_int(stmt, 2);
        *avg_time = sqlite3_column_double(stmt, 3);
    }
    
    sqlite3_finalize(stmt);
    return 0;
}