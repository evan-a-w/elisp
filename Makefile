CC = clang
CFLAGS = -Wall -Wextra -Werror -Wpedantic -std=c11 -g -fsanitize=address

all: garb_test treap_test

main: main.o parser.o tokeniser.o garb.o
	$(CC) $(CFLAGS) -o main main.o parser.o tokeniser.o lib.o

main.o: main.c parser.h tokeniser.h garb.h
	$(CC) $(CFLAGS) -c main.c

parser.o: parser.c parser.h tokeniser.h
	$(CC) $(CFLAGS) -c parser.c

tokeniser.o: tokeniser.c tokeniser.h
	$(CC) $(CFLAGS) -c tokeniser.c

treap_test: treap.o treap_test.o gclib.a
	$(CC) $(CFLAGS) -o treap_test treap.o gclib.a treap_test.o

treap_test.o: treap_test.c garb.h treap.h roots.h
	$(CC) $(CFLAGS) -c treap_test.c

treap.o: treap.c treap.h garb.h
	$(CC) $(CFLAGS) -c treap.c

garb_test: garb_test.c gclib.a
	$(CC) $(CFLAGS) -o garb_test garb_test.c gclib.a

gclib.a: garb.o long_table.o roots.o roots.h garb.h
	ar rcs gclib.a garb.o roots.o long_table.o

roots.o: roots.c garb.h long_table.h roots.h
	$(CC) $(CFLAGS) -c roots.c

long_table.o: long_table.c long_table.h
	$(CC) $(CFLAGS) -c long_table.c

garb.o: garb.c garb.h
	$(CC) $(CFLAGS) -c garb.c

clean:
	rm -f *.o main treap_test garb_test *.a
