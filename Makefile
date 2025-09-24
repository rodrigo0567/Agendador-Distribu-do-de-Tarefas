CC = gcc
CFLAGS = -Wall -Wextra -pthread -I./include
TARGET = libtslog.a
TEST_TARGET = test_concurrent

# Arquivos fonte
LIB_SRCS = src/libtslog/tslog.c
LIB_OBJS = $(LIB_SRCS:.c=.o)

TEST_SRCS = tests/test_threads.c
TEST_OBJS = $(TEST_SRCS:.c=.o)

.PHONY: all clean test

all: $(TARGET) test

$(TARGET): $(LIB_OBJS)
	ar rcs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TARGET) $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $(TEST_TARGET) $(TEST_OBJS) -L. -ltslog

clean:
	rm -f $(LIB_OBJS) $(TEST_OBJS) $(TARGET) $(TEST_TARGET) *.log

run_test: test
	./$(TEST_TARGET)