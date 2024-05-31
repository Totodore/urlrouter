CC = gcc
CFLAGS = -std=c99 -Wpedantic -Wall -Wextra -g -O3

.PHONY: clean

build: urlrouter.h
	$(CC) $(CFLAGS) -o test test.c

clean:
	rm -f test