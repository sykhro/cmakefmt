#include "formatter.h"
#include <string.h>
#include <ctype.h>

static bool is_keyword(const char *str, size_t len) {
    if (len == 0) return false;
    for (size_t i = 0; i < len; i++) {
        if (str[i] >= 'a' && str[i] <= 'z') return false; 
    }
    return true; 
}

static bool is_cmake_keyword(const char *str, size_t len) {
    const char* keywords[] = {
        "PROPERTIES", "PROPERTY", "TARGET", "TARGETS", "DESTINATION", "COMMAND", 
        "DEPENDS", "WORKING_DIRECTORY", "COMMENT", "SOURCES", "PUBLIC", "PRIVATE", "INTERFACE", 
        "FILES", "PROGRAMS", "INCLUDES", "EXPORT", "ALIAS", "STRINGS", "DEFINED", "COMPONENTS", 
        "OPTIONAL", "REQUIRED", "APPEND", "ENV", "HINTS", "PATHS", "DOC", "VERSION", "LIBRARY", 
        "RUNTIME", "ARCHIVE", "FRAMEWORK", "BUNDLE", "NAMELINK_ONLY", "NAMELINK_SKIP", 
        "PERMISSIONS", "CONFIGURATIONS", "COMPONENT", "MATCHES", "EXISTS", "TEST", "POLICY",
        "CACHE", "FORCE", "FILEPATH", "PATH", "STRING", "INTERNAL", "BOOL", "MAIN_DEPENDENCY",
        "IMPLICIT_DEPENDS", "DEPFILE", "JOB_POOL", "VERBATIM", "COMMAND_EXPAND_LISTS",
        "APPEND_STRING", "GLOBAL", "DIRECTORY", "SOURCE", "INSTALL", "BRIEF_DOCS", "FULL_DOCS",
        "VARS", "ARGS", "PULL", "PUSH", "MACROS", "NAMES", "LANGUAGES", "CONFIGS"
    };
    for (size_t i = 0; i < sizeof(keywords)/sizeof(keywords[0]); i++) {
        if (strlen(keywords[i]) == len && strncmp(str, keywords[i], len) == 0) return true;
    }
    return false;
}

typedef struct {
    int indent_level;
    const CMakeFormatConfig *config;
    FILE *out;
    bool needs_indent;
    int arg_indent;
    int align_opts_max_arg1;
    int align_opts_max_arg2;
} FormatterState;

static void print_indent(FormatterState *state, int extra) {
    if (state->needs_indent) {
        int total = state->indent_level * state->config->IndentWidth + extra;
        if (state->config->UseTab) {
            int tabs = total / state->config->IndentWidth;
            int spaces = total % state->config->IndentWidth;
            for (int i = 0; i < tabs; i++) fputc('\t', state->out);
            for (int i = 0; i < spaces; i++) fputc(' ', state->out);
        } else {
            for (int i = 0; i < total; i++) fputc(' ', state->out);
        }
        state->needs_indent = false;
    }
}

static void increase_indent(FormatterState *state, const char *name, size_t len) {
    if (len == 2 && strncasecmp(name, "if", 2) == 0) state->indent_level++;
    else if (len == 5 && strncasecmp(name, "while", 5) == 0) state->indent_level++;
    else if (len == 7 && strncasecmp(name, "foreach", 7) == 0) state->indent_level++;
    else if (len == 8 && strncasecmp(name, "function", 8) == 0) state->indent_level++;
    else if (len == 5 && strncasecmp(name, "macro", 5) == 0) state->indent_level++;
    else if (len == 5 && strncasecmp(name, "block", 5) == 0) state->indent_level++;
}

static void decrease_indent(FormatterState *state, const char *name, size_t len) {
    if ((len == 5 && strncasecmp(name, "endif", 5) == 0) ||
        (len == 8 && strncasecmp(name, "endwhile", 8) == 0) ||
        (len == 10 && strncasecmp(name, "endforeach", 10) == 0) ||
        (len == 11 && strncasecmp(name, "endfunction", 11) == 0) ||
        (len == 8 && strncasecmp(name, "endmacro", 8) == 0) ||
        (len == 8 && strncasecmp(name, "endblock", 8) == 0)) {
        if (state->indent_level > 0) state->indent_level--;
    }
}

static void tweak_indent_for_command(FormatterState *state, const char *name, size_t len, int *temp_indent) {
    *temp_indent = state->indent_level;
    decrease_indent(state, name, len); // permanent decrease for ends
    // Temporary decrease for else/elseif
    if ((len == 4 && strncasecmp(name, "else", 4) == 0) ||
        (len == 6 && strncasecmp(name, "elseif", 6) == 0)) {
        *temp_indent = state->indent_level > 0 ? state->indent_level - 1 : 0;
    } else {
        *temp_indent = state->indent_level;
    }
}

static int get_single_line_length(ASTNode *node, FormatterState *state, int indent) {
    int len = indent * state->config->IndentWidth;
    bool need_space = false;
    bool first_in_parens = false;
    bool inside_parens = false;
    for (size_t i = 0; i < node->child_count; i++) {
        ASTNode *child = node->children[i];
        if (child->type == NODE_IDENTIFIER) {
            len += child->token.length;
        } else if (child->type == NODE_LPAREN) {
            if (state->config->SpaceBeforeParens && !inside_parens) len++;
            len++;
            inside_parens = true;
            first_in_parens = true;
            if (state->config->SpacesInParens) len++;
            need_space = false;
        } else if (child->type == NODE_RPAREN) {
            if (state->config->SpacesInParens && !first_in_parens) len++;
            len++;
            inside_parens = false;
            first_in_parens = false;
        } else if (child->type == NODE_SPACE || child->type == NODE_NEWLINE) {
           if (inside_parens) need_space = true;
        } else if (child->type == NODE_LINE_COMMENT || child->type == NODE_BRACKET_COMMENT) {
             return 999999; 
        } else {
             if (!first_in_parens && need_space) len++;
             len += child->token.length;
             need_space = true;
             first_in_parens = false;
        }
    }
    return len;
}

static void calculate_option_alignment(ASTNode *root, size_t start_idx, FormatterState *state) {
    state->align_opts_max_arg1 = 0;
    state->align_opts_max_arg2 = 0;
    
    size_t i = start_idx;
    int blank_lines = 0;
    while (i < root->child_count) {
        ASTNode *child = root->children[i];
        if (child->type == NODE_NEWLINE) {
            blank_lines++;
            if (blank_lines > 1) break; // Break group on blank line
        } else if (child->type == NODE_SPACE) {
            // ignore
        } else if (child->type == NODE_LINE_COMMENT || child->type == NODE_BRACKET_COMMENT) {
            blank_lines = 0;
        } else if (child->type == NODE_COMMAND_INVOCATION) {
            blank_lines = 0;
            ASTNode *cmd_id = NULL;
            for (size_t c = 0; c < child->child_count; c++) {
                if (child->children[c]->type == NODE_IDENTIFIER) {
                    cmd_id = child->children[c];
                    break;
                }
            }
            if (!cmd_id || cmd_id->token.length != 6 || strncasecmp(cmd_id->token.start, "option", 6) != 0) {
                break; // Not an option command
            }
            
            int arg_idx = 0;
            int arg1_len = 0;
            int arg2_len = 0;
            for (size_t c = 0; c < child->child_count; c++) {
                ASTNode *arg_node = child->children[c];
                if (arg_node->type == NODE_UNQUOTED_ARGUMENT || arg_node->type == NODE_QUOTED_ARGUMENT || arg_node->type == NODE_BRACKET_ARGUMENT) {
                    if (arg_idx == 0) arg1_len = arg_node->token.length;
                    else if (arg_idx == 1) arg2_len = arg_node->token.length;
                    arg_idx++;
                }
            }
            if (arg1_len > state->align_opts_max_arg1) state->align_opts_max_arg1 = arg1_len;
            if (arg2_len > state->align_opts_max_arg2) state->align_opts_max_arg2 = arg2_len;
        } else {
            break;
        }
        i++;
    }
}

static void format_command_invocation(FormatterState *state, ASTNode *node) {
    const char *cmd_name = "";
    size_t cmd_len = 0;
    
    // Find identifier
    for (size_t i = 0; i < node->child_count; i++) {
        if (node->children[i]->type == NODE_IDENTIFIER) {
            cmd_name = node->children[i]->token.start;
            cmd_len = node->children[i]->token.length;
            break;
        }
    }

    int print_indent_level = state->indent_level;
    tweak_indent_for_command(state, cmd_name, cmd_len, &print_indent_level);
    
    // Print indent
    if (state->needs_indent) {
        int total = print_indent_level * state->config->IndentWidth;
        if (state->config->UseTab) {
            int tabs = total / state->config->IndentWidth;
            int spaces = total % state->config->IndentWidth;
            for (int i = 0; i < tabs; i++) fputc('\t', state->out);
            for (int i = 0; i < spaces; i++) fputc(' ', state->out);
        } else {
            for (int i = 0; i < total; i++) fputc(' ', state->out);
        }
        state->needs_indent = false;
    }

    int single_line_len = get_single_line_length(node, state, print_indent_level);
    bool force_single_line = (state->config->KeepShortStatementOnSameLine > 0 && single_line_len <= state->config->KeepShortStatementOnSameLine);

    bool has_newlines = false;
    for (size_t i = 0; i < node->child_count; i++) {
        if (node->children[i]->type == NODE_NEWLINE) { has_newlines = true; break; }
    }

    int positional_arg_count = 0;
    int total_arg_count = 0;
    bool emitted_internal_newline = false;

    // Now format children
    bool inside_parens = false;
    bool need_space = false;
    bool first_in_parens = false;
    for (size_t i = 0; i < node->child_count; i++) {
        ASTNode *child = node->children[i];
        
        if (child->type == NODE_IDENTIFIER) {
            fprintf(state->out, "%.*s", (int)child->token.length, child->token.start);
            state->arg_indent = (print_indent_level * state->config->IndentWidth) + child->token.length + 1; 
        } else if (child->type == NODE_LPAREN) {
            if (state->config->SpaceBeforeParens && !inside_parens) {
                fputc(' ', state->out);
                state->arg_indent++;
            }
            fputc('(', state->out);
            inside_parens = true;
            first_in_parens = true;
            if (state->config->SpacesInParens) {
                fputc(' ', state->out);
            }
            need_space = false;
        } else if (child->type == NODE_RPAREN) {
            bool should_put_newline = state->config->ClosingParensOnNewLine && !force_single_line && emitted_internal_newline;
            if (should_put_newline) {
                if (!state->needs_indent) {
                    fputc('\n', state->out);
                    state->needs_indent = true;
                }
            } else if (state->config->SpacesInParens && !first_in_parens && !state->needs_indent) { 
                 fputc(' ', state->out);
            }
            
            if (state->needs_indent) {
                int total = print_indent_level * state->config->IndentWidth;
                if (state->config->UseTab) {
                    int tabs = total / state->config->IndentWidth;
                    int spaces = total % state->config->IndentWidth;
                    for (int s = 0; s < tabs; s++) fputc('\t', state->out);
                    for (int s = 0; s < spaces; s++) fputc(' ', state->out);
                } else {
                    for (int s = 0; s < total; s++) fputc(' ', state->out);
                }
                state->needs_indent = false;
            }
            fputc(')', state->out);
            inside_parens = false;
            first_in_parens = false;
        } else if (child->type == NODE_SPACE) {
            if (inside_parens && force_single_line) need_space = true;
        } else if (child->type == NODE_NEWLINE) {
            bool is_before_closing = false;
            if (inside_parens) {
                bool is_last_newline = true;
                for (size_t j = i + 1; j < node->child_count; j++) {
                    if (node->children[j]->type == NODE_RPAREN) break;
                    if (node->children[j]->type != NODE_SPACE && node->children[j]->type != NODE_NEWLINE) {
                        is_last_newline = false;
                        break;
                    }
                }
                if (is_last_newline) {
                    bool prev_is_line_comment = false;
                    for (int j = (int)i - 1; j >= 0; j--) {
                        if (node->children[j]->type == NODE_SPACE) continue;
                        if (node->children[j]->type == NODE_LINE_COMMENT) prev_is_line_comment = true;
                        break;
                    }
                    if (!prev_is_line_comment) {
                        if (!state->config->ClosingParensOnNewLine) {
                            is_before_closing = true;
                        } else if (!emitted_internal_newline) {
                            is_before_closing = true;
                        }
                    }
                }
            }
            if (force_single_line || is_before_closing) {
                if (inside_parens) need_space = true;
            } else {
                fputc('\n', state->out);
                state->needs_indent = true;
                need_space = false;
                first_in_parens = false; // first line might be empty
                if (inside_parens && !is_before_closing) {
                    emitted_internal_newline = true;
                }
            }
        } else if (child->type == NODE_LINE_COMMENT || child->type == NODE_BRACKET_COMMENT) {
            if (!first_in_parens && need_space) { fputc(' ', state->out); need_space = false; }
            if (state->needs_indent) {
                int extra = inside_parens && !state->config->AlignArguments ? state->config->IndentWidth : 0;
                if (inside_parens && state->config->AlignArguments) {
                    for (int s = 0; s < state->arg_indent; s++) fputc(' ', state->out);
                } else {
                    int total = print_indent_level * state->config->IndentWidth + extra;
                    if (state->config->UseTab) {
                        int tabs = total / state->config->IndentWidth;
                        int spaces = total % state->config->IndentWidth;
                        for (int s = 0; s < tabs; s++) fputc('\t', state->out);
                        for (int s = 0; s < spaces; s++) fputc(' ', state->out);
                    } else {
                        for (int s = 0; s < total; s++) fputc(' ', state->out);
                    }
                }
                state->needs_indent = false;
            }
            fprintf(state->out, "%.*s", (int)child->token.length, child->token.start);
            if (child->type == NODE_BRACKET_COMMENT) need_space = true; 
            first_in_parens = false;
        } else {
            // Arguments
            total_arg_count++;
            bool is_kw = is_keyword(child->token.start, child->token.length);
            if (!is_kw) positional_arg_count++;

            bool break_for_keyword = state->config->BreakBeforeKeywordArgument &&
                                     is_cmake_keyword(child->token.start, child->token.length);

            if (!force_single_line && !state->needs_indent && !first_in_parens) {
                if ((has_newlines && state->config->AlwaysBreakAfterFirstArgument && positional_arg_count == 2) || 
                    break_for_keyword) {
                    fputc('\n', state->out);
                    state->needs_indent = true;
                    need_space = false;
                    emitted_internal_newline = true;
                }
            }

            if (state->needs_indent) {
                int extra = inside_parens && !state->config->AlignArguments ? state->config->IndentWidth : 0;
                if (inside_parens && state->config->AlignArguments) {
                     for (int s = 0; s < state->arg_indent; s++) fputc(' ', state->out);
                } else {
                    int total = print_indent_level * state->config->IndentWidth + extra;
                    if (state->config->UseTab) {
                        int tabs = total / state->config->IndentWidth;
                        int spaces = total % state->config->IndentWidth;
                        for (int s = 0; s < tabs; s++) fputc('\t', state->out);
                        for (int s = 0; s < spaces; s++) fputc(' ', state->out);
                    } else {
                        for (int s = 0; s < total; s++) fputc(' ', state->out);
                    }
                }
                state->needs_indent = false;
            } else if (!first_in_parens && need_space) {
                fputc(' ', state->out);
            }
            fprintf(state->out, "%.*s", (int)child->token.length, child->token.start);

            if (state->config->AlignOptions && cmd_len == 6 && strncasecmp(cmd_name, "option", 6) == 0) {
                int pad = 0;
                if (total_arg_count == 1) {
                     pad = state->align_opts_max_arg1 - child->token.length;
                } else if (total_arg_count == 2) {
                     pad = state->align_opts_max_arg2 - child->token.length;
                }
                for (int p = 0; p < pad; p++) fputc(' ', state->out);
            }

            need_space = true;
            first_in_parens = false;
        }
    }

    increase_indent(state, cmd_name, cmd_len);
}

void format_ast(ASTNode *root, const CMakeFormatConfig *config, FILE *out) {
    FormatterState state = {0};
    state.config = config;
    state.out = out;
    state.needs_indent = true;

    int pending_newlines = 0;
    bool has_content = false;

    for (size_t i = 0; i < root->child_count; i++) {
        ASTNode *child = root->children[i];
        
        if (child->type == NODE_SPACE) {
            continue;
        } else if (child->type == NODE_NEWLINE) {
            pending_newlines++;
        } else {
            // output pending newlines before this token
            if (has_content) {
                int to_print = pending_newlines > 2 ? 2 : pending_newlines;
                for (int n = 0; n < to_print; n++) {
                    fputc('\n', state.out);
                }
                if (to_print > 0) state.needs_indent = true;
            }
            pending_newlines = 0;
            has_content = true;

            if (child->type == NODE_COMMAND_INVOCATION) {
                ASTNode *cmd_id = NULL;
                for (size_t c = 0; c < child->child_count; c++) {
                    if (child->children[c]->type == NODE_IDENTIFIER) {
                        cmd_id = child->children[c];
                        break;
                    }
                }
                bool is_option = cmd_id && cmd_id->token.length == 6 && strncasecmp(cmd_id->token.start, "option", 6) == 0;
                
                if (state.config->AlignOptions && is_option) {
                    if (state.align_opts_max_arg1 == 0) {
                        calculate_option_alignment(root, i, &state);
                    }
                } else {
                    state.align_opts_max_arg1 = 0;
                    state.align_opts_max_arg2 = 0;
                }

                format_command_invocation(&state, child);
            } else if (child->type == NODE_LINE_COMMENT || child->type == NODE_BRACKET_COMMENT) {
                print_indent(&state, 0);
                fprintf(state.out, "%.*s", (int)child->token.length, child->token.start);
            } else {
                state.align_opts_max_arg1 = 0;
                state.align_opts_max_arg2 = 0;
                print_indent(&state, 0);
                fprintf(state.out, "%.*s", (int)child->token.length, child->token.start);
            }
        }
    }

    if (has_content) {
        fputc('\n', state.out);
    }
}
