#include <stdio.h>

#include "tokeniser.h"
#include "parser.h"

int main(int argc, char **argv) {
    if (argc == 1) {
        printf("Usage: %s <filename> ...", argv[0]);
    }
    for (int i = 1; i < argc; i++) {
        FILE *f = fopen(argv[i], "r");
        printf("File %d:\n", i);
        if (f == NULL) {
            fprintf(stderr, "Could not open file %s\n", argv[i]);
        } else {
            tokeniser_t *t = new_tokeniser(f);
            ast_t *p = parse(t);
            if (p == NULL) {
                fprintf(stderr, "Could not parse file %s", argv[i]);
            } else {
                print_ast(stdout, p);
            }
            printf("\n------------------------\n");
            free_ast(p);
            free_tokeniser(t);
        }
    }

    return 0;
}
