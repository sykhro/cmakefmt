#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <stdio.h>

// Some basic ClangFormat-like keys we might want to support
typedef struct {
    int IndentWidth;
    int ColumnLimit;
    bool UseTab;
    bool SpacesInParens;
    bool SpaceBeforeParens;
    bool AlignArguments;
    bool ClosingParensOnNewLine;
    int KeepShortStatementOnSameLine;
    bool AlwaysBreakAfterFirstArgument;
    bool BreakBeforeKeywordArgument;
    bool AlignOptions;
} CMakeFormatConfig;

void config_init_defaults(CMakeFormatConfig *config);
bool config_load_from_file(CMakeFormatConfig *config, const char *filepath);
void config_dump(const CMakeFormatConfig *config, FILE *out);

#endif
