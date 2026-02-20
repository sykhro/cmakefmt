#ifndef FORMATTER_H
#define FORMATTER_H

#include "parser.h"
#include "config.h"
#include <stdio.h>

void format_ast(ASTNode *root, const CMakeFormatConfig *config, FILE *out);

#endif
