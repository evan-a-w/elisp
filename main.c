#include <stdio.h>

#include "eval.h"

int main(int argc, char **argv) {
    gc_init();
    st_init();

    if (argc == 2) {
        FILE *f = fopen(argv[1], "r");
        if (f == NULL) {
            perror(argv[1]);
            return 1;
        }
        tokeniser_t *t = new_tokeniser(f);
        if (t == NULL) {
            fprintf(stderr, "Failed to create tokeniser\n");
            return 1;
        }
        ast_t *a = parse_one(t);
        print_ast(stdout, a);
        printf("\n");
        print_value(eval_ast(a)); 
        printf("\n");

        free_ast(a);
        free_tokeniser(t);
    } else {
        printf("Usage: %s <filename>\n", argv[0]);
    }

    st_destroy();
    gc_destroy();
}
