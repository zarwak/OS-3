#include "shell.h"

// Built-in command: exit
int builtin_exit(char** arglist) {
    printf("Shell terminated.\n");
    exit(0);
}

// Built-in command: cd
int builtin_cd(char** arglist) {
    if (arglist[1] == NULL) {
        // No directory provided, go to home directory
        char* home = getenv("HOME");
        if (home == NULL) {
            fprintf(stderr, "cd: HOME environment variable not set\n");
            return 1;
        }
        if (chdir(home) != 0) {
            perror("cd");
            return 1;
        }
    } else {
        // Change to specified directory
        if (chdir(arglist[1]) != 0) {
            perror("cd");
            return 1;
        }
    }
    
    // Update PWD variable
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        set_variable("PWD", cwd);
    }
    
    return 0;
}

// Built-in command: help
int builtin_help(char** arglist) {
    printf("Built-in commands:\n");
    printf("  cd <directory>    - Change current working directory\n");
    printf("  exit              - Terminate the shell\n");
    printf("  help              - Display this help message\n");
    printf("  history           - Display command history\n");
    printf("  jobs              - Display background jobs\n");
    printf("  set               - Display all variables\n");
    return 0;
}

// Built-in command: jobs
int builtin_jobs(char** arglist) {
    print_jobs();
    return 0;
}

// Built-in command: history
int builtin_history(char** arglist) {
    print_history();
    return 0;
}

// NEW: Built-in command: set (display variables)
int builtin_set(char** arglist) {
    print_variables();
    return 0;
}

// Main built-in command handler
int handle_builtin(char** arglist) {
    if (arglist[0] == NULL) {
        return 0; // No command
    }

    if (strcmp(arglist[0], "exit") == 0) {
        builtin_exit(arglist);
        return 1;
    } else if (strcmp(arglist[0], "cd") == 0) {
        builtin_cd(arglist);
        return 1;
    } else if (strcmp(arglist[0], "help") == 0) {
        builtin_help(arglist);
        return 1;
    } else if (strcmp(arglist[0], "jobs") == 0) {
        builtin_jobs(arglist);
        return 1;
    } else if (strcmp(arglist[0], "history") == 0) {
        builtin_history(arglist);
        return 1;
    } else if (strcmp(arglist[0], "set") == 0) {  // NEW: set command
        builtin_set(arglist);
        return 1;
    }

    return 0; // Not a built-in command
}
