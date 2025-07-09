# Makefile for IndexKV daemon with pkg-config

CC = gcc
CFLAGS = -Wall -Wextra -O2 -g $(shell pkg-config --cflags libcbor)
LDFLAGS = $(shell pkg-config --libs libcbor) -lvictor

SRCS = buffer.c fileutils.c log.c main.c opt.c protocol.c server.c socket.c
OBJS = $(SRCS:.c=.o)
TARGET = victor

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

