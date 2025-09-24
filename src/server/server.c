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

#define MAX_CLIENTS 10
#define BUFFER_SIZE 2048

typedef struct {
    int socket;
    struct sockaddr_in address;
    job_queue_t *queue;
    tslog_t *logger;
} client_thread_args_t;

job_queue_t job_queue;
tslog_t logger;
int server_running = 1;

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
        
        // Simples echo para teste (com truncamento seguro)
        buffer[BUFFER_SIZE-1] = '\0'; // Garantir terminação
        tslog_info(args->logger, "Mensagem recebida: %s", buffer);
        
        // Responder ao cliente com truncamento seguro
        char response[BUFFER_SIZE];
        int max_msg = BUFFER_SIZE - 20; // Espaço para "Servidor recebeu: "
        if (strlen(buffer) > max_msg) {
            buffer[max_msg] = '\0'; // Truncar se necessário
        }
        snprintf(response, BUFFER_SIZE, "Servidor recebeu: %s", buffer);
        write(client_socket, response, strlen(response));
        
        // Simular processamento de job
        if (strstr(buffer, "JOB:") != NULL) {
            job_t job;
            strncpy(job.script, buffer + 4, MAX_SCRIPT_SIZE - 1);
            job.script[MAX_SCRIPT_SIZE - 1] = '\0'; // Garantir terminação
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
    (void)logger; // Marcar como usado para evitar warning
    
    while (server_running) {
        sleep(10); // Verificar a cada 10 segundos
        job_queue_list(&job_queue);
    }
    
    return NULL;
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    pthread_t monitor_thread;
    
    // Inicializar logger
    if (tslog_init(&logger, "server.log", TSLOG_INFO) != 0) {
        fprintf(stderr, "Erro ao inicializar logger\n");
        return 1;
    }
    
    tslog_info(&logger, "=== SERVIDOR INICIADO ===");
    
    // Inicializar fila de jobs
    if (job_queue_init(&job_queue, &logger) != 0) {
        tslog_error(&logger, "Erro ao inicializar fila de jobs");
        return 1;
    }
    
    // Criar socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        tslog_error(&logger, "Erro ao criar socket");
        return 1;
    }
    
    // Configurar endereço
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);
    
    // Bind
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        tslog_error(&logger, "Erro no bind");
        close(server_socket);
        return 1;
    }
    
    // Listen
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        tslog_error(&logger, "Erro no listen");
        close(server_socket);
        return 1;
    }
    
    tslog_info(&logger, "Servidor ouvindo na porta %d", SERVER_PORT);
    
    // Thread para monitorar a fila
    pthread_create(&monitor_thread, NULL, queue_monitor, &logger);
    
    // Aceitar conexões
    while (server_running) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            if (server_running) {
                tslog_error(&logger, "Erro ao aceitar conexão");
            }
            continue;
        }
        
        // Criar thread para cliente
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
            pthread_detach(client_thread); // Não precisamos join
        }
    }
    
    
    tslog_info(&logger, "Servidor finalizando...");
    close(server_socket);
    job_queue_destroy(&job_queue);
    tslog_destroy(&logger);
    
    return 0;
}