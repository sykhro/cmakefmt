#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

// AST Node Types
typedef enum {
    NODE_FILE,
    NODE_COMMAND_INVOCATION,
    NODE_IDENTIFIER,
    NODE_UNQUOTED_ARGUMENT,
    NODE_QUOTED_ARGUMENT,
    NODE_BRACKET_ARGUMENT,
    NODE_LINE_COMMENT,
    NODE_BRACKET_COMMENT,
    NODE_SPACE,
    NODE_NEWLINE,
    NODE_LPAREN,
    NODE_RPAREN,
} NodeType;

typedef struct ASTNode {
    NodeType type;
    Token token;
    struct ASTNode **children;
    size_t child_count;
    size_t child_capacity;
} ASTNode;

ASTNode *parse_cmake(const char *source);
void free_ast(ASTNode *node);
void print_ast(ASTNode *node, int depth);

#endif
