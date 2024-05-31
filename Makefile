CC = gcc
CFLAGS = -std=c99 -Wpedantic -Wall -Wextra -g -O3

.PHONY: clean

test: urlrouter.h test.c
	$(CC) $(CFLAGS) -o test test.c

example: urlrouter.h example.c
	$(CC) -o example example.c

clean:
	rm -f test example