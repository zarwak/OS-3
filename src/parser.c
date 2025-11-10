#include "shell.h"

// Helper function to check if a character is a quote
int is_quote(char c) {
    return c == '\'' || c == '"';
}

// Improved parse function that handles quotes
int parse_redirection_pipes(char* cmdline, pipeline_t* pipeline) {
    if (cmdline == NULL || pipeline == NULL) {
        return -1;
    }

    // Expand variables in the command line
    char* expanded_cmdline = expand_variables(cmdline);
    if (expanded_cmdline == NULL) {
        return -1;
    }

    // Initialize pipeline
    pipeline->num_commands = 0;
    for (int i = 0; i < MAX_PIPES; i++) {
        pipeline->commands[i].input_file = NULL;
        pipeline->commands[i].output_file = NULL;
        pipeline->commands[i].background = 0;
        for (int j = 0; j < MAXARGS; j++) {
            pipeline->commands[i].args[j] = NULL;
        }
    }

    char* tokens[MAX_LEN];
    int token_count = 0;
    char* current = expanded_cmdline;
    
    // Improved tokenization that handles quotes
    while (*current != '\0' && token_count < MAX_LEN - 1) {
        // Skip leading whitespace
        while (*current == ' ' || *current == '\t') current++;
        
        if (*current == '\0') break;
        
        // Handle quoted strings
        if (is_quote(*current)) {
            char quote = *current;
            char* start = ++current; // Skip opening quote
            
            // Find closing quote
            while (*current != '\0' && *current != quote) {
                current++;
            }
            
            if (*current == quote) {
                tokens[token_count] = malloc(current - start + 1);
                strncpy(tokens[token_count], start, current - start);
                tokens[token_count][current - start] = '\0';
                token_count++;
                current++; // Skip closing quote
            } else {
                // Unclosed quote - use rest of string
                tokens[token_count] = strdup(start);
                token_count++;
                break;
            }
        } 
        // Handle redirection and pipe operators
        else if (*current == '<' || *current == '>' || *current == '|' || *current == '&' || *current == ';') {
            tokens[token_count] = malloc(2);
            tokens[token_count][0] = *current;
            tokens[token_count][1] = '\0';
            token_count++;
            current++;
        }
        // Handle regular tokens
        else {
            char* start = current;
            while (*current != '\0' && *current != ' ' && *current != '\t' && 
                   *current != '<' && *current != '>' && *current != '|' && 
                   *current != '&' && *current != ';' && !is_quote(*current)) {
                current++;
            }
            
            if (current > start) {
                tokens[token_count] = malloc(current - start + 1);
                strncpy(tokens[token_count], start, current - start);
                tokens[token_count][current - start] = '\0';
                token_count++;
            }
        }
    }
    
    tokens[token_count] = NULL;

    // Free expanded command line after tokenization
    free(expanded_cmdline);

    if (token_count == 0) {
        return -1; // Empty command
    }

    int cmd_index = 0;
    int arg_index = 0;
    int i = 0;

    while (i < token_count) {
        // Check for pipe symbol
        if (strcmp(tokens[i], "|") == 0) {
            pipeline->commands[cmd_index].args[arg_index] = NULL;
            cmd_index++;
            arg_index = 0;
            i++;
            continue;
        }
        
        // Check for command separator (semicolon)
        else if (strcmp(tokens[i], ";") == 0) {
            pipeline->commands[cmd_index].args[arg_index] = NULL;
            cmd_index++;
            arg_index = 0;
            i++;
            continue;
        }
        
        // Check for input redirection
        else if (strcmp(tokens[i], "<") == 0) {
            if (i + 1 < token_count) {
                pipeline->commands[cmd_index].input_file = strdup(tokens[i + 1]);
                i += 2;
            } else {
                fprintf(stderr, "Syntax error: no file specified for input redirection\n");
                return -1;
            }
        }
        
        // Check for output redirection
        else if (strcmp(tokens[i], ">") == 0) {
            if (i + 1 < token_count) {
                pipeline->commands[cmd_index].output_file = strdup(tokens[i + 1]);
                i += 2;
            } else {
                fprintf(stderr, "Syntax error: no file specified for output redirection\n");
                return -1;
            }
        }
        
        // Check for background execution
        else if (strcmp(tokens[i], "&") == 0) {
            pipeline->commands[cmd_index].background = 1;
            i++;
        }
        
        // Regular argument
        else {
            pipeline->commands[cmd_index].args[arg_index++] = strdup(tokens[i++]);
        }

        // Check bounds
        if (cmd_index >= MAX_PIPES) {
            fprintf(stderr, "Error: too many commands (max %d)\n", MAX_PIPES);
            // Free tokens before returning
            for (int j = 0; j < token_count; j++) free(tokens[j]);
            return -1;
        }
        if (arg_index >= MAXARGS - 1) {
            fprintf(stderr, "Error: too many arguments (max %d)\n", MAXARGS);
            // Free tokens before returning
            for (int j = 0; j < token_count; j++) free(tokens[j]);
            return -1;
        }
    }

    // Terminate the last command's argument list
    if (arg_index > 0) {
        pipeline->commands[cmd_index].args[arg_index] = NULL;
    }
    
    pipeline->num_commands = cmd_index + 1;
    
    // Free tokens
    for (int j = 0; j < token_count; j++) free(tokens[j]);
    
    return pipeline->num_commands;
}

// Free memory allocated for pipeline
void free_pipeline(pipeline_t* pipeline) {
    if (pipeline == NULL) return;
    
    for (int i = 0; i < pipeline->num_commands; i++) {
        command_t* cmd = &pipeline->commands[i];
        
        if (cmd->input_file) {
            free(cmd->input_file);
            cmd->input_file = NULL;
        }
        if (cmd->output_file) {
            free(cmd->output_file);
            cmd->output_file = NULL;
        }
        
        for (int j = 0; j < MAXARGS && cmd->args[j] != NULL; j++) {
            free(cmd->args[j]);
            cmd->args[j] = NULL;
        }
    }
    pipeline->num_commands = 0;
}
