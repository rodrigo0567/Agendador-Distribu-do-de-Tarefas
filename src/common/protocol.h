#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>
#include <time.h>
#define MAX_SCRIPT_SIZE 1024
#define MAX_RESULT_SIZE 2048
#define SERVER_PORT 8080
#define WORKER_TIMEOUT 30  // segundos

typedef enum {
    JOB_PENDING = 0,
    JOB_RUNNING = 1,
    JOB_COMPLETED = 2,
    JOB_FAILED = 3,
    JOB_TIMEOUT = 4
} job_status_t;

typedef enum {
    CMD_SUBMIT_JOB = 1,
    CMD_REQUEST_JOB = 2,
    CMD_JOB_RESULT = 3,
    CMD_SHUTDOWN = 4,
    CMD_LIST_JOBS = 5,
    CMD_REGISTER_WORKER = 6,    
    CMD_HEARTBEAT = 7           
} command_type_t;

typedef struct {
    int job_id;
    int success;
    char output[MAX_RESULT_SIZE];
    double execution_time;      
} job_result_t;

typedef struct {
    int job_id;
    char script[MAX_SCRIPT_SIZE];
    int priority;               // 1-10 (10 = máxima)
    int timeout;               // segundos
    job_status_t status;
    time_t submitted_at;        
    time_t started_at;         
    int assigned_worker;        
} job_t;

typedef struct {
    int worker_id;
    char hostname[64];
    int active_jobs;
    time_t last_heartbeat;      
    int is_alive;               
} worker_info_t;

typedef struct {
    command_type_t type;
    int client_id;
    union {
        job_t job;
        job_result_t result;
        worker_info_t worker;  
    } data;
} message_t;

// Funções de serialização
int serialize_message(const message_t *msg, char *buffer, size_t size);
int deserialize_message(const char *buffer, size_t size, message_t *msg);

#endif