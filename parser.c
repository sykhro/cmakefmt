#include "parser.h"
#include <stdlib.h>
#include <stdio.h>

static ASTNode *create_node(NodeType type, Token token) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->type = type;
    node->token = token;
    return node;
}

static void add_child(ASTNode *parent, ASTNode *child) {
    if (parent->child_count == parent->child_capacity) {
        parent->child_capacity = parent->child_capacity == 0 ? 8 : parent->child_capacity * 2;
        parent->children = realloc(parent->children, parent->child_capacity * sizeof(ASTNode *));
    }
    parent->children[parent->child_count++] = child;
}

void free_ast(ASTNode *node) {
    if (!node) return;
    for (size_t i = 0; i < node->child_count; i++) {
        free_ast(node->children[i]);
    }
    free(node->children);
    free(node);
}

typedef struct {
    Lexer lexer;
    Token current;
    Token previous;
} Parser;

static void advance_parser(Parser *parser) {
    parser->previous = parser->current;
    parser->current = lexer_next_token(&parser->lexer);
}

static bool match_parser(Parser *parser, TokenType type) {
    if (parser->current.type == type) {
        advance_parser(parser);
        return true;
    }
    return false;
}

static void parse_arguments(Parser *parser, ASTNode *cmd_node, int paren_depth) {
    while (parser->current.type != TOKEN_EOF) {
        if (parser->current.type == TOKEN_RPAREN) {
            if (paren_depth > 0) {
                return; // Let the caller consume the RPAREN
            } else {
                // If depth is 0, this is the closing RPAREN for the command invocation
                add_child(cmd_node, create_node(NODE_RPAREN, parser->current));
                advance_parser(parser);
                return;
            }
        }

        if (parser->current.type == TOKEN_LPAREN) {
            add_child(cmd_node, create_node(NODE_LPAREN, parser->current));
            advance_parser(parser);
            parse_arguments(parser, cmd_node, paren_depth + 1);
            if (parser->current.type == TOKEN_RPAREN) {
                add_child(cmd_node, create_node(NODE_RPAREN, parser->current));
                advance_parser(parser);
            }
            continue;
        }

        NodeType type;
        switch (parser->current.type) {
            case TOKEN_UNQUOTED_ARGUMENT: type = NODE_UNQUOTED_ARGUMENT; break;
            case TOKEN_QUOTED_ARGUMENT: type = NODE_QUOTED_ARGUMENT; break;
            case TOKEN_BRACKET_ARGUMENT: type = NODE_BRACKET_ARGUMENT; break;
            case TOKEN_SPACE: type = NODE_SPACE; break;
            case TOKEN_NEWLINE: type = NODE_NEWLINE; break;
            case TOKEN_LINE_COMMENT: type = NODE_LINE_COMMENT; break;
            case TOKEN_BRACKET_COMMENT: type = NODE_BRACKET_COMMENT; break;
            default: type = NODE_UNQUOTED_ARGUMENT; break;
        }
        add_child(cmd_node, create_node(type, parser->current));
        advance_parser(parser);
    }
}

static ASTNode *parse_command_invocation(Parser *parser) {
    ASTNode *cmd_node = create_node(NODE_COMMAND_INVOCATION, parser->current);
    
    ASTNode *id_node = create_node(NODE_IDENTIFIER, parser->current);
    add_child(cmd_node, id_node);
    advance_parser(parser);

    while (parser->current.type == TOKEN_SPACE || parser->current.type == TOKEN_NEWLINE ||
           parser->current.type == TOKEN_LINE_COMMENT || parser->current.type == TOKEN_BRACKET_COMMENT) {
        NodeType type = NODE_SPACE;
        if (parser->current.type == TOKEN_NEWLINE) type = NODE_NEWLINE;
        if (parser->current.type == TOKEN_LINE_COMMENT) type = NODE_LINE_COMMENT;
        if (parser->current.type == TOKEN_BRACKET_COMMENT) type = NODE_BRACKET_COMMENT;
        
        add_child(cmd_node, create_node(type, parser->current));
        advance_parser(parser);
    }

    if (parser->current.type == TOKEN_LPAREN) {
        add_child(cmd_node, create_node(NODE_LPAREN, parser->current));
        advance_parser(parser);
    } else {
        return cmd_node;
    }

    parse_arguments(parser, cmd_node, 0);

    return cmd_node;
}

ASTNode *parse_cmake(const char *source) {
    Parser parser;
    lexer_init(&parser.lexer, source);
    advance_parser(&parser);

    ASTNode *file_node = create_node(NODE_FILE, (Token){0});

    while (parser.current.type != TOKEN_EOF) {
        if (parser.current.type == TOKEN_SPACE) {
            add_child(file_node, create_node(NODE_SPACE, parser.current));
            advance_parser(&parser);
        } else if (parser.current.type == TOKEN_NEWLINE) {
            add_child(file_node, create_node(NODE_NEWLINE, parser.current));
            advance_parser(&parser);
        } else if (parser.current.type == TOKEN_LINE_COMMENT) {
            add_child(file_node, create_node(NODE_LINE_COMMENT, parser.current));
            advance_parser(&parser);
        } else if (parser.current.type == TOKEN_BRACKET_COMMENT) {
            add_child(file_node, create_node(NODE_BRACKET_COMMENT, parser.current));
            advance_parser(&parser);
        } else if (parser.current.type == TOKEN_UNQUOTED_ARGUMENT) {
            // Unquoted argument at top level could be an identifier
            add_child(file_node, parse_command_invocation(&parser));
        } else {
            // Fallback: treat as unquoted?
            add_child(file_node, create_node(NODE_UNQUOTED_ARGUMENT, parser.current));
            advance_parser(&parser);
        }
    }

    return file_node;
}

void print_ast(ASTNode *node, int depth) {
    for (int i = 0; i < depth; i++) printf("  ");
    const char *names[] = {
        "NODE_FILE", "NODE_COMMAND_INVOCATION", "NODE_IDENTIFIER",
        "NODE_UNQUOTED_ARGUMENT", "NODE_QUOTED_ARGUMENT", "NODE_BRACKET_ARGUMENT",
        "NODE_LINE_COMMENT", "NODE_BRACKET_COMMENT", "NODE_SPACE", "NODE_NEWLINE",
        "NODE_LPAREN", "NODE_RPAREN"
    };
    printf("%s", names[node->type]);
    if (node->token.length > 0) {
        printf(" '%.*s'", (int)node->token.length, node->token.start);
    }
    printf("\n");
    for (size_t i = 0; i < node->child_count; i++) {
        print_ast(node->children[i], depth + 1);
    }
}
