#include "shell.h"

// Execute a single command with redirection
int execute_redirection(command_t* cmd) {
    if (cmd == NULL || cmd->args[0] == NULL) {
        return -1;
    }

    // Handle built-in commands separately (they don't fork)
    if (handle_builtin(cmd->args)) {
        return 0;
    }

    // Handle background execution
    if (cmd->background) {
        return execute_background(cmd);
    }

    int stdin_backup = -1;
    int stdout_backup = -1;
    int input_fd = -1;
    int output_fd = -1;

    // Handle input redirection
    if (cmd->input_file != NULL) {
        stdin_backup = dup(STDIN_FILENO); // Backup original stdin
        input_fd = open(cmd->input_file, O_RDONLY);
        if (input_fd < 0) {
            perror("open input file");
            if (stdin_backup >= 0) close(stdin_backup);
            return -1;
        }
        if (dup2(input_fd, STDIN_FILENO) < 0) {
            perror("dup2 input");
            close(input_fd);
            if (stdin_backup >= 0) close(stdin_backup);
            return -1;
        }
        close(input_fd);
    }

    // Handle output redirection
    if (cmd->output_file != NULL) {
        stdout_backup = dup(STDOUT_FILENO); // Backup original stdout
        output_fd = open(cmd->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (output_fd < 0) {
            perror("open output file");
            if (stdout_backup >= 0) close(stdout_backup);
            if (stdin_backup >= 0) {
                dup2(stdin_backup, STDIN_FILENO);
                close(stdin_backup);
            }
            return -1;
        }
        if (dup2(output_fd, STDOUT_FILENO) < 0) {
            perror("dup2 output");
            close(output_fd);
            if (stdout_backup >= 0) close(stdout_backup);
            if (stdin_backup >= 0) {
                dup2(stdin_backup, STDIN_FILENO);
                close(stdin_backup);
            }
            return -1;
        }
        close(output_fd);
    }

    // Execute the command
    int status;
    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process
        execvp(cmd->args[0], cmd->args);
        perror("execvp");
        exit(1);
    } else if (pid > 0) {
        // Parent process
        waitpid(pid, &status, 0);
        
        // Restore original file descriptors
        if (stdin_backup >= 0) {
            dup2(stdin_backup, STDIN_FILENO);
            close(stdin_backup);
        }
        if (stdout_backup >= 0) {
            dup2(stdout_backup, STDOUT_FILENO);
            close(stdout_backup);
        }
        
        return WEXITSTATUS(status);
    } else {
        perror("fork");
        return -1;
    }
}

// Execute a pipeline of commands
int execute_pipeline(pipeline_t* pipeline) {
    if (pipeline == NULL || pipeline->num_commands == 0) {
        return -1;
    }

    // For now, handle sequential execution of chained commands
    // (Pipes with background execution is complex, so we'll do sequential)
    int result = 0;
    for (int i = 0; i < pipeline->num_commands; i++) {
        result = execute_single_command(&pipeline->commands[i]);
        if (result != 0) {
            break; // Stop on first error
        }
    }
    return result;
}

// Execute a single command (with or without redirection/background)
int execute_single_command(command_t* cmd) {
    if (cmd == NULL || cmd->args[0] == NULL) {
        return -1;
    }

    // Check if there's any redirection or background
    if (cmd->input_file != NULL || cmd->output_file != NULL || cmd->background) {
        return execute_redirection(cmd);
    }

    // No redirection/background, use original execute function for built-in check
    if (handle_builtin(cmd->args)) {
        return 0;
    }

    // Execute external command without redirection
    return execute(cmd->args) == 0 ? 0 : -1;
}
