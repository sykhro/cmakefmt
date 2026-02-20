#include "parser.h"
#include "config.h"
#include "formatter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        fprintf(stderr, "In-place CMake reformatter.\nUsage: %s <file> ...\n       %s --dump-config\n", argv[0], argv[0]);
        return (argc < 2) ? 1 : 0;
    }
    
    CMakeFormatConfig config;
    config_init_defaults(&config);
    config_load_from_file(&config, ".cmake_format");

    if (strcmp(argv[1], "--dump-config") == 0) {
        config_dump(&config, stdout);
        return 0;
    }
    
    for (int i = 1; i < argc; i++) {
        const char *filename = argv[i];
        
        FILE *f = fopen(filename, "rb");
        if (!f) {
            perror("fopen");
            continue;
        }
        
        fseek(f, 0, SEEK_END);
        long length = ftell(f);
        fseek(f, 0, SEEK_SET);
        
        char *source = malloc(length + 1);
        fread(source, 1, length, f);
        source[length] = '\0';
        fclose(f);
        
        ASTNode *ast = parse_cmake(source);
        
        FILE *out = fopen(filename, "wb");
        if (out) {
            format_ast(ast, &config, out);
            fclose(out);
        } else {
            perror("fopen write");
        }
        
        free_ast(ast);
        free(source);
    }
    
    return 0;
}
