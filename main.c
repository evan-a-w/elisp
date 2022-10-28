#include <stdio.h>

#include "tokeniser.h"
#include "parser.h"

int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        FILE *f = fopen(argv[i], "r");
        printf("File %d: ", i);
        if (f == NULL) {
            fprintf(stderr, "Could not open file %s\n", argv[i]);
        } else {
            tokeniser_t *t = new_tokeniser(f);
            program_t *p = parse(t);
            if (p == NULL) {
                fprintf(stderr, "Could not parse file %s\n", argv[i]);
            } else {
                print_program(stdout, p);
            }
            printf("\n");
            free_program(p);
            free_tokeniser(t);
        }
    }

    return 0;
}
