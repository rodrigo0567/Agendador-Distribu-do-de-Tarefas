CC = gcc
CFLAGS = -Wall -Wextra -pthread -I./include -I./src/common
TARGET = libtslog.a
SERVER_TARGET = server
CLIENT_TARGET = client
TEST_TARGET = test_concurrent

# Arquivos fonte
LIB_SRCS = src/libtslog/tslog.c
LIB_OBJS = $(LIB_SRCS:.c=.o)

SERVER_SRCS = src/server/server.c src/server/job_queue.c
SERVER_OBJS = $(SERVER_SRCS:.c=.o)

CLIENT_SRCS = src/client/client.c
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)

TEST_SRCS = tests/test_threads.c
TEST_OBJS = $(TEST_SRCS:.c=.o)

.PHONY: all clean test server client

all: $(TARGET) server client test

$(TARGET): $(LIB_OBJS)
	ar rcs $@ $^

server: $(TARGET) $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $(SERVER_TARGET) $(SERVER_OBJS) -L. -ltslog

client: $(TARGET) $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $(CLIENT_TARGET) $(CLIENT_OBJS) -L. -ltslog

test: $(TARGET) $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $(TEST_TARGET) $(TEST_OBJS) -L. -ltslog

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(LIB_OBJS) $(SERVER_OBJS) $(CLIENT_OBJS) $(TEST_OBJS) \
	      $(TARGET) $(SERVER_TARGET) $(CLIENT_TARGET) $(TEST_TARGET) *.log

run_server: server
	./$(SERVER_TARGET)

run_client: client
	./$(CLIENT_TARGET) interactive

run_test: test
	./$(TEST_TARGET)