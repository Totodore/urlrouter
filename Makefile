CC = gcc
CFLAGS = -std=c99 -Wpedantic -Wall -Wextra -g
.PHONY: clean all_tests

example: example.c urlrouter.o
	$(CC) -o example $^

all_tests: insert base_test tests/unittest

insert: tests/insert.c urlrouter.o
	$(CC) $(CFLAGS) -Wno-pointer-to-int-cast -Wno-int-conversion -I. -o tests/insert $^

base_test: tests/test.c urlrouter.o
	$(CC) $(CFLAGS) -Wno-pointer-to-int-cast -Wno-int-conversion -I. -o tests/test $^

tests/unittest: urlrouter.c
	$(CC) $(CFLAGS) -O3 -DURLROUTER_TEST -DURLROUTER_IO -DURLROUTER_ASSERT -o tests/unittest $^

urlrouter.o: urlrouter.c
	$(CC) $(CFLAGS) -DURLROUTER_IO -c -o urlrouter.o urlrouter.c

clean:
	rm -f *.o tests/test tests/unittest tests/find tests/insert
