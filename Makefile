CC=gcc
CFLAGS=-Wall -Wextra -Werror -pedantic -std=gnu11 -D_GNU_SOURCE

BUILD_DIR=build
BIN=tc

INC_DIR=include
CFLAGS+=-I$(INC_DIR)

# Debug
CFLAGS+=-g

OBJ_DIR=$(BUILD_DIR)/obj
SRC_DIR=src

SRC=$(wildcard $(SRC_DIR)/*.c)
OBJ=$(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC))

TORRENT_FILE=torrent/example.torrent

$(BUILD_DIR)/$(BIN): main.c $(OBJ) $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/$(BIN) $(OBJ) $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR): $(BUILD_DIR)
	mkdir -p $(OBJ_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

.PHONY: valgrind
valgrind: $(BUILD_DIR)/$(BIN)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $< -t $(TORRENT_FILE)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
