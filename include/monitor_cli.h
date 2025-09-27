#ifndef MONITOR_CLI_H
#define MONITOR_CLI_H

#include "job_queue.h"
#include "worker_manager.h"
#include "../include/tslog.h"

typedef struct monitor_cli_t {
    job_queue_t *queue;
    worker_manager_t *wm;  // ADICIONADO
    tslog_t *logger;
    int running;
    pthread_mutex_t display_mutex;
} monitor_cli_t;

// CORRIGIDO: adicionar worker_manager_t *wm
int monitor_cli_init(monitor_cli_t *mon, job_queue_t *queue, worker_manager_t *wm, tslog_t *logger);
void monitor_cli_destroy(monitor_cli_t *mon);
void monitor_cli_refresh(monitor_cli_t *mon);
void* monitor_thread_func(void *arg);

#endif
