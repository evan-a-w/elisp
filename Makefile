CC = clang
CFLAGS = -Wall -Wextra -Werror -Wpedantic -std=c11 -g -fsanitize=address

main: main.o parser.o tokeniser.o garb.o rb.o
	$(CC) $(CFLAGS) -o main main.o parser.o tokeniser.o lib.o

main.o: main.c parser.h tokeniser.h garb.h
	$(CC) $(CFLAGS) -c main.c

parser.o: parser.c parser.h tokeniser.h
	$(CC) $(CFLAGS) -c parser.c

tokeniser.o: tokeniser.c tokeniser.h
	$(CC) $(CFLAGS) -c tokeniser.c

treap_test: treap.o garb.o treap_test.o root_list.o long_table.o
	$(CC) $(CFLAGS) -o treap_test treap.o garb.o treap_test.o root_list.o long_table.o

treap_test.o: treap_test.c
	$(CC) $(CFLAGS) -c treap_test.c

treap.o: treap.c treap.h garb.h
	$(CC) $(CFLAGS) -c treap.c

garb_test: garb_test.c garb.o root_list.o long_table.o
	$(CC) $(CFLAGS) -o garb_test garb_test.c garb.o root_list.o long_table.o

root_list.o: root_list.c garb.h long_table.h
	$(CC) $(CFLAGS) -c root_list.c

long_table.o: long_table.c long_table.h
	$(CC) $(CFLAGS) -c long_table.c

garb.o: garb.c garb.h
	$(CC) $(CFLAGS) -c garb.c

list.o: list.c list.h garb.h
	$(CC) $(CFLAGS) -c list.c

clean:
	rm -f *.o main treap_test garb_test
