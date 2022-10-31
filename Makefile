CC = clang
CFLAGS = -Wall -Wextra -Werror -Wpedantic -std=c11 -g -fsanitize=address

all: garb_test.exe treap_test.exe parse_test.exe

eval.o: eval.c eval.h env.o
	$(CC) $(CFLAGS) -c eval.c

env.o: env.c env.h map.h list.h const_string.h roots.h
	$(CC) $(CFLAGS) -c env.c

parse_test.exe: parse_test.o ast.a
	$(CC) $(CFLAGS) -o parse_test.exe parse_test.o ast.a

parse_test.o: parse_test.c parser.h tokeniser.h garb.h
	$(CC) $(CFLAGS) -c parse_test.c

ast.a: parser.o tokeniser.o parser.h tokeniser.h
	ar rcs ast.a parser.o tokeniser.o

parser.o: parser.c parser.h tokeniser.h
	$(CC) $(CFLAGS) -c parser.c

tokeniser.o: tokeniser.c tokeniser.h
	$(CC) $(CFLAGS) -c tokeniser.c

treap_test.exe: treap.o treap_test.o gclib.a
	$(CC) $(CFLAGS) -o treap_test.exe treap.o gclib.a treap_test.o

treap_test.o: treap_test.c garb.h treap.h roots.h
	$(CC) $(CFLAGS) -c treap_test.c

map.o: map.c map.h garb.h roots.h list.h const_string.h api.h base_types.h
	$(CC) $(CFLAGS) -c map.c

list.o: list.h garb.h api.h roots.h base_types.h
	$(CC) $(CFLAGS) -c list.c

const_string.o: const_string.c const_string.h base_types.h
	$(CC) $(CFLAGS) -c const_string.c

treap.o: treap.c treap.h garb.h
	$(CC) $(CFLAGS) -c treap.c

garb_test.exe: garb_test.c gclib.a
	$(CC) $(CFLAGS) -o garb_test.exe garb_test.c gclib.a

gclib.a: garb.o long_table.o roots.o roots.h garb.h
	ar rcs gclib.a garb.o roots.o long_table.o

roots.o: roots.c garb.h long_table.h roots.h
	$(CC) $(CFLAGS) -c roots.c

long_table.o: long_table.c long_table.h
	$(CC) $(CFLAGS) -c long_table.c

garb.o: garb.c garb.h
	$(CC) $(CFLAGS) -c garb.c

clean:
	rm -f *.o *.a *.exe
