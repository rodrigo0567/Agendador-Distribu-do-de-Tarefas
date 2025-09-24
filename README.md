# Agendador-Distribu-do-de-Tarefas# Arquitetura do Sistema - Agendador Distribuído de Tarefas

## Visão Geral
Sistema cliente-servidor onde:
- **Servidor**: Mantém fila de jobs com prioridade
- **Workers**: Conectam-se, executam jobs e retornam resultados
- **Logging**: Thread-safe para todo o sistema

## Componentes Principais

### 1. Biblioteca libtslog
- Logging thread-safe com diferentes níveis
- Timestamps precisos
- Mutex para exclusão mútua

### 2. Servidor (Futuro)
- **JobQueue**: Fila thread-safe de jobs
- **WorkerManager**: Gerencia workers conectados
- **NetworkServer**: Aceita conexões TCP
- **MonitorCLI**: Interface administrativa

### 3. Cliente Worker (Futuro)
- **JobExecutor**: Executa scripts Python/Lua
- **NetworkClient**: Comunicação com servidor
- **ResultHandler**: Processa retornos

## Diagrama de Componentes

[Worker Client] ←→ [Server] ←→ [Monitor CLI]
↑ ↑
[Job Executor] [Job Queue] ←→ [Logger]
↑
[Worker Manager]


## Estrutura de Dados Principais
- `Job`: {id, script_path, priority, timeout, status}
- `Worker`: {id, status, current_job, connection_info}
- `ThreadSafeQueue`: Fila com mutex e condition variables

## Diagrama de Sequência - Logging com Múltiplas Threads
``` mermaid
sequenceDiagram
    participant T1 as Thread 1
    participant T2 as Thread 2
    participant T3 as Thread 3
    participant M as Mutex
    participant L as Logger
    participant F as Arquivo de Log

    Note over T1,T3: Início concorrente
    
    T1->>M: lock()
    Note right of T1: Exclusão mútua
    M-->>T1: mutex adquirido
    T1->>L: tslog_info("Thread 1 - Msg 1")
    L->>F: [TIMESTAMP] [INFO] Thread 1 - Msg 1
    T1->>M: unlock()
    
    T2->>M: lock() (bloqueia temporariamente)
    Note right of T2: Thread aguarda mutex
    M-->>T2: mutex adquirido
    T2->>L: tslog_error("Thread 2 - Erro X")
    L->>F: [TIMESTAMP] [ERROR] Thread 2 - Erro X
    T2->>M: unlock()
    
    T3->>M: lock()
    M-->>T3: mutex adquirido
    T3->>L: tslog_debug("Thread 3 - Debug info")
    L->>F: [TIMESTAMP] [DEBUG] Thread 3 - Debug info
    T3->>M: unlock()
    
    Note over T1,T3: Fim - logs preservam ordem crítica
```

## Diagrama de Sequência - Fluxo de Inicialização do Sistema
``` mermaid
sequenceDiagram
    participant Main as Programa Principal
    participant Logger as tslog_t
    participant Mutex as pthread_mutex_t
    participant File as Arquivo Log

    Main->>Logger: tslog_init()
    Logger->>Mutex: pthread_mutex_init()
    Note right of Logger: Inicializa mutex
    Logger->>File: fopen("app.log", "a")
    Note right of Logger: Abre arquivo de log
    File-->>Logger: FILE* handle
    Logger-->>Main: SUCCESS (0)
    
    Main->>Logger: tslog_info("Sistema iniciado")
    Logger->>Mutex: pthread_mutex_lock()
    Mutex-->>Logger: mutex OK
    Logger->>File: escreve timestamp + nível + mensagem
    Logger->>File: fflush()
    Logger->>Mutex: pthread_mutex_unlock()
    
    Main->>Logger: tslog_destroy()
    Logger->>Mutex: pthread_mutex_lock()
    Logger->>File: fclose()
    Logger->>Mutex: pthread_mutex_unlock()
    Logger->>Mutex: pthread_mutex_destroy()
```

## Diagrama de Sequência - Arquitetura Futura do Sistema Completo

``` mermaid
sequenceDiagram
    participant Worker as Worker Client
    participant Server as Servidor
    participant Queue as Job Queue
    participant Logger as tslog
    participant Monitor as Monitor CLI

    Note over Worker,Monitor: Fluxo de execução de job (Etapas futuras)
    
    Worker->>Server: CONNECT (socket)
    Server->>Logger: tslog_info("Worker X conectado")
    
    Worker->>Server: REQUEST_JOB
    Server->>Queue: get_next_job()
    Queue-->>Server: Job {id, script, priority}
    Server->>Logger: tslog_info("Job Y atribuído")
    Server-->>Worker: SEND_JOB(job_data)
    
    Worker->>Worker: EXECUTE_SCRIPT(python/lua)
    Worker->>Server: JOB_RESULT(success, output)
    Server->>Logger: tslog_info("Job Y finalizado")
    Server->>Monitor: UPDATE_STATS(job_result)
    
    Monitor->>Server: GET_STATS()
    Server-->>Monitor: Estatísticas atualizadas
```
