#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "lib.h"

char *allocate_string(char *s) {
    int len = strlen(s);
    char *new_s = malloc(len + 1);
    strcpy(new_s, s);
    return new_s;
}
