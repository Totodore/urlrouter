CC = gcc
WARNINGS = -Wpedantic -Wall -Wextra
CFLAGS = -std=c99 $(WARNINGS) -g

.PHONY: clean

build:
	$(CC) $(CFLAGS) -o test test.c

clean:
	rm -f test