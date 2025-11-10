#define  SHELL_H
#define SHELL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

// Check if readline is available by testing its existence
#if __has_include(<readline/readline.h>) && __has_include(<readline/history.h>)
#include <readline/readline.h>
#include <readline/history.h>
#define USE_READLINE 1
#else
// Fallback: define dummy functions if readline not available
static inline char* readline(const char* prompt) {
    printf("%s", prompt);
    char* line = malloc(512);
    if (fgets(line, 512, stdin)) {
        // Remove newline
        char* newline = strchr(line, '\n');
        if (newline) *newline = '\0';
        return line;
    }
    free(line);
    return NULL;
}

static inline void add_history(const char* line) {
    // Do nothing - fallback
    (void)line;
}

static inline char** rl_completion_matches(const char* text, char*(*generator)(const char*, int)) {
    (void)text;
    (void)generator;
    return NULL;
}

static inline char* rl_filename_completion_function(const char* text, int state) {
    (void)text;
    (void)state;
    return NULL;
}

// Dummy variables for readline
char* rl_readline_name = "";
int rl_completion_query_items = 100;
rl_completion_func_t* rl_attempted_completion_function = NULL;
#endif

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30
#define PROMPT "FCIT> "
#define HISTORY_SIZE 20
#define MAX_PIPES 10
#define MAX_JOBS 100
#define MAX_IF_BLOCKS 10
#define MAX_BLOCK_LINES 20
#define MAX_VARIABLES 100  // NEW: Maximum number of variables
#define VAR_NAME_LEN 50    // NEW: Maximum variable name length
#define VAR_VALUE_LEN 256  // NEW: Maximum variable value length

// NEW: Structure for shell variables
typedef struct {
    char name[VAR_NAME_LEN];
    char value[VAR_VALUE_LEN];
} variable_t;

// Structure for if-then-else block
typedef struct {
    char* condition;                    // Condition command
    char* then_commands[MAX_BLOCK_LINES]; // Commands in then block
    char* else_commands[MAX_BLOCK_LINES]; // Commands in else block
    int then_count;                     // Number of commands in then block
    int else_count;                     // Number of commands in else block
    int has_else;                       // Whether else block exists
} if_block_t;

// Job status enumeration
typedef enum {
    JOB_RUNNING,
    JOB_STOPPED,
    JOB_DONE
} job_status_t;

// Structure for background job tracking
typedef struct {
    pid_t pid;           // Process ID
    char* command;       // Command string
    job_status_t status; // Job status
    int job_id;          // Job ID number
} job_t;

// Structure to hold command information with redirection
typedef struct {
    char* args[MAXARGS];     // Command arguments
    char* input_file;        // File for input redirection (<)
    char* output_file;       // File for output redirection (>)
    int background;          // Run in background (&)
} command_t;

// Structure to hold pipeline information
typedef struct {
    command_t commands[MAX_PIPES];  // Commands in the pipeline
    int num_commands;               // Number of commands in pipeline
} pipeline_t;

// Function prototypes
char* read_cmd(char* prompt, FILE* fp);
char** tokenize(char* cmdline);
int execute(char** arglist);
int handle_builtin(char** arglist);

// History function prototypes
void add_to_history(const char* cmd);
void print_history();
char* get_history_command(int n);
int is_history_command(const char* cmdline);
char* expand_history_command(const char* cmdline);

// Readline-based command reader
char* read_cmd_readline(const char* prompt);
void initialize_readline();

// Redirection and pipe function prototypes
int parse_redirection_pipes(char* cmdline, pipeline_t* pipeline);
void free_pipeline(pipeline_t* pipeline);
int execute_redirection(command_t* cmd);
int execute_pipeline(pipeline_t* pipeline);
int execute_single_command(command_t* cmd);

// Job control function prototypes
void init_jobs();
void add_job(pid_t pid, const char* command);
void remove_job(pid_t pid);
void update_jobs();
void print_jobs();
void cleanup_zombies();
int execute_background(command_t* cmd);

// if-then-else function prototypes
int parse_if_block(char** lines, int num_lines, if_block_t* if_block);
int execute_if_block(if_block_t* if_block);
int is_control_keyword(const char* word);
char* read_multiline_command(const char* initial_prompt);
void free_if_block(if_block_t* if_block);

// NEW: Variable function prototypes
void init_variables();
void set_variable(const char* name, const char* value);
char* get_variable(const char* name);
int is_variable_assignment(const char* cmdline);
int handle_variable_assignment(const char* cmdline);
char* expand_variables(const char* str);
void print_variables();
