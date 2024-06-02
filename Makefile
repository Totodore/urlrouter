CC = gcc
CFLAGS = -std=c99 -Wpedantic -Wall -Wextra -g -O3

.PHONY: clean

test: urlrouter.h tests/insert.c
	$(CC) $(CFLAGS) -Wno-pointer-to-int-cast -Wno-int-conversion -I. -o tests/insert tests/insert.c

example: urlrouter.h example.c
	$(CC) -o example example.c

clean:
	rm -f test example tests/insert