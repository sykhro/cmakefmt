#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>

void config_init_defaults(CMakeFormatConfig *config) {
    config->IndentWidth = 2; // Default for CMake is often 2 spaces
    config->ColumnLimit = 80;
    config->UseTab = false;
    config->SpacesInParens = false;
    config->SpaceBeforeParens = false;
    config->AlignArguments = true;
    config->ClosingParensOnNewLine = false;
    config->KeepShortStatementOnSameLine = 0;
    config->AlwaysBreakAfterFirstArgument = false;
    config->BreakBeforeKeywordArgument = false;
    config->AlignOptions = false;
}

static char *trim_whitespace(char *str) {
    char *end;
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return str;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

static bool parse_bool(const char *val) {
    if (strcasecmp(val, "true") == 0 || strcasecmp(val, "yes") == 0 || strcasecmp(val, "1") == 0) return true;
    return false;
}

bool config_load_from_file(CMakeFormatConfig *config, const char *filepath) {
    FILE *f = fopen(filepath, "r");
    if (!f) return false;

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        char *trimmed = trim_whitespace(line);
        if (trimmed[0] == '#' || trimmed[0] == '\0') continue; // comment or empty block
        if (strncmp(trimmed, "---", 3) == 0 || strncmp(trimmed, "...", 3) == 0) continue; // YAML document boundaries

        char *colon = strchr(trimmed, ':');
        if (!colon) continue;

        *colon = '\0';
        char *key = trim_whitespace(trimmed);
        char *val = trim_whitespace(colon + 1);

        if (strcmp(key, "IndentWidth") == 0) {
            config->IndentWidth = atoi(val);
        } else if (strcmp(key, "ColumnLimit") == 0) {
            config->ColumnLimit = atoi(val);
        } else if (strcmp(key, "UseTab") == 0) {
            // Can be Never, Always, etc in clang-format, but let's do a simple check
            if (strcasecmp(val, "Always") == 0 || strcasecmp(val, "true") == 0) {
                config->UseTab = true;
            } else {
                config->UseTab = false;
            }
        } else if (strcmp(key, "SpacesInParens") == 0) {
            if (strcasecmp(val, "Never") == 0 || strcasecmp(val, "false") == 0) {
                config->SpacesInParens = false;
            } else {
                config->SpacesInParens = true;
            }
        } else if (strcmp(key, "SpaceBeforeParens") == 0) {
            if (strcasecmp(val, "Never") == 0 || strcasecmp(val, "false") == 0) {
                config->SpaceBeforeParens = false;
            } else {
                config->SpaceBeforeParens = true;
            }
        } else if (strcmp(key, "AlignArguments") == 0) { // Using a custom key roughly matching AlignOperands
            config->AlignArguments = parse_bool(val);
        } else if (strcmp(key, "AlignOperands") == 0) {
            if (strcasecmp(val, "DontAlign") == 0) config->AlignArguments = false;
            else config->AlignArguments = true;
        } else if (strcmp(key, "ClosingParensOnNewLine") == 0) {
            config->ClosingParensOnNewLine = parse_bool(val);
        } else if (strcmp(key, "AlwaysBreakAfterFirstArgument") == 0) {
            config->AlwaysBreakAfterFirstArgument = parse_bool(val);
        } else if (strcmp(key, "KeepShortStatementOnSameLine") == 0) {
            config->KeepShortStatementOnSameLine = atoi(val);
        } else if (strcmp(key, "BreakBeforeKeywordArgument") == 0) {
            config->BreakBeforeKeywordArgument = parse_bool(val);
        } else if (strcmp(key, "AlignOptions") == 0) {
            config->AlignOptions = parse_bool(val);
        }
    }


    fclose(f);
    return true;
}

void config_dump(const CMakeFormatConfig *config, FILE *out) {
    fprintf(out, "---\n");
    fprintf(out, "AlignArguments: %s\n", config->AlignArguments ? "true" : "false");
    fprintf(out, "AlignOptions: %s\n", config->AlignOptions ? "true" : "false");
    fprintf(out, "AlwaysBreakAfterFirstArgument: %s\n", config->AlwaysBreakAfterFirstArgument ? "true" : "false");
    fprintf(out, "BreakBeforeKeywordArgument: %s\n", config->BreakBeforeKeywordArgument ? "true" : "false");
    fprintf(out, "ClosingParensOnNewLine: %s\n", config->ClosingParensOnNewLine ? "true" : "false");
    fprintf(out, "ColumnLimit: %d\n", config->ColumnLimit);
    fprintf(out, "IndentWidth: %d\n", config->IndentWidth);
    fprintf(out, "KeepShortStatementOnSameLine: %d\n", config->KeepShortStatementOnSameLine);
    fprintf(out, "SpaceBeforeParens: %s\n", config->SpaceBeforeParens ? "true" : "false");
    fprintf(out, "SpacesInParens: %s\n", config->SpacesInParens ? "true" : "false");
    fprintf(out, "UseTab: %s\n", config->UseTab ? "true" : "false");
    fprintf(out, "...\n");
}
