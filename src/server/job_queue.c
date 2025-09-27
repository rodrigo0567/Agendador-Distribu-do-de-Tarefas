#include "../common/protocol.h"
#include "job_queue.h"
#include <stdlib.h>
#include <string.h>
#include "database.h"


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

int job_queue_push_priority(job_queue_t *queue, const job_t *job) {
    if (!queue || !job) return -1;
    
    job_node_t *new_node = malloc(sizeof(job_node_t));
    if (!new_node) {
        tslog_error(queue->logger, "Falha ao alocar memória para novo job");
        return -1;
    }
    
    new_node->job = *job;
    new_node->job.job_id = queue->next_job_id++;
    new_node->job.status = JOB_PENDING;
    new_node->job.submitted_at = time(NULL);
    new_node->next = NULL;
    
    pthread_mutex_lock(&queue->mutex);
    
    // Inserção ordenada por prioridade (maior primeiro)
    if (queue->head == NULL) {
        // Fila vazia
        queue->head = new_node;
        queue->tail = new_node;
    } else {
        job_node_t *current = queue->head;
        job_node_t *prev = NULL;
        
        // Encontrar posição baseada na prioridade (decrescente)
        while (current != NULL && current->job.priority >= job->priority) {
            prev = current;
            current = current->next;
        }
        
        if (prev == NULL) {
            // Inserir no início (maior prioridade)
            new_node->next = queue->head;
            queue->head = new_node;
        } else {
            // Inserir no meio/fim
            new_node->next = prev->next;
            prev->next = new_node;
            
            if (prev == queue->tail) {
                queue->tail = new_node;
            }
        }
    }
    
    queue->size++;
    
    tslog_info(queue->logger, "Job %d adicionado (pri: %d, timeout: %d)", 
               new_node->job.job_id, new_node->job.priority, new_node->job.timeout);
    
    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);
    database_save_job(&new_node->job);

    return new_node->job.job_id;
}

// NOVA FUNÇÃO: Obter próximo job considerando prioridades
int job_queue_pop_priority(job_queue_t *queue, job_t *job) {
    if (!queue || !job) return -1;
    
    pthread_mutex_lock(&queue->mutex);
    
    // Aguardar até que haja jobs na fila
    while (queue->head == NULL) {
        tslog_debug(queue->logger, "Fila vazia - aguardando jobs...");
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }
    
    // Sempre pega o primeiro (já está ordenado por prioridade)
    job_node_t *node = queue->head;
    *job = node->job;
    job->status = JOB_RUNNING;
    job->started_at = time(NULL);
    
    queue->head = node->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    
    queue->size--;
    free(node);
    
    tslog_info(queue->logger, "Job %d removido para execução (pri: %d)", 
               job->job_id, job->priority);
    
    pthread_mutex_unlock(&queue->mutex);
    return 0;
}

// NOVA FUNÇÃO: Verificar timeouts
void job_queue_check_timeouts(job_queue_t *queue) {
    if (!queue) return;
    
    pthread_mutex_lock(&queue->mutex);
    
    time_t now = time(NULL);
    job_node_t *current = queue->head;
    int timeout_count = 0;
    
    while (current != NULL) {
        if (current->job.status == JOB_RUNNING) {
            time_t running_time = now - current->job.started_at;
            if (running_time > current->job.timeout) {
                tslog_warn(queue->logger, "Job %d excedeu timeout (%d > %d segundos)", 
                          current->job.job_id, (int)running_time, current->job.timeout);
                current->job.status = JOB_TIMEOUT;
                timeout_count++;
            }
        }
        current = current->next;
    }
    
    if (timeout_count > 0) {
        tslog_info(queue->logger, "%d jobs expirados por timeout", timeout_count);
    }
    
    pthread_mutex_unlock(&queue->mutex);
}

// NOVA FUNÇÃO: Estatísticas da fila
void job_queue_stats(job_queue_t *queue, int *total, int *pending, int *running, int *completed) {
    if (!queue) return;
    
    pthread_mutex_lock(&queue->mutex);
    
    *total = queue->size;
    *pending = 0;
    *running = 0;
    *completed = 0;
    
    job_node_t *current = queue->head;
    while (current != NULL) {
        switch (current->job.status) {
            case JOB_PENDING: (*pending)++; break;
            case JOB_RUNNING: (*running)++; break;
            case JOB_COMPLETED: (*completed)++; break;
            default: break;
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&queue->mutex);
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
            case JOB_TIMEOUT: status_str = "TIMEOUT"; break;
            default: status_str = "DESCONHECIDO";
        }
        
        char time_buf[64];
        struct tm *timeinfo = localtime(&current->job.submitted_at);
        strftime(time_buf, sizeof(time_buf), "%H:%M:%S", timeinfo);
        
        tslog_info(queue->logger, "#%d: [%s] %s (pri: %d, timeout: %ds) - %s",
                   current->job.job_id, time_buf, current->job.script,
                   current->job.priority, current->job.timeout, status_str);
        
        current = current->next;
        count++;
    }
    
    if (count == 0) {
        tslog_info(queue->logger, "Fila vazia");
    }
    
    pthread_mutex_unlock(&queue->mutex);
}