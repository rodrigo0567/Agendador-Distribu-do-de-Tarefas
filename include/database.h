#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include "../common/protocol.h"
#include "../include/tslog.h"

// Inicialização e finalização
int database_init(tslog_t *logger);
void database_close();

// Operações com jobs
int database_save_job(const job_t *job);
int database_update_job_result(int job_id, int success, const char *result, double exec_time);
int database_get_job_stats(int *total, int *completed, int *failed, double *avg_time);

#endif