#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "../common/protocol.h"
#include "../common/job_executor.h"  
#include "../../include/tslog.h"
#define BUFFER_SIZE 4096

tslog_t logger;

int connect_to_server() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        tslog_error(&logger, "Erro ao criar socket");
        return -1;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        tslog_error(&logger, "Endereço inválido");
        close(sock);
        return -1;
    }
    
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        tslog_error(&logger, "Erro ao conectar com servidor");
        close(sock);
        return -1;
    }
    
    tslog_info(&logger, "Conectado ao servidor");
    return sock;
}

void register_worker(int sock) {
    char message[BUFFER_SIZE];
    snprintf(message, BUFFER_SIZE, "REGISTER_WORKER");
    write(sock, message, strlen(message));
    tslog_info(&logger, "Worker registrado no servidor");
}

int request_job(int sock) {
    char message[BUFFER_SIZE] = "REQUEST_JOB";
    write(sock, message, strlen(message));
    
    char response[BUFFER_SIZE];
    ssize_t bytes_read = read(sock, response, BUFFER_SIZE - 1);
    
    if (bytes_read <= 0) {
        return -1;
    }
    
    response[bytes_read] = '\0';
    
    if (strncmp(response, "JOB:", 4) == 0) {
        int job_id;
        char script[MAX_SCRIPT_SIZE];
        int timeout;
        
        sscanf(response, "JOB:%d:%[^:]:%d", &job_id, script, &timeout);
        tslog_info(&logger, "Job recebido: ID=%d, Script=%s", job_id, script);
        
        return job_id;
    } else if (strcmp(response, "NO_JOBS") == 0) {
        tslog_debug(&logger, "Nenhum job disponível");
        return 0;
    }
    
    return -1;
}

void send_job_result(int sock, int job_id, int success, const char *output, double exec_time) {
    char message[BUFFER_SIZE];
    snprintf(message, BUFFER_SIZE, "JOB_RESULT:%d:%d:%.2f:%s", 
             job_id, success, exec_time, output);
    
    write(sock, message, strlen(message));
    tslog_info(&logger, "Resultado do job %d enviado", job_id);
}

void worker_loop() {
    int sock = connect_to_server();
    if (sock < 0) return;
    
    register_worker(sock);
    
    while (1) {
        int job_id = request_job(sock);
        
    if (job_id > 0) {
    char output[MAX_RESULT_SIZE];
    double exec_time;
    
    if (job_id % 2 == 0) {
        exec_time = execute_script("print('Python Job', __import__('time').time())", 
                                  output, sizeof(output), 30);
    } else {
        exec_time = execute_script("print('Lua Job', os.time())", 
                                  output, sizeof(output), 30);
    }
    
    int success = (exec_time >= 0);
    send_job_result(sock, job_id, success, output, exec_time);
}
        
    }
    
    close(sock);
}

int main(int argc, char *argv[]) {
    if (tslog_init(&logger, "worker.log", TSLOG_INFO) != 0) {
        fprintf(stderr, "Erro ao inicializar logger do worker\n");
        return 1;
    }
    
    tslog_info(&logger, "=== WORKER INICIADO ===");
    tslog_info(&logger, "Worker pronto para processar jobs");
    
    worker_loop();
    
    tslog_info(&logger, "Worker finalizado");
    tslog_destroy(&logger);
    return 0;
}