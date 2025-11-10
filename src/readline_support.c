#include "shell.h"

// Custom completion function for Readline
char* command_generator(const char* text, int state) {
    static int list_index, len;
    const char* name;
    
    // List of built-in commands for completion
    static const char* builtin_commands[] = {
        "cd", "exit", "help", "history", "jobs", NULL
    };
    
    // Common system commands for completion
    static const char* common_commands[] = {
        "ls", "pwd", "whoami", "echo", "cat", "grep", "mkdir", "rmdir",
        "cp", "mv", "rm", "chmod", "ps", "kill", "find", "which", NULL
    };

    if (!state) {
        list_index = 0;
        len = strlen(text);
    }
    
    // First, try to complete with built-in commands
    while (builtin_commands[list_index] != NULL) {
        name = builtin_commands[list_index];
        list_index++;
        if (strncmp(name, text, len) == 0) {
            return strdup(name);
        }
    }
    
    // Reset index for common commands
    if (builtin_commands[list_index] == NULL) {
        // Find where builtin_commands array ends
        int i = 0;
        while (builtin_commands[i] != NULL) i++;
        list_index = i;
    }
    
    // Then try common system commands
    int common_start = list_index;
    while (common_commands[list_index - common_start] != NULL) {
        name = common_commands[list_index - common_start];
        list_index++;
        if (strncmp(name, text, len) == 0) {
            return strdup(name);
        }
    }
    
    // Finally, let Readline handle filename completion
    return NULL;
}

// Custom completion function that sets up our generator
char** custom_completion(const char* text, int start, int end) {
    char** matches = NULL;
    
    // Prevent unused parameter warnings
    (void)end;
    
    // If this is the first word, use our command completion
    if (start == 0) {
        matches = rl_completion_matches(text, command_generator);
    }
    // Otherwise, use default filename completion
    else {
        matches = rl_completion_matches(text, rl_filename_completion_function);
    }
    
    return matches;
}

// Readline-based command reader (replaces read_cmd)
char* read_cmd_readline(const char* prompt) {
    char* line = readline(prompt);
    
    if (line && *line) {
        // Add non-empty lines to Readline's history
        add_history(line);
    }
    
    return line;
}

// Initialize Readline with our custom settings
void initialize_readline() {
    // Allow conditional parsing of the ~/.inputrc file
    rl_readline_name = "myshell";
    
    // Set up custom completion
    rl_attempted_completion_function = custom_completion;
    
    // Tell Readline where to find completion matches
    rl_completion_query_items = 100;
    
    // Note: rl_completion_ignore_case might not be available in all versions
    // We'll handle case sensitivity in our generator function instead
}
