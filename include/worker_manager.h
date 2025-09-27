#ifndef WORKER_MANAGER_H
#define WORKER_MANAGER_H

#include "../common/protocol.h"
#include "job_queue.h"
#include "../include/tslog.h"

typedef struct worker_manager_t {
    int sockfd;
    job_queue_t *queue;
    tslog_t *logger;
    pthread_mutex_t lock;
} worker_manager_t;

int worker_manager_init(worker_manager_t *manager, tslog_t *logger, job_queue_t *queue);
void worker_manager_destroy(worker_manager_t *manager);

int worker_manager_register(worker_manager_t *manager, int socket, const char *hostname);
int worker_manager_assign_job(worker_manager_t *manager, int worker_id, job_t *job);
void worker_manager_heartbeat(worker_manager_t *manager, int worker_id);
void worker_manager_check_heartbeats(worker_manager_t *manager);
void worker_manager_list(worker_manager_t *manager);

void* worker_monitor_thread_func(void *arg);

#endif