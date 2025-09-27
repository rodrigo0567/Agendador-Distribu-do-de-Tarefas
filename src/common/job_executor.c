#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include "job_executor.h"
#include "../../include/tslog.h"

double execute_script(const char *script, char *output, size_t output_size, int timeout) {
    FILE *fp;
    char command[2048];  // Aumentado para comandos maiores
    char temp_output[4096];
    double execution_time = 0.0;
    
    // Detectar tipo de script e criar comando apropriado
    if (strstr(script, "python") != NULL || strstr(script, ".py") != NULL) {
        // Usar python3 explicitamente
        snprintf(command, sizeof(command), "timeout %d python3 -c \"%s\" 2>&1", timeout, script);
    } else if (strstr(script, "lua") != NULL || strstr(script, ".lua") != NULL) {
        // Usar lua explicitamente
        snprintf(command, sizeof(command), "timeout %d lua -e \"%s\" 2>&1", timeout, script);
    } else {
        // Comando genérico
        snprintf(command, sizeof(command), "timeout %d %s 2>&1", timeout, script);
    }
    
    tslog_info(NULL, "Executando comando: %s", command);  // Log para debug
    
    clock_t start = clock();
    
    // Executar comando
    fp = popen(command, "r");
    if (fp == NULL) {
        snprintf(output, output_size, "Erro ao executar comando: %s", command);
        return -1.0;
    }
    
    // Ler output
    temp_output[0] = '\0';
    size_t total_read = 0;
    char buffer[256];
    
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (total_read + strlen(buffer) < sizeof(temp_output) - 1) {
            strcat(temp_output, buffer);
            total_read += strlen(buffer);
        } else {
            break; // Evitar overflow
        }
    }
    
    int status = pclose(fp);
    clock_t end = clock();
    
    execution_time = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    // Processar resultado
    if (WIFEXITED(status)) {
        int exit_status = WEXITSTATUS(status);
        if (exit_status == 124) { // timeout
            snprintf(output, output_size, "TIMEOUT: Script excedeu o tempo limite de %d segundos", timeout);
        } else if (exit_status != 0) {
            snprintf(output, output_size, "ERRO[%d]: %s", exit_status, temp_output);
        } else {
            // Remover quebras de linha extras
            if (strlen(temp_output) > 0 && temp_output[strlen(temp_output)-1] == '\n') {
                temp_output[strlen(temp_output)-1] = '\0';
            }
            snprintf(output, output_size, "%s", temp_output);
        }
    } else {
        snprintf(output, output_size, "Script terminou anormalmente. Status: %d", status);
    }
    
    return execution_time;
}

// Função auxiliar para testar scripts de arquivo
double execute_script_file(const char *filename, char *output, size_t output_size, int timeout) {
    char command[1024];
    snprintf(command, sizeof(command), "timeout %d %s 2>&1", timeout, filename);
    return execute_script(command, output, output_size, timeout);
}

#ifdef TEST_JOB_EXECUTOR
int main() {
    char output[1024];
    
    printf("=== TESTE DO EXECUTOR DE SCRIPTS ===\n");
    
    // Teste Python básico
    printf("\n1. Testando Python básico...\n");
    double time = execute_script("print('Hello Python'); print('2 + 2 =', 2+2)", 
                                output, sizeof(output), 5);
    printf("Tempo: %.2fs\n", time);
    printf("Output: %s\n", output);
    
    // Teste Python com import
    printf("\n2. Testando Python com import...\n");
    time = execute_script("import math; print('Raiz quadrada de 16:', math.sqrt(16))", 
                         output, sizeof(output), 5);
    printf("Tempo: %.2fs\n", time);
    printf("Output: %s\n", output);
    
    // Teste Lua básico
    printf("\n3. Testando Lua básico...\n");
    time = execute_script("print('Hello Lua'); print('2 + 2 =', 2+2)", 
                         output, sizeof(output), 5);
    printf("Tempo: %.2fs\n", time);
    printf("Output: %s\n", output);
    
    // Teste Lua com loop
    printf("\n4. Testando Lua com loop...\n");
    time = execute_script("for i=1,3 do print('Numero:', i) end", 
                         output, sizeof(output), 5);
    printf("Tempo: %.2fs\n", time);
    printf("Output: %s\n", output);
    
    // Teste timeout
    printf("\n5. Testando timeout...\n");
    time = execute_script("import time; time.sleep(10); print('Este não deve aparecer')", 
                         output, sizeof(output), 2);
    printf("Tempo: %.2fs\n", time);
    printf("Output: %s\n", output);
    
    printf("\n=== TESTE CONCLUÍDO ===\n");
    return 0;
}
#endif