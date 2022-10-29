#ifndef TOKENISER_H
#define TOKENISER_H

#include <stdio.h>
#include <stdbool.h>

typedef long long ll;

typedef struct tokeniser tokeniser_t;

typedef enum tok {
    Error,
    LParen,
    RParen,
    LBrace,
    RBrace,
    LBracket,
    RBracket,
    Ident,
    Int,
    Str,
    QuoteMark,
    None,
} tok_t;

typedef struct token {
    tok_t tok;
    union {
        char *s;
        ll i;
    };
    int line;
    int col;
} token_t;

void free_token(token_t t);
token_t next_token(tokeniser_t *tokeniser);
token_t peek_token(tokeniser_t *tokeniser);
void next_peek(tokeniser_t *t);
token_t *get_last(tokeniser_t *tokeniser);
token_t take_last_token(tokeniser_t *tokeniser);
void free_last_token(tokeniser_t *tokeniser);
tokeniser_t *new_tokeniser(FILE *f);
void free_tokeniser(tokeniser_t *t);
bool is_eof(tokeniser_t *t);
char *token_to_string(token_t *t);
void skip_whitespace(tokeniser_t *t);

#endif
