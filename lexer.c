#include "lexer.h"
#include <string.h>
#include <ctype.h>

void lexer_init(Lexer *lexer, const char *source) {
    lexer->start = source;
    lexer->current = source;
    lexer->line = 1;
    lexer->column = 1;
}

static bool is_at_end(Lexer *lexer) {
    return *lexer->current == '\0';
}

static char advance(Lexer *lexer) {
    lexer->current++;
    lexer->column++;
    return lexer->current[-1];
}

static char peek(Lexer *lexer) {
    return *lexer->current;
}

static char peek_next(Lexer *lexer) {
    if (is_at_end(lexer)) return '\0';
    return lexer->current[1];
}

static bool match(Lexer *lexer, char expected) {
    if (is_at_end(lexer)) return false;
    if (*lexer->current != expected) return false;
    lexer->current++;
    lexer->column++;
    return true;
}

static Token make_token(Lexer *lexer, TokenType type) {
    Token token;
    token.type = type;
    token.start = lexer->start;
    token.length = (size_t)(lexer->current - lexer->start);
    token.line = lexer->line;
    // The column logic must account for the token's start column
    token.column = lexer->column - token.length;
    return token;
}

static Token error_token(Lexer *lexer, const char *message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = strlen(message);
    token.line = lexer->line;
    token.column = lexer->column;
    return token;
}

static void skip_whitespace(Lexer *lexer) {
    // Only internal use if needed, but we output SPACE/NEWLINE tokens
}

static Token number_of_equals(Lexer *lexer, int *equals_count) {
    *equals_count = 0;
    while (peek(lexer) == '=') {
        advance(lexer);
        (*equals_count)++;
    }
    return make_token(lexer, TOKEN_ERROR);
}

static Token bracket_content(Lexer *lexer, int equals_count, TokenType type) {
    while (!is_at_end(lexer)) {
        char c = peek(lexer);
        if (c == '\n') {
            lexer->line++;
            lexer->column = 1;
            lexer->current++;
        } else if (c == ']') {
            advance(lexer);
            int current_equals = 0;
            while (peek(lexer) == '=') {
                advance(lexer);
                current_equals++;
            }
            if (peek(lexer) == ']' && current_equals == equals_count) {
                advance(lexer); // consume closing bracket
                return make_token(lexer, type);
            }
        } else {
            advance(lexer);
        }
    }
    return error_token(lexer, "Unterminated bracket argument/comment.");
}

Token lexer_next_token(Lexer *lexer) {
    lexer->start = lexer->current;

    if (is_at_end(lexer)) return make_token(lexer, TOKEN_EOF);

    char c = advance(lexer);

    if (c == ' ' || c == '\t') {
        while (peek(lexer) == ' ' || peek(lexer) == '\t') {
            advance(lexer);
        }
        return make_token(lexer, TOKEN_SPACE);
    }

    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
        return make_token(lexer, TOKEN_NEWLINE);
    }
    
    // Windows CRLF
    if (c == '\r' && peek(lexer) == '\n') {
        advance(lexer);
        lexer->line++;
        lexer->column = 1;
        return make_token(lexer, TOKEN_NEWLINE);
    }

    if (c == '(') return make_token(lexer, TOKEN_LPAREN);
    if (c == ')') return make_token(lexer, TOKEN_RPAREN);

    if (c == '#') {
        // Line comment or bracket comment
        if (peek(lexer) == '[') {
            advance(lexer);
            int equals_count;
            number_of_equals(lexer, &equals_count);
            if (peek(lexer) == '[') {
                advance(lexer);
                return bracket_content(lexer, equals_count, TOKEN_BRACKET_COMMENT);
            }
            // fallback to line comment if it wasn't a valid bracket comment open?
            // Actually CMake says #[=[ is a bracket comment, but #[= without another [ is just a line comment.
             while (peek(lexer) != '\n' && !is_at_end(lexer)) {
                 advance(lexer);
             }
             return make_token(lexer, TOKEN_LINE_COMMENT);
        } else {
            while (peek(lexer) != '\n' && !is_at_end(lexer)) {
                advance(lexer);
            }
            return make_token(lexer, TOKEN_LINE_COMMENT);
        }
    }

    if (c == '"') {
        while (peek(lexer) != '"' && !is_at_end(lexer)) {
            if (peek(lexer) == '\\' && peek_next(lexer) == '"') {
                advance(lexer);
                advance(lexer);
            } else if (peek(lexer) == '\n') {
                lexer->line++;
                lexer->column = 1;
                advance(lexer);
            } else {
                advance(lexer);
            }
        }
        if (is_at_end(lexer)) return error_token(lexer, "Unterminated string.");
        advance(lexer); // close quote
        return make_token(lexer, TOKEN_QUOTED_ARGUMENT);
    }

    if (c == '[') {
        int equals_count;
        const char *fallback = lexer->current;
        int fallback_col = lexer->column;
        number_of_equals(lexer, &equals_count);
        if (peek(lexer) == '[') {
            advance(lexer);
            return bracket_content(lexer, equals_count, TOKEN_BRACKET_ARGUMENT);
        }
        // If it's not a bracket argument, it's just an unquoted argument
        lexer->current = fallback;
        lexer->column = fallback_col;
    }

    // Unquoted argument
    // Allowed chars in unquoted: anything except whitespace, (), #, ", \
    // Wait, \ can escape those. 
    while (!is_at_end(lexer)) {
        char p = peek(lexer);
        if (p == ' ' || p == '\t' || p == '\n' || p == '\r' || p == '(' || p == ')' || p == '#' || p == '"') {
            break;
        }
        if (p == '\\') {
            // escape sequence
            advance(lexer);
            if (!is_at_end(lexer)) {
                char esc = peek(lexer);
                if (esc == '\n') {
                    lexer->line++;
                    lexer->column = 1;
                }
                advance(lexer);
            }
        } else {
            advance(lexer);
        }
    }
    
    // Identifier check? Standard CMake identifier relies on context (first argument in a command is an identifier)
    // We'll leave it as UNQUOTED_ARGUMENT and let the parser decide if it's an IDENTIFIER.
    return make_token(lexer, TOKEN_UNQUOTED_ARGUMENT);
}
