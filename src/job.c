#include "shell.h"

// Global job list
static job_t jobs[MAX_JOBS];
static int next_job_id = 1;

// Initialize job list
void init_jobs() {
    for (int i = 0; i < MAX_JOBS; i++) {
        jobs[i].pid = -1;
        jobs[i].command = NULL;
        jobs[i].status = JOB_DONE;
        jobs[i].job_id = 0;
    }
    next_job_id = 1;
}

// Add a new background job
void add_job(pid_t pid, const char* command) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].pid == -1) {
            jobs[i].pid = pid;
            jobs[i].command = strdup(command);
            jobs[i].status = JOB_RUNNING;
            jobs[i].job_id = next_job_id++;
            printf("[%d] %d\n", jobs[i].job_id, jobs[i].pid);
            return;
        }
    }
    fprintf(stderr, "Error: too many background jobs\n");
}

// Remove a completed job
void remove_job(pid_t pid) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].pid == pid) {
            free(jobs[i].command);
            jobs[i].pid = -1;
            jobs[i].command = NULL;
            jobs[i].status = JOB_DONE;
            jobs[i].job_id = 0;
            return;
        }
    }
}

// Update job status and remove completed jobs
void update_jobs() {
    int status;
    pid_t pid;
    
    // Check all jobs
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].pid != -1) {
            pid = waitpid(jobs[i].pid, &status, WNOHANG | WUNTRACED);
            
            if (pid > 0) {
                if (WIFEXITED(status)) {
                    printf("[%d] Done    %s\n", jobs[i].job_id, jobs[i].command);
                    remove_job(jobs[i].pid);
                } else if (WIFSIGNALED(status)) {
                    printf("[%d] Killed  %s\n", jobs[i].job_id, jobs[i].command);
                    remove_job(jobs[i].pid);
                } else if (WIFSTOPPED(status)) {
                    jobs[i].status = JOB_STOPPED;
                    printf("[%d] Stopped %s\n", jobs[i].job_id, jobs[i].command);
                }
            }
        }
    }
}

// Print all active jobs
void print_jobs() {
    int found = 0;
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].pid != -1) {
            const char* status_str = "Running";
            if (jobs[i].status == JOB_STOPPED) {
                status_str = "Stopped";
            }
            printf("[%d] %s %s\n", jobs[i].job_id, status_str, jobs[i].command);
            found = 1;
        }
    }
    if (!found) {
        printf("No background jobs\n");
    }
}

// Clean up zombie processes
void cleanup_zombies() {
    int status;
    pid_t pid;
    
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // Found a completed child process
        remove_job(pid);
    }
}

// Execute a command in background
int execute_background(command_t* cmd) {
    if (cmd == NULL || cmd->args[0] == NULL) {
        return -1;
    }

    // Build command string for job tracking
    char cmd_str[MAX_LEN] = "";
    for (int i = 0; cmd->args[i] != NULL; i++) {
        if (i > 0) strcat(cmd_str, " ");
        strcat(cmd_str, cmd->args[i]);
    }

    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process - set up redirection if needed
        if (cmd->input_file != NULL) {
            int input_fd = open(cmd->input_file, O_RDONLY);
            if (input_fd >= 0) {
                dup2(input_fd, STDIN_FILENO);
                close(input_fd);
            }
        }
        if (cmd->output_file != NULL) {
            int output_fd = open(cmd->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (output_fd >= 0) {
                dup2(output_fd, STDOUT_FILENO);
                close(output_fd);
            }
        }
        
        // Execute the command
        execvp(cmd->args[0], cmd->args);
        perror("execvp");
        exit(1);
    } else if (pid > 0) {
        // Parent process - add to job list
        add_job(pid, cmd_str);
        return 0;
    } else {
        perror("fork");
        return -1;
    }
}
