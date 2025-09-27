#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "../common/protocol.h"
#include "../include/job_queue.h"
#include "../include/tslog.h"
#include "monitor_cli.h"
#include "database.h"
#include "globals.h"
#include "worker_manager.h"  /* <-- incluído para garantir worker_manager_t */

#define MAX_CLIENTS 10
#define BUFFER_SIZE 2048

typedef struct {
    int socket;
    struct sockaddr_in address;
    job_queue_t *queue;
    tslog_t *logger;
} client_thread_args_t;

extern job_queue_t job_queue;
extern tslog_t logger;
extern monitor_cli_t monitor_cli;
extern int server_running;
extern worker_manager_t worker_manager;

void* client_handler(void *arg) {
    client_thread_args_t *args = (client_thread_args_t*)arg;
    int client_socket = args->socket;
    char buffer[BUFFER_SIZE];

    tslog_info(args->logger, "Nova conexão cliente aceita");

    while (server_running) {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytes_read = read(client_socket, buffer, BUFFER_SIZE - 1);

        if (bytes_read <= 0) {
            tslog_info(args->logger, "Cliente desconectado");
            break;
        }

        buffer[BUFFER_SIZE-1] = '\0'; // Garantir terminação
        tslog_info(args->logger, "Mensagem recebida: %s", buffer);

        char response[BUFFER_SIZE];
        int max_msg = BUFFER_SIZE - 20; // Espaço para "Servidor recebeu: "
        if (strlen(buffer) > (size_t) max_msg) {
            buffer[max_msg] = '\0'; // Truncar se necessário
        }
        snprintf(response, BUFFER_SIZE, "Servidor recebeu: %s", buffer);
        write(client_socket, response, strlen(response));

        if (strstr(buffer, "JOB:") != NULL) {
            job_t job;
            strncpy(job.script, buffer + 4, MAX_SCRIPT_SIZE - 1);
            job.script[MAX_SCRIPT_SIZE - 1] = '\0';
            job.priority = 5;
            job.timeout = 30;
            job.status = JOB_PENDING;

            int job_id = job_queue_push(args->queue, &job);

            char job_response[BUFFER_SIZE];
            snprintf(job_response, BUFFER_SIZE, "JOB_ACCEPTED:%d", job_id);
            write(client_socket, job_response, strlen(job_response));

            tslog_info(args->logger, "Job %d aceito: %s", job_id, job.script);
        }
    }

    close(client_socket);
    free(args);
    return NULL;
}

void* queue_monitor(void *arg) {
    tslog_t *logger = (tslog_t*)arg;
    (void)logger; // evitar warning

    while (server_running) {
        sleep(10); // Verificar a cada 10 segundos
        job_queue_list(&job_queue);
    }

    return NULL;
}

/* CORREÇÃO: renomeada para evitar conflito com pthread_t */
void* worker_monitor_func(void *arg) {
    worker_manager_t *manager = (worker_manager_t*)arg;

    while (server_running) {
        sleep(30);
        worker_manager_check_heartbeats(manager);
        worker_manager_list(manager);
    }

    return NULL;
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    pthread_t worker_monitor_thread;

    /* Inicializar logger */
    if (tslog_init(&logger, "server.log", TSLOG_INFO) != 0) {
        fprintf(stderr, "Erro ao inicializar logger\n");
        return 1;
    }

    tslog_info(&logger, "=== SERVIDOR INICIADO ===");

    /* INICIALIZAÇÃO DO DATABASE (NOVO) */
    if (database_init(&logger) != 0) {
        tslog_error(&logger, "Erro ao inicializar database");
        monitor_cli_destroy(&monitor_cli);
        worker_manager_destroy(&worker_manager);
        job_queue_destroy(&job_queue);
        return 1;
    }

    /* Inicializar fila de jobs */
    if (job_queue_init(&job_queue, &logger) != 0) {
        tslog_error(&logger, "Erro ao inicializar fila de jobs");
        return 1;
    }

    /* INICIALIZAÇÃO DO WORKER MANAGER (NOVO) */
    if (worker_manager_init(&worker_manager, &logger, &job_queue) != 0) {
        tslog_error(&logger, "Erro ao inicializar worker manager");
        job_queue_destroy(&job_queue);
        return 1;
    }

    /* INICIALIZAÇÃO DO MONITOR CLI (NOVO) */
    if (monitor_cli_init(&monitor_cli, &job_queue, &worker_manager, &logger) != 0) {
        tslog_error(&logger, "Erro ao inicializar monitor CLI");
        worker_manager_destroy(&worker_manager);
        job_queue_destroy(&job_queue);
        return 1;
    }

    /* Criar socket */
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        tslog_error(&logger, "Erro ao criar socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        tslog_error(&logger, "Erro no bind");
        close(server_socket);
        return 1;
    }

    if (listen(server_socket, MAX_CLIENTS) < 0) {
        tslog_error(&logger, "Erro no listen");
        close(server_socket);
        return 1;
    }

    tslog_info(&logger, "Servidor ouvindo na porta %d", SERVER_PORT);

    /* Thread para monitorar workers (NOVO) */
    if (pthread_create(&worker_monitor_thread, NULL, worker_monitor_thread_func, &worker_manager) != 0) {
        tslog_error(&logger, "Erro ao criar thread de monitor de workers");
        /* seguir com execução — dependendo do design, talvez deva abortar */
    }

    /* Aceitar conexões */
    while (server_running) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            if (server_running) {
                tslog_error(&logger, "Erro ao aceitar conexão");
            }
            continue;
        }

        client_thread_args_t *args = malloc(sizeof(client_thread_args_t));
        args->socket = client_socket;
        args->address = client_addr;
        args->queue = &job_queue;
        args->logger = &logger;

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, client_handler, args) != 0) {
            tslog_error(&logger, "Erro ao criar thread para cliente");
            close(client_socket);
            free(args);
        } else {
            pthread_detach(client_thread);
        }
    }

    /* Cleanup */
    tslog_info(&logger, "Servidor finalizando...");

    close(server_socket);
    database_close();
    monitor_cli_destroy(&monitor_cli);
    worker_manager_destroy(&worker_manager);
    job_queue_destroy(&job_queue);
    tslog_destroy(&logger);

    return 0;
}
void job_queue_get_stats(job_queue_t *queue, int *total, int *pending, int *running, int *completed) {
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