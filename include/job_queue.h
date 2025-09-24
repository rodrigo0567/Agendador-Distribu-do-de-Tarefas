#ifndef JOB_QUEUE_H
#define JOB_QUEUE_H

#include "tslog.h"
#include "../src/common/protocol.h"  
#include <pthread.h>

typedef struct job_node {
    job_t job;
    struct job_node *next;
} job_node_t;

typedef struct {
    job_node_t *head;
    job_node_t *tail;
    int size;
    int next_job_id;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    tslog_t *logger;
} job_queue_t;

// Inicialização/destruição
int job_queue_init(job_queue_t *queue, tslog_t *logger);
void job_queue_destroy(job_queue_t *queue);

// Operações
int job_queue_push(job_queue_t *queue, const job_t *job);
int job_queue_pop(job_queue_t *queue, job_t *job);
int job_queue_size(job_queue_t *queue);
void job_queue_list(job_queue_t *queue);

#endif