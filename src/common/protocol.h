#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>  
#include <stdint.h>  
#define MAX_SCRIPT_SIZE 1024
#define MAX_RESULT_SIZE 2048
#define SERVER_PORT 8080

typedef enum {
    JOB_PENDING = 0,
    JOB_RUNNING = 1,
    JOB_COMPLETED = 2,
    JOB_FAILED = 3
} job_status_t;

typedef enum {
    CMD_SUBMIT_JOB = 1,
    CMD_REQUEST_JOB = 2,
    CMD_JOB_RESULT = 3,
    CMD_SHUTDOWN = 4,
    CMD_LIST_JOBS = 5
} command_type_t;

typedef struct {
    int job_id;
    int success;           // 0 = falha, 1 = sucesso
    char output[MAX_RESULT_SIZE];
} job_result_t;

typedef struct {
    int job_id;
    char script[MAX_SCRIPT_SIZE];
    int priority;           // 1-10 (10 = máxima prioridade)
    int timeout;           // segundos
    job_status_t status;
} job_t;

typedef struct {
    command_type_t type;
    int client_id;
    union {
        job_t job;                    
        job_result_t result;          
    } data;
} message_t;

// Funções de serialização (implementação futura)
int serialize_message(const message_t *msg, char *buffer, size_t size);
int deserialize_message(const char *buffer, size_t size, message_t *msg);

#endif