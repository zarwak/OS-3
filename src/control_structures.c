#include "shell.h"

// Check if a word is a control structure keyword
int is_control_keyword(const char* word) {
    return (strcmp(word, "if") == 0 ||
            strcmp(word, "then") == 0 ||
            strcmp(word, "else") == 0 ||
            strcmp(word, "fi") == 0);
}

// Read multiline command for control structures
char* read_multiline_command(const char* initial_prompt) {
    printf("%s", initial_prompt);
    
    char* full_command = malloc(MAX_LEN);
    if (!full_command) return NULL;
    full_command[0] = '\0';
    
    char line[MAX_LEN];
    int if_count = 0;
    int reading_block = 0;
    
    while (fgets(line, MAX_LEN, stdin)) {
        // Remove newline
        line[strcspn(line, "\n")] = '\0';
        
        // Tokenize to check for control keywords
        char* tokens[MAXARGS];
        int token_count = 0;
        char* token = strtok(line, " \t");
        
        while (token != NULL && token_count < MAXARGS - 1) {
            tokens[token_count++] = token;
            token = strtok(NULL, " \t");
        }
        tokens[token_count] = NULL;
        
        // Count if/fi to track block structure
        if (token_count > 0) {
            if (strcmp(tokens[0], "if") == 0) {
                if_count++;
                reading_block = 1;
            } else if (strcmp(tokens[0], "fi") == 0) {
                if_count--;
            }
        }
        
        // Append line to full command
        if (strlen(full_command) + strlen(line) + 2 < MAX_LEN) {
            if (full_command[0] != '\0') {
                strcat(full_command, " ");
            }
            strcat(full_command, line);
        }
        
        // Stop reading when we reach the matching fi
        if (if_count == 0 && reading_block) {
            break;
        }
        
        if (if_count < 0) {
            fprintf(stderr, "Syntax error: unexpected 'fi'\n");
            free(full_command);
            return NULL;
        }
        
        // Show continuation prompt
        if (if_count > 0) {
            printf("> ");
        }
    }
    
    if (if_count != 0) {
        fprintf(stderr, "Syntax error: unclosed if block\n");
        free(full_command);
        return NULL;
    }
    
    return full_command;
}

// Parse if-then-else block from command lines
int parse_if_block(char** lines, int num_lines, if_block_t* if_block) {
    if (num_lines < 3) { // Minimum: if, then, fi
        fprintf(stderr, "Syntax error: incomplete if block\n");
        return -1;
    }
    
    // Initialize if_block
    if_block->condition = NULL;
    if_block->then_count = 0;
    if_block->else_count = 0;
    if_block->has_else = 0;
    
    for (int i = 0; i < MAX_BLOCK_LINES; i++) {
        if_block->then_commands[i] = NULL;
        if_block->else_commands[i] = NULL;
    }
    
    int line_index = 0;
    int in_then_block = 0;
    int in_else_block = 0;
    
    // Parse first line: if condition
    if (strncmp(lines[line_index], "if ", 3) != 0) {
        fprintf(stderr, "Syntax error: expected 'if' at beginning\n");
        return -1;
    }
    
    // Extract condition (everything after "if ")
    if_block->condition = strdup(lines[line_index] + 3);
    line_index++;
    
    // Parse then block
    if (line_index >= num_lines || strcmp(lines[line_index], "then") != 0) {
        fprintf(stderr, "Syntax error: expected 'then' after if condition\n");
        free(if_block->condition);
        return -1;
    }
    line_index++;
    
    in_then_block = 1;
    
    // Parse then commands until else or fi
    while (line_index < num_lines && in_then_block) {
        if (strcmp(lines[line_index], "else") == 0) {
            in_then_block = 0;
            in_else_block = 1;
            if_block->has_else = 1;
            line_index++;
        } else if (strcmp(lines[line_index], "fi") == 0) {
            in_then_block = 0;
            // Don't increment line_index - fi will be handled below
        } else {
            if (if_block->then_count < MAX_BLOCK_LINES) {
                if_block->then_commands[if_block->then_count++] = strdup(lines[line_index]);
            } else {
                fprintf(stderr, "Error: too many commands in then block\n");
                return -1;
            }
            line_index++;
        }
    }
    
    // Parse else block if present
    if (in_else_block) {
        while (line_index < num_lines && in_else_block) {
            if (strcmp(lines[line_index], "fi") == 0) {
                in_else_block = 0;
                // Don't increment line_index - fi will be handled below
            } else {
                if (if_block->else_count < MAX_BLOCK_LINES) {
                    if_block->else_commands[if_block->else_count++] = strdup(lines[line_index]);
                } else {
                    fprintf(stderr, "Error: too many commands in else block\n");
                    return -1;
                }
                line_index++;
            }
        }
    }
    
    // Check for closing fi
    if (line_index >= num_lines || strcmp(lines[line_index], "fi") != 0) {
        fprintf(stderr, "Syntax error: expected 'fi' at end of if block\n");
        return -1;
    }
    
    return 0;
}

// Execute an if-then-else block
int execute_if_block(if_block_t* if_block) {
    if (if_block == NULL || if_block->condition == NULL) {
        return -1;
    }
    
    // Execute the condition command
    pipeline_t pipeline;
    int condition_result = 0;
    
    if (parse_redirection_pipes(if_block->condition, &pipeline) > 0) {
        condition_result = execute_pipeline(&pipeline);
        free_pipeline(&pipeline);
    } else {
        fprintf(stderr, "Error: failed to parse condition command\n");
        return -1;
    }
    
    // Execute appropriate block based on condition result
    if (condition_result == 0) {
        // Condition succeeded - execute then block
        for (int i = 0; i < if_block->then_count; i++) {
            if (if_block->then_commands[i] != NULL) {
                if (parse_redirection_pipes(if_block->then_commands[i], &pipeline) > 0) {
                    execute_pipeline(&pipeline);
                    free_pipeline(&pipeline);
                }
            }
        }
    } else if (if_block->has_else) {
        // Condition failed and else exists - execute else block
        for (int i = 0; i < if_block->else_count; i++) {
            if (if_block->else_commands[i] != NULL) {
                if (parse_redirection_pipes(if_block->else_commands[i], &pipeline) > 0) {
                    execute_pipeline(&pipeline);
                    free_pipeline(&pipeline);
                }
            }
        }
    }
    
    return 0;
}

// Free memory allocated for if block
void free_if_block(if_block_t* if_block) {
    if (if_block == NULL) return;
    
    if (if_block->condition) {
        free(if_block->condition);
    }
    
    for (int i = 0; i < if_block->then_count; i++) {
        if (if_block->then_commands[i]) {
            free(if_block->then_commands[i]);
        }
    }
    
    for (int i = 0; i < if_block->else_count; i++) {
        if (if_block->else_commands[i]) {
            free(if_block->else_commands[i]);
        }
    }
}
