#ifndef TSLOG_H
#define TSLOG_H

#include <stdio.h>
#include <time.h>
#include <pthread.h>

typedef enum {
    TSLOG_ERROR,
    TSLOG_WARN,
    TSLOG_INFO,
    TSLOG_DEBUG
} tslog_level_t;

typedef struct {
    FILE *log_file;
    tslog_level_t level;
    pthread_mutex_t mutex;
    char *timestamp_format;
} tslog_t;

// Inicialização e destruição
int tslog_init(tslog_t *logger, const char *filename, tslog_level_t level);
void tslog_destroy(tslog_t *logger);

// Funções de logging
void tslog_log(tslog_t *logger, tslog_level_t level, const char *format, ...);
void tslog_error(tslog_t *logger, const char *format, ...);
void tslog_warn(tslog_t *logger, const char *format, ...);
void tslog_info(tslog_t *logger, const char *format, ...);
void tslog_debug(tslog_t *logger, const char *format, ...);

const char* tslog_level_to_string(tslog_level_t level);
void tslog_get_timestamp(char *buffer, size_t size, const char *format);

#endif