CC = clang
CFLAGS = -Wall -Wextra -Werror -Wpedantic -std=c11 -g -fsanitize=address

all: main

main: main.o parser.o tokeniser.o lib.o
	$(CC) $(CFLAGS) -o main main.o parser.o tokeniser.o lib.o

main.o: main.c parser.h tokeniser.h lib.h
	$(CC) $(CFLAGS) -c main.c

parser.o: parser.c parser.h tokeniser.h lib.h
	$(CC) $(CFLAGS) -c parser.c

tokeniser.o: tokeniser.c tokeniser.h lib.h
	$(CC) $(CFLAGS) -c tokeniser.c

lib.o: lib.c lib.h
	$(CC) $(CFLAGS) -c lib.c

rb.o: rb.c rb.h garb.h
	$(CC) $(CFLAGS) -c rb.c

garb_test: garb_test.c garb.o root_list.o long_table.o
	$(CC) $(CFLAGS) -o garb_test garb_test.c garb.o root_list.o long_table.o

root_list.o: root_list.c garb.h long_table.h
	$(CC) $(CFLAGS) -c root_list.c

long_table.o: long_table.c long_table.h
	$(CC) $(CFLAGS) -c long_table.c

garb.o: garb.c garb.h
	$(CC) $(CFLAGS) -c garb.c

clean:
	rm *.o main garb_test
