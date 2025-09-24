#include "tslog.h"
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

int tslog_init(tslog_t *logger, const char *filename, tslog_level_t level) {
    if (!logger) return -1;
    
    // Inicializar mutex
    if (pthread_mutex_init(&logger->mutex, NULL) != 0) {
        return -1;
    }
    
    // Abrir arquivo de log
    logger->log_file = fopen(filename, "a");
    if (!logger->log_file) {
        pthread_mutex_destroy(&logger->mutex);
        return -1;
    }
    
    logger->level = level;
    logger->timestamp_format = "%Y-%m-%d %H:%M:%S";
    
    return 0;
}

void tslog_destroy(tslog_t *logger) {
    if (!logger) return;
    
    pthread_mutex_lock(&logger->mutex);
    
    if (logger->log_file && logger->log_file != stdout && logger->log_file != stderr) {
        fclose(logger->log_file);
    }
    
    pthread_mutex_unlock(&logger->mutex);
    pthread_mutex_destroy(&logger->mutex);
}

void tslog_get_timestamp(char *buffer, size_t size, const char *format) {
    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    strftime(buffer, size, format, timeinfo);
}

const char* tslog_level_to_string(tslog_level_t level) {
    switch (level) {
        case TSLOG_ERROR: return "ERROR";
        case TSLOG_WARN:  return "WARN";
        case TSLOG_INFO:  return "INFO";
        case TSLOG_DEBUG: return "DEBUG";
        default:          return "UNKNOWN";
    }
}

void tslog_log(tslog_t *logger, tslog_level_t level, const char *format, ...) {
    if (!logger || level > logger->level) return;
    
    pthread_mutex_lock(&logger->mutex);
    
    // Timestamp
    char timestamp[64];
    tslog_get_timestamp(timestamp, sizeof(timestamp), logger->timestamp_format);
    
    // Nível do log
    const char *level_str = tslog_level_to_string(level);
    
    // Imprimir cabeçalho
    fprintf(logger->log_file, "[%s] [%s] ", timestamp, level_str);
    
    // Imprimir mensagem
    va_list args;
    va_start(args, format);
    vfprintf(logger->log_file, format, args);
    va_end(args);
    
    fprintf(logger->log_file, "\n");
    fflush(logger->log_file);
    
    pthread_mutex_unlock(&logger->mutex);
}

// Funções específicas por nível
void tslog_error(tslog_t *logger, const char *format, ...) {
    va_list args;
    va_start(args, format);
    pthread_mutex_lock(&logger->mutex);
    fprintf(logger->log_file, "[ERROR] ");
    vfprintf(logger->log_file, format, args);
    fprintf(logger->log_file, "\n");
    fflush(logger->log_file);
    pthread_mutex_unlock(&logger->mutex);
    va_end(args);
}

void tslog_warn(tslog_t *logger, const char *format, ...) {
    va_list args;
    va_start(args, format);
    pthread_mutex_lock(&logger->mutex);
    fprintf(logger->log_file, "[WARN] ");
    vfprintf(logger->log_file, format, args);
    fprintf(logger->log_file, "\n");
    fflush(logger->log_file);
    pthread_mutex_unlock(&logger->mutex);
    va_end(args);
}

void tslog_info(tslog_t *logger, const char *format, ...) {
    va_list args;
    va_start(args, format);
    pthread_mutex_lock(&logger->mutex);
    fprintf(logger->log_file, "[INFO] ");
    vfprintf(logger->log_file, format, args);
    fprintf(logger->log_file, "\n");
    fflush(logger->log_file);
    pthread_mutex_unlock(&logger->mutex);
    va_end(args);
}

void tslog_debug(tslog_t *logger, const char *format, ...) {
    va_list args;
    va_start(args, format);
    pthread_mutex_lock(&logger->mutex);
    fprintf(logger->log_file, "[DEBUG] ");
    vfprintf(logger->log_file, format, args);
    fprintf(logger->log_file, "\n");
    fflush(logger->log_file);
    pthread_mutex_unlock(&logger->mutex);
    va_end(args);
}