#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>
#include "monitor_cli.h"
#include "job_queue.h"
#include "worker_manager.h"
#include "../../include/tslog.h"

#define INPUT_BUFFER_SIZE 256

static pthread_t monitor_thread;

void clear_screen() {
    printf("\033[2J\033[H");
}

// CORRIGIDO: usar mon->queue e mon->wm consistentemente
void display_dashboard(monitor_cli_t *mon) {
    pthread_mutex_lock(&mon->display_mutex);
    
    clear_screen();
    printf("=== AGENDADOR DISTRIBUÍDO - MONITOR ADMINISTRATIVO ===\n\n");
    
    // CORRIGIDO: usar queue diretamente
    printf("📊 ESTATÍSTICAS DE JOBS:\n");
    job_queue_list(mon->queue);
    printf("\n");
    
    // CORRIGIDO: usar wm diretamente
    printf("👥 WORKERS ATIVOS:\n");
    worker_manager_list(mon->wm);
    printf("\n");
    
    printf("⚙️  COMANDOS: list, stats, pause, resume, shutdown, clear, help, quit\n");
    printf("> ");
    fflush(stdout);
    
    pthread_mutex_unlock(&mon->display_mutex);
}

// CORRIGIDO: campos consistentes
void process_command(monitor_cli_t *mon, const char *command) {
    if (strncmp(command, "list", 4) == 0) {
        printf("\n=== LISTA COMPLETA DE JOBS ===\n");
        job_queue_list(mon->queue);  // CORRIGIDO: mon->queue
        printf("\n=== WORKERS ===\n");
        worker_manager_list(mon->wm);  // CORRIGIDO: mon->wm
        printf("\nPressione Enter para continuar...");
        getchar();
        
    } else if (strncmp(command, "stats", 5) == 0) {
        printf("\n=== ESTATÍSTICAS ===\n");
        job_queue_list(mon->queue);
        printf("\nPressione Enter para continuar...");
        getchar();
        
    } else if (strncmp(command, "pause", 5) == 0) {
        printf("\n⏸️  Sistema pausado.\n");
        tslog_info(mon->logger, "Sistema pausado via monitor CLI");
        
    } else if (strncmp(command, "resume", 6) == 0) {
        printf("\n▶️  Sistema retomado.\n");
        tslog_info(mon->logger, "Sistema retomado via monitor CLI");
        
    } else if (strncmp(command, "shutdown", 8) == 0) {
        printf("\n🛑 Desligamento solicitado. Confirmar? (s/N): ");
        fflush(stdout);
        
        char confirm[10];
        fgets(confirm, sizeof(confirm), stdin);
        
        if (confirm[0] == 's' || confirm[0] == 'S') {
            printf("Desligando sistema...\n");
            mon->running = 0;
            tslog_info(mon->logger, "Desligamento iniciado via monitor CLI");
        }
        
    } else if (strncmp(command, "clear", 5) == 0) {
        // Já é feito no display_dashboard
        
    } else if (strncmp(command, "help", 4) == 0) {
        printf("\n=== AJUDA DOS COMANDOS ===\n");
        printf("list     - Listar jobs e workers\n");
        printf("stats    - Estatísticas\n");
        printf("pause    - Pausar sistema\n");
        printf("resume   - Retomar sistema\n");
        printf("shutdown - Desligar sistema\n");
        printf("clear    - Limpar tela\n");
        printf("help     - Mostrar ajuda\n");
        printf("quit     - Sair do monitor\n");
        printf("\nPressione Enter para continuar...");
        getchar();
        
    } else if (strncmp(command, "quit", 4) == 0 || strncmp(command, "exit", 4) == 0) {
        printf("\nSaindo do monitor.\n");
        mon->running = 0;
        
    } else {
        printf("\n❌ Comando não reconhecido: %s\n", command);
        printf("Digite 'help' para ver os comandos.\n");
        printf("Pressione Enter para continuar...");
        getchar();
    }
}

void* monitor_thread_func(void *arg) {
    monitor_cli_t *mon = (monitor_cli_t*)arg;
    char input[INPUT_BUFFER_SIZE];
    
    tslog_info(mon->logger, "Monitor CLI iniciado");
    
    while (mon->running) {
        display_dashboard(mon);
        
        if (fgets(input, sizeof(input), stdin) != NULL) {
            input[strcspn(input, "\n")] = 0;
            
            if (strlen(input) > 0) {
                process_command(mon, input);
            }
        }
        
        sleep(2);
    }
    
    tslog_info(mon->logger, "Monitor CLI finalizado");
    return NULL;
}

// CORRIGIDO: assinatura consistente com o .h
int monitor_cli_init(monitor_cli_t *mon, job_queue_t *queue, worker_manager_t *wm, tslog_t *logger) {
    if (!mon || !queue || !wm || !logger) return -1;
    
    mon->queue = queue;
    mon->wm = wm;  // CORRIGIDO
    mon->logger = logger;
    mon->running = 1;
    
    if (pthread_mutex_init(&mon->display_mutex, NULL) != 0) {
        tslog_error(logger, "Falha ao inicializar mutex do monitor");
        return -1;
    }
    
    if (pthread_create(&monitor_thread, NULL, monitor_thread_func, mon) != 0) {
        tslog_error(logger, "Falha ao criar thread do monitor");
        pthread_mutex_destroy(&mon->display_mutex);
        return -1;
    }
    
    tslog_info(logger, "Monitor CLI inicializado");
    return 0;
}

void monitor_cli_destroy(monitor_cli_t *mon) {
    if (!mon) return;
    
    mon->running = 0;
    pthread_join(monitor_thread, NULL);
    pthread_mutex_destroy(&mon->display_mutex);
    
    tslog_info(mon->logger, "Monitor CLI destruído");
}

void monitor_cli_refresh(monitor_cli_t *mon) {
    (void)mon; // Não implementado ainda
}
