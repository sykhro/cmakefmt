#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>
#include <stdbool.h>

typedef enum {
    TOKEN_ERROR,
    TOKEN_EOF,

    // Structural
    TOKEN_LPAREN,
    TOKEN_RPAREN,

    // Content
    TOKEN_IDENTIFIER,
    TOKEN_UNQUOTED_ARGUMENT,
    TOKEN_QUOTED_ARGUMENT,
    TOKEN_BRACKET_ARGUMENT,
    TOKEN_LINE_COMMENT,
    TOKEN_BRACKET_COMMENT,

    // Whitespace
    TOKEN_SPACE,
    TOKEN_NEWLINE,
} TokenType;

typedef struct {
    TokenType type;
    const char *start;
    size_t length;
    int line;
    int column;
} Token;

typedef struct {
    const char *start;
    const char *current;
    int line;
    int column;
} Lexer;

void lexer_init(Lexer *lexer, const char *source);
Token lexer_next_token(Lexer *lexer);

#endif
