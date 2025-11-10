#include "shell.h"

// Function to check if command starts with control structure
int starts_with_control_structure(const char* cmdline) {
    if (cmdline == NULL) return 0;
    
    // Skip leading whitespace
    while (*cmdline == ' ' || *cmdline == '\t') cmdline++;
    
    return (strncmp(cmdline, "if ", 3) == 0);
}

// Function to split multiline command into individual lines
int split_into_lines(const char* full_command, char*** lines) {
    if (full_command == NULL || lines == NULL) return 0;
    
    // Count lines (split by semicolons or newlines in the buffer)
    int line_count = 1;
    const char* ptr = full_command;
    
    while (*ptr) {
        if (*ptr == ';') line_count++;
        ptr++;
    }
    
    // Allocate lines array
    *lines = malloc(line_count * sizeof(char*));
    if (*lines == NULL) return 0;
    
    // Split the command
    char* copy = strdup(full_command);
    int count = 0;
    char* token = strtok(copy, ";");
    
    while (token != NULL && count < line_count) {
        // Trim whitespace
        while (*token == ' ' || *token == '\t') token++;
        char* end = token + strlen(token) - 1;
        while (end > token && (*end == ' ' || *end == '\t' || *end == '\n')) end--;
        *(end + 1) = '\0';
        
        (*lines)[count++] = strdup(token);
        token = strtok(NULL, ";");
    }
    
    free(copy);
    return count;
}

int main() {
    char* cmdline;
    pipeline_t pipeline;
    int result;

    // Initialize Readline if available
    initialize_readline();
    
    // Initialize job control
    init_jobs();
    
    // Initialize variables
    init_variables();

    while (1) {
        // Clean up zombie processes before prompt
        cleanup_zombies();
        update_jobs();

        // Use readline if available, otherwise fallback
        cmdline = read_cmd_readline(PROMPT);
        
        if (cmdline == NULL) {
            break; // EOF (Ctrl+D)
        }

        // NEW: Handle variable assignments FIRST, before history expansion
        if (handle_variable_assignment(cmdline)) {
            // Variable was assigned, don't execute as command
            free(cmdline);
            continue;
        }

        // Handle history expansion before adding to our internal history
        if (is_history_command(cmdline)) {
            char* expanded_cmd = expand_history_command(cmdline);
            if (expanded_cmd != NULL) {
                free(cmdline);
                cmdline = malloc(strlen(expanded_cmd) + 1);
                strcpy(cmdline, expanded_cmd);
                printf("%s\n", cmdline);  // Show the expanded command
            } else {
                free(cmdline);
                continue;  // Skip to next iteration if history expansion failed
            }
        }
        
        // Handle control structures (if-then-else)
        if (starts_with_control_structure(cmdline)) {
            // Read the complete multiline block
            char* full_block = read_multiline_command(cmdline);
            free(cmdline);
            
            if (full_block == NULL) {
                continue;
            }
            
            // Split into individual lines
            char** lines = NULL;
            int line_count = split_into_lines(full_block, &lines);
            
            if (line_count > 0) {
                // Parse and execute the if block
                if_block_t if_block;
                if (parse_if_block(lines, line_count, &if_block) == 0) {
                    execute_if_block(&if_block);
                    free_if_block(&if_block);
                }
                
                // Free lines
                for (int i = 0; i < line_count; i++) {
                    free(lines[i]);
                }
                free(lines);
            }
            
            free(full_block);
            continue;
        }
        
        // Add non-empty commands to our internal history (after expansion)
        if (cmdline[0] != '\0' && cmdline[0] != '\n') {
            add_to_history(cmdline);
        }

        // Parse for redirection, pipes, and command chaining
        if (parse_redirection_pipes(cmdline, &pipeline) > 0) {
            // Execute the parsed command(s)
            result = execute_pipeline(&pipeline);
            
            // Free allocated memory
            free_pipeline(&pipeline);
        } else {
            fprintf(stderr, "Error: failed to parse command\n");
        }
        
        free(cmdline);
    }

    printf("\nShell exited.\n");
    return 0;
}
