# Makefile for VictorDB servers with pkg-config

CC = gcc
CFLAGS = -Wall -Wextra -O2 -g3 $(shell pkg-config --cflags libcbor)
LDFLAGS = $(shell pkg-config --libs libcbor) -L/usr/local/lib/ -lvictor

# Common source files
COMMON_SRCS = buffer.c fileutils.c log.c opt.c protocol.c socket.c server.c
COMMON_OBJS = $(COMMON_SRCS:.c=.o)

# Vector index server specific sources
INDEX_SRCS = $(COMMON_SRCS) index_main.c viproto.c index_server.c
INDEX_OBJS = $(INDEX_SRCS:.c=.o)

# Table (key-value) server specific sources  
TABLE_SRCS = $(COMMON_SRCS) table_main.c kvproto.c table_server.c
TABLE_OBJS = $(TABLE_SRCS:.c=.o)

# Targets
INDEX_TARGET = victor_index
TABLE_TARGET = victor_table

.PHONY: all clean index table

all: $(INDEX_TARGET) $(TABLE_TARGET)

index: $(INDEX_TARGET)

table: $(TABLE_TARGET)

$(INDEX_TARGET): $(INDEX_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(TABLE_TARGET): $(TABLE_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(INDEX_OBJS) $(TABLE_OBJS) $(INDEX_TARGET) $(TABLE_TARGET)

