CC = gcc
CFLAGS = -std=c99 -Wpedantic -Wall -Wextra -g -O3

.PHONY: clean

test: tests/insert.c urlrouter.o
	$(CC) $(CFLAGS) -Wno-pointer-to-int-cast -Wno-int-conversion -I. -o tests/insert $^

unittest: urlrouter.c
	$(CC) $(CFLAGS) -DURLROUTER_TEST -DURLROUTER_IO -DURLROUTER_ASSERT -o unittest urlrouter.c

urlrouter.o: urlrouter.c
	$(CC) $(CFLAGS) -c -o urlrouter.o urlrouter.c

example: example.c urlrouter.o
	$(CC) -o example $^

clean:
	rm -f *.o test unittest example tests/insert
