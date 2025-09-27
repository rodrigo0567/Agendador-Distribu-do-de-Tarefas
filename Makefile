CC = gcc
CFLAGS = -Wall -Wextra -pthread -I./include -I./src/common
LDFLAGS = -pthread -lsqlite3
TARGET = libtslog.a
SERVER_TARGET = server
CLIENT_TARGET = client
WORKER_TARGET = worker
TEST_TARGET = test_concurrent

# Arquivos fonte
LIB_SRCS = src/libtslog/tslog.c
LIB_OBJS = $(LIB_SRCS:.c=.o)

SERVER_SRCS = src/server/server.c src/server/job_queue.c src/server/worker_manager.c src/server/monitor_cli.c src/server/globals.c src/common/database.c
SERVER_OBJS = $(SERVER_SRCS:.c=.o)

CLIENT_SRCS = src/client/client.c
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)

WORKER_SRCS = src/client/worker_client.c src/common/job_executor.c
WORKER_OBJS = $(WORKER_SRCS:.c=.o)

TEST_SRCS = tests/test_threads.c
TEST_OBJS = $(TEST_SRCS:.c=.o)

.PHONY: all clean test server client worker

all: $(TARGET) server client worker test

$(TARGET): $(LIB_OBJS)
	ar rcs $@ $^

server: $(TARGET) $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $(SERVER_TARGET) $(SERVER_OBJS) -L. -ltslog $(LDFLAGS)

client: $(TARGET) $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $(CLIENT_TARGET) $(CLIENT_OBJS) -L. -ltslog $(LDFLAGS)

worker: $(TARGET) $(WORKER_OBJS)
	$(CC) $(CFLAGS) -o $(WORKER_TARGET) $(WORKER_OBJS) -L. -ltslog $(LDFLAGS)

test: $(TARGET) $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $(TEST_TARGET) $(TEST_OBJS) -L. -ltslog $(LDFLAGS)

test_executor: src/common/job_executor.c libtslog.a
	$(CC) -DTEST_JOB_EXECUTOR -I./include -I./src/common -o test_executor src/common/job_executor.c -L. -ltslog

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(LIB_OBJS) $(SERVER_OBJS) $(CLIENT_OBJS) $(WORKER_OBJS) $(TEST_OBJS) \
	      $(TARGET) $(SERVER_TARGET) $(CLIENT_TARGET) $(WORKER_TARGET) $(TEST_TARGET) *.log scheduler.db

run_server: server
	./$(SERVER_TARGET)

run_client: client
	./$(CLIENT_TARGET) interactive

run_worker: worker
	./$(WORKER_TARGET)

run_test: test
	./$(TEST_TARGET)

demo: all
	./scripts/run_complete_demo.sh
