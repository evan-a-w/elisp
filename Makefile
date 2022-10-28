CC = clang
CFLAGS = -Wall -Wextra -Werror -Wpedantic -std=c11 -g

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

clean:
	rm *.o main
