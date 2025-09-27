#ifndef GLOBALS_H
#define GLOBALS_H

#include "job_queue.h"
#include "worker_manager.h"
#include "monitor_cli.h"
#include "../include/tslog.h"

// Declarações globais
extern job_queue_t job_queue;
extern tslog_t logger;
extern worker_manager_t worker_manager;
extern monitor_cli_t monitor_cli;
extern int server_running;

#endif
