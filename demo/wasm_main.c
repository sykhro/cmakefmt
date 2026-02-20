#include "../parser.h"
#include "../config.h"
#include "../formatter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <emscripten.h>

static void write_to_file(const char *filename, const char *content) {
    FILE *f = fopen(filename, "wb");
    if (f) {
        fputs(content, f);
        fclose(f);
    }
}

static char* read_from_file(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) return NULL;
    
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *buffer = malloc(length + 1);
    if (buffer) {
        fread(buffer, 1, length, f);
        buffer[length] = '\0';
    }
    fclose(f);
    return buffer;
}

EMSCRIPTEN_KEEPALIVE
char* format_cmake_code(const char *source, const char *config_yaml) {
    write_to_file(".cmake_format", config_yaml);

    CMakeFormatConfig config;
    config_init_defaults(&config);
    config_load_from_file(&config, ".cmake_format");

    ASTNode *ast = parse_cmake(source);

    const char *out_filename = "formatted.cmake";
    FILE *out = fopen(out_filename, "wb");
    if (out) {
        format_ast(ast, &config, out);
        fclose(out);
    }

    free_ast(ast);

    return read_from_file(out_filename);
}

EMSCRIPTEN_KEEPALIVE
void free_string(char *ptr) {
    free(ptr);
}

EMSCRIPTEN_KEEPALIVE
char* get_default_config() {
    CMakeFormatConfig config;
    config_init_defaults(&config);
    
    // Write defaults to a temporary string using memory file if available, or just a file
    const char *tmp_filename = "default.cmake_format";
    FILE *out = fopen(tmp_filename, "wb");
    if (out) {
        config_dump(&config, out);
        fclose(out);
    }
    
    return read_from_file(tmp_filename);
}
