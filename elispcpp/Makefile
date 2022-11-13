CC = clang
CPC = clang++
CPPFLAGS = -Wall -Wextra -Werror -Wpedantic -std=c++2a -g -fsanitize=address
CFLAGS = -Wall -Wextra -Werror -Wpedantic -std=c11 -g -fsanitize=address

stdlib.o: stdlib.c stdlib.h garb.h roots.h env.h
	$(CC) $(CFLAGS) -c stdlib.c

parse.o: parser.c parser.h tokeniser.c tokeniser.h
	$(CC) $(CFLAGS) -c parser.c tokeniser.c

env.o: env.cpp env.h
	$(CPC) $(CPPFLAGS) -c env.cpp

roots.o: roots.cpp roots.h garb.h
	$(CPC) $(CPPFLAGS) -c roots.cpp

garb.o: garb.cpp garb.h
	$(CPC) $(CPPFLAGS) -c garb.cpp
