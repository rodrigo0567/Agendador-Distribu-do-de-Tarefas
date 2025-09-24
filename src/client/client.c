#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../common/protocol.h"
#include "../include/tslog.h"

#define BUFFER_SIZE 2048

tslog_t logger;

void print_usage() {
    printf("Uso: client <comando> [argumentos]\n");
    printf("Comandos:\n");
    printf("  submit <script>    - Submeter um job\n");
    printf("  list               - Listar jobs no servidor\n");
    printf("  interactive        - Modo interativo\n");
}

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

int submit_job(const char *script) {
    int sock = connect_to_server();
    if (sock < 0) return -1;
    
    char message[BUFFER_SIZE];
    snprintf(message, BUFFER_SIZE, "JOB:%s", script);
    
    if (write(sock, message, strlen(message)) < 0) {
        tslog_error(&logger, "Erro ao enviar job");
        close(sock);
        return -1;
    }
    
    char response[BUFFER_SIZE];
    ssize_t bytes_read = read(sock, response, BUFFER_SIZE - 1);
    if (bytes_read > 0) {
        response[bytes_read] = '\0';
        printf("Resposta do servidor: %s\n", response);
    }
    
    close(sock);
    return 0;
}

void interactive_mode() {
    printf("Modo interativo - Ctrl+C para sair\n");
    
    while (1) {
        printf("\n> ");
        fflush(stdout);
        
        char input[BUFFER_SIZE];
        if (!fgets(input, BUFFER_SIZE, stdin)) {
            break;
        }
        
        input[strcspn(input, "\n")] = 0; // Remover newline
        
        if (strlen(input) == 0) continue;
        
        if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0) {
            break;
        }
        
        if (strncmp(input, "submit ", 7) == 0) {
            submit_job(input + 7);
        } else {
            printf("Comando desconhecido. Use 'submit <script>' ou 'quit'\n");
        }
    }
}

int main(int argc, char *argv[]) {
    if (tslog_init(&logger, "client.log", TSLOG_INFO) != 0) {
        fprintf(stderr, "Erro ao inicializar logger do cliente\n");
        return 1;
    }
    
    if (argc < 2) {
        print_usage();
        tslog_destroy(&logger);
        return 1;
    }
    
    if (strcmp(argv[1], "submit") == 0) {
        if (argc < 3) {
            printf("Erro: script não especificado\n");
            print_usage();
        } else {
            submit_job(argv[2]);
        }
    } else if (strcmp(argv[1], "interactive") == 0) {
        interactive_mode();
    } else {
        printf("Comando desconhecido: %s\n", argv[1]);
        print_usage();
    }
    
    tslog_destroy(&logger);
    return 0;
} 