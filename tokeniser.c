#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "tokeniser.h"
#include "lib.h"

struct tokeniser {
    char *buf;
    int line;
    int col;
    int pos;
    token_t last_token;
};

void free_token(token_t t) {
    switch (t.tok) {
    case Ident:
    case Error:
    case Str:
        free(t.s);
        break;
    default:
        break;
    }
}

bool is_ident_start(char c) {
    char *others = "_-+*/%&|!^~?:'><=";
    for (unsigned i = 0; i < strlen(others); i++) {
        if (c == others[i]) {
            return true;
        }
    }
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool not_quote(char c) {
    return c != '"' || c == '\0';
}

bool is_ident(char c) {
    return is_ident_start(c) || (c >= '0' && c <= '9');
}

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

char *read_while(tokeniser_t *t, bool (*pred)(char)) {
    int start = t->pos;
    while (pred(t->buf[t->pos])) {
        if (t->buf[t->pos] == '\n') {
            t->line++;
            t->col = 1;
        } else {
            t->col++;
        }
        t->pos++;
    }
    int len = t->pos - start;
    char *s = malloc(len + 1);
    memcpy(s, t->buf + start, len);
    s[len] = '\0';
    return s;
}

token_t next_token(tokeniser_t *t) {
    if (t->last_token.tok != None) {
        token_t temp =  t->last_token;
        t->last_token.tok = None;
        return temp;
    }
    
    int col = t->col;
    int line = t->line;
    char *buf = t->buf;

    while (buf[t->pos] != '\0') {
        switch (buf[t->pos]) {
        case '\0':
        {
            char *e = allocate_string("Unexpected end of file");
            return (token_t) { .tok = Error, .s = e, .line = line, .col = col };
        }
        case ' ':
        case '\t':
        case '\r':
            col = t->col++;
            t->pos++;
            break;
        case '\n':
            t->pos++;
            line = t->line++;
            col = t->col = 1;
            break;
        case '"':
        {
            t->pos++;
            t->col++;
            char *s = read_while(t, not_quote);
            if (buf[t->pos] == '"') {
                t->pos++;
                t->col++;
                return (token_t) { .tok = Str, .s = s, .line = line, .col = col };
            } else {
                free(s);
                char *e = allocate_string("Unterminated string");
                return (token_t) { .tok = Error, .s = e, .line = line, .col = col};
            }
        }
        case '1': case '2': case '3': case '4': case '5':
        case '6': case '7': case '8': case '9':
        {
            char *buf = read_while(t, is_digit);
            ll i;
            sscanf(buf, "%lld", &i);
            free(buf);
            return (token_t) { .tok = Int, .i = i, .line = line, .col = col };
        }
        case '(':
            t->pos++;
            t->col++;
            return (token_t) { .tok = LParen, .line = line, .col = col };
        case ')':
            t->pos++;
            t->col++;
            return (token_t) { .tok = RParen, .line = line, .col = col };
        case '{':
            t->pos++;
            t->col++;
            return (token_t) { .tok = LBrace, .line = line, .col = col };
        case '}':
            t->pos++;
            t->col++;
            return (token_t) { .tok = RBrace, .line = line, .col = col };
        case '[':
            t->pos++;
            return (token_t) { .tok = LBracket, .line = line, .col = col };
        case ']':
            t->pos++;
            t->col++;
            return (token_t) { .tok = RBracket, .line = line, .col = col };
        default:
            if (is_ident_start(buf[t->pos])) {
                char *buf = read_while(t, is_ident);
                return (token_t) { .tok = Ident, .s = buf, .line = line, .col = col };
            }
            char *e = malloc(400);
            sprintf(e, "Unexpected character '%c' at line %d, col %d\n", buf[t->pos], line, col);
            return (token_t) { .tok = Error, .s = e, .line = line, .col = col };
        }
    }

    return (token_t) { .tok = None, .line = line, .col = col };
}

void next_peek(tokeniser_t *t) {
    if (t->last_token.tok == None) {
        t->last_token = next_token(t);
    }
}

token_t peek_token(tokeniser_t *t) {
    next_peek(t);
    return t->last_token;
}

token_t *get_last(tokeniser_t *tokeniser) {
    return &tokeniser->last_token;
}

token_t take_last_token(tokeniser_t *tokeniser) {
    token_t temp = tokeniser->last_token;
    tokeniser->last_token.tok = None;
    return temp;
}

void free_last_token(tokeniser_t *tokeniser) {
    free_token(take_last_token(tokeniser));
}

tokeniser_t *new_tokeniser(FILE *f) {
    fseek(f, 0, SEEK_END);
    int len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(len + 1);
    fread(buf, 1, len, f);
    buf[len] = '\0';
    tokeniser_t *t = malloc(sizeof(tokeniser_t));
    *t = (tokeniser_t) { .buf = buf, .line = 1, .col = 1, .pos = 0, .last_token = { .tok = None } };
    return t;
}

void free_tokeniser(tokeniser_t *t) {
    free(t->buf);
    free_last_token(t);
}

bool is_eof(tokeniser_t *t) {
    skip_whitespace(t);
    return t->buf[t->pos] == '\0';
}

char *token_to_string(token_t *t) {
    char *fst;
    switch (t->tok) {
    case None:
        fst = "None";
        break;
    case Error:
    case Str:
    case Ident:
        fst = t->s;
        break;
    case Int:
        fst = malloc(100);
        *fst = 0;
        sprintf(fst, "%lld", t->i);
        break;
    case LParen:
        fst = "(";
        break;
    case RParen:
        fst = ")";
        break;
    case LBrace:
        fst = "{";
        break;
    case RBrace:
        fst = "}";
        break;
    case LBracket:
        fst = "[";
        break;
    case RBracket:
        fst = "]";
        break;
    default:
        fst = "unknown";
    }

    char *buf = malloc(100);
    *buf = 0;
    sprintf(buf, " at line %d, col %d", t->line, t->col);
    char *res = malloc(strlen(fst) + strlen(buf) + 1);
    strcpy(res, fst);
    strcat(res, buf);
    free(buf);
    if (t->tok == Int)
        free(fst);
    return res;
}

void skip_whitespace(tokeniser_t *t) {
    for (;;) {
        switch (t->buf[t->pos]) {
        case ' ':
        case '\t':
        case '\r':
            t->pos++;
            t->col++;
            break;
        case '\n':
            t->pos++;
            t->line++;
            t->col = 1;
            break;
        default:
            return;
        }
    }
}
