#include "../include/tslog.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

#define NUM_THREADS 5
#define LOGS_PER_THREAD 10

tslog_t logger;

void* worker_thread(void* arg) {
    int thread_id = *(int*)arg;
    
    for (int i = 0; i < LOGS_PER_THREAD; i++) {
        tslog_info(&logger, "Thread %d - Mensagem %d", thread_id, i);
        usleep(rand() % 100000); // Pequena variação no tempo
    }
    
    tslog_debug(&logger, "Thread %d finalizada", thread_id);
    return NULL;
}

int main() {
    // Inicializar logger
    if (tslog_init(&logger, "test_concurrent.log", TSLOG_DEBUG) != 0) {
        fprintf(stderr, "Erro ao inicializar logger\n");
        return 1;
    }
    
    tslog_info(&logger, "=== INÍCIO TESTE CONCORRÊNCIA ===");
    
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    
    // Criar threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i + 1;
        if (pthread_create(&threads[i], NULL, worker_thread, &thread_ids[i]) != 0) {
            tslog_error(&logger, "Erro ao criar thread %d", i);
            return 1;
        }
        tslog_debug(&logger, "Thread %d criada", i + 1);
    }
    
    // Aguardar threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        tslog_debug(&logger, "Thread %d finalizada", i + 1);
    }
    
    tslog_info(&logger, "=== FIM TESTE CONCORRÊNCIA ===");
    
    tslog_destroy(&logger);
    return 0;
}