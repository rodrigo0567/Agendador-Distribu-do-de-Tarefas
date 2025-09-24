#include "../common/protocol.h"
#include "job_queue.h"
#include <stdlib.h>
#include <string.h>

int job_queue_init(job_queue_t *queue, tslog_t *logger) {
    if (!queue || !logger) return -1;
    
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    queue->next_job_id = 1;
    queue->logger = logger;
    
    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        tslog_error(logger, "Falha ao inicializar mutex da fila");
        return -1;
    }
    
    if (pthread_cond_init(&queue->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&queue->mutex);
        tslog_error(logger, "Falha ao inicializar condition variable");
        return -1;
    }
    
    tslog_info(logger, "Fila de jobs inicializada");
    return 0;
}

void job_queue_destroy(job_queue_t *queue) {
    if (!queue) return;
    
    pthread_mutex_lock(&queue->mutex);
    
    // Liberar todos os nós
    job_node_t *current = queue->head;
    while (current != NULL) {
        job_node_t *next = current->next;
        free(current);
        current = next;
    }
    
    pthread_mutex_unlock(&queue->mutex);
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->not_empty);
    
    tslog_info(queue->logger, "Fila de jobs destruída");
}

int job_queue_push(job_queue_t *queue, const job_t *job) {
    if (!queue || !job) return -1;
    
    job_node_t *new_node = malloc(sizeof(job_node_t));
    if (!new_node) {
        tslog_error(queue->logger, "Falha ao alocar memória para novo job");
        return -1;
    }
    
    // Copiar job e atribuir ID
    new_node->job = *job;
    new_node->job.job_id = queue->next_job_id++;
    new_node->job.status = JOB_PENDING;
    new_node->next = NULL;
    
    pthread_mutex_lock(&queue->mutex);
    
    if (queue->tail == NULL) {
        // Fila vazia
        queue->head = new_node;
        queue->tail = new_node;
    } else {
        // Adicionar ao final
        queue->tail->next = new_node;
        queue->tail = new_node;
    }
    
    queue->size++;
    
    tslog_info(queue->logger, "Job %d adicionado à fila (script: %s)", 
               new_node->job.job_id, new_node->job.script);
    
    // Sinalizar que a fila não está mais vazia
    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
    
    return new_node->job.job_id;
}

int job_queue_pop(job_queue_t *queue, job_t *job) {
    if (!queue || !job) return -1;
    
    pthread_mutex_lock(&queue->mutex);
    
    // Aguardar até que haja jobs na fila
    while (queue->head == NULL) {
        tslog_debug(queue->logger, "Fila vazia - aguardando jobs...");
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }
    
    // Remover do início
    job_node_t *node = queue->head;
    *job = node->job;
    job->status = JOB_RUNNING;
    
    queue->head = node->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    
    queue->size--;
    free(node);
    
    tslog_info(queue->logger, "Job %d removido da fila para execução", job->job_id);
    pthread_mutex_unlock(&queue->mutex);
    
    return 0;
}

int job_queue_size(job_queue_t *queue) {
    if (!queue) return -1;
    
    pthread_mutex_lock(&queue->mutex);
    int size = queue->size;
    pthread_mutex_unlock(&queue->mutex);
    
    return size;
}

void job_queue_list(job_queue_t *queue) {
    if (!queue) return;
    
    pthread_mutex_lock(&queue->mutex);
    
    tslog_info(queue->logger, "=== FILA DE JOBS (%d jobs) ===", queue->size);
    
    job_node_t *current = queue->head;
    int count = 0;
    
    while (current != NULL) {
        const char *status_str;
        switch (current->job.status) {
            case JOB_PENDING: status_str = "PENDENTE"; break;
            case JOB_RUNNING: status_str = "EXECUTANDO"; break;
            case JOB_COMPLETED: status_str = "CONCLUÍDO"; break;
            case JOB_FAILED: status_str = "FALHOU"; break;
            default: status_str = "DESCONHECIDO";
        }
        
        tslog_info(queue->logger, "Job %d: %s (pri: %d, timeout: %ds) - %s",
                   current->job.job_id, current->job.script,
                   current->job.priority, current->job.timeout, status_str);
        
        current = current->next;
        count++;
    }
    
    if (count == 0) {
        tslog_info(queue->logger, "Fila vazia");
    }
    
    pthread_mutex_unlock(&queue->mutex);
}