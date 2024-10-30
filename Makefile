CC=gcc
CFLAGS=-Wall -Wextra -Werror -pedantic -std=c99

BIN=tc

.PHONY: all
all: main

.PHONY: clean
main: main.c
	$(CC) $(CFLAGS) -o $(BIN) main.c

.PHONY: clean
clean:
	rm -f $(BIN)
