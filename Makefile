CC=gcc
CFLAGS=-Wall -Wextra -Werror -pedantic -std=c99

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

.PHONY: all
all: main.c $(OBJ) $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/$(BIN) $(OBJ) $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR): $(BUILD_DIR)
	mkdir -p $(OBJ_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

.PHONY: valgrind
valgrind: all
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $(BUILD_DIR)/$(BIN)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
