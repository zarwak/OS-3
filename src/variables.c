#include "shell.h"

// Global variables array
static variable_t variables[MAX_VARIABLES];
static int variable_count = 0;

// Initialize variables system
void init_variables() {
    variable_count = 0;
    
    // Set some default environment variables
    char* home = getenv("HOME");
    if (home) {
        set_variable("HOME", home);
    }
    
    char* user = getenv("USER");
    if (user) {
        set_variable("USER", user);
    }
    
    char* pwd = getenv("PWD");
    if (pwd) {
        set_variable("PWD", pwd);
    }
    
    char* shell = getenv("SHELL");
    if (shell) {
        set_variable("SHELL", shell);
    } else {
        set_variable("SHELL", "/bin/myshell");
    }
}

// Set a variable (create or update)
void set_variable(const char* name, const char* value) {
    if (name == NULL || value == NULL) return;
    
    // Check if variable already exists
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            strncpy(variables[i].value, value, VAR_VALUE_LEN - 1);
            variables[i].value[VAR_VALUE_LEN - 1] = '\0';
            return;
        }
    }
    
    // Create new variable if space available
    if (variable_count < MAX_VARIABLES) {
        strncpy(variables[variable_count].name, name, VAR_NAME_LEN - 1);
        variables[variable_count].name[VAR_NAME_LEN - 1] = '\0';
        
        strncpy(variables[variable_count].value, value, VAR_VALUE_LEN - 1);
        variables[variable_count].value[VAR_VALUE_LEN - 1] = '\0';
        
        variable_count++;
    } else {
        fprintf(stderr, "Error: too many variables (max %d)\n", MAX_VARIABLES);
    }
}

// Get a variable's value
char* get_variable(const char* name) {
    if (name == NULL) return NULL;
    
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return variables[i].value;
        }
    }
    
    // Also check environment variables
    char* env_value = getenv(name);
    if (env_value != NULL) {
        return env_value;
    }
    
    return NULL;
}

// Check if a command line is a variable assignment
int is_variable_assignment(const char* cmdline) {
    if (cmdline == NULL) return 0;
    
    // Skip leading whitespace
    const char* ptr = cmdline;
    while (*ptr == ' ' || *ptr == '\t') ptr++;
    
    // Check for valid variable name characters
    const char* equal_sign = strchr(ptr, '=');
    if (equal_sign == NULL || equal_sign == ptr) {
        return 0; // No equal sign or equal sign at start
    }
    
    // Check that there are no spaces around the equal sign
    // Actually, let's be more permissive - allow spaces after equal sign for values with spaces
    if (equal_sign > ptr && (*(equal_sign - 1) == ' ' || *(equal_sign - 1) == '\t')) {
        return 0; // Space before equal sign is not allowed
    }
    
    // Check variable name validity (alphanumeric and underscore)
    for (const char* p = ptr; p < equal_sign; p++) {
        if (!((*p >= 'a' && *p <= 'z') || 
              (*p >= 'A' && *p <= 'Z') || 
              (*p >= '0' && *p <= '9') || 
              *p == '_')) {
            return 0;
        }
    }
    
    // First character must be alphabetic or underscore
    if (!((*ptr >= 'a' && *ptr <= 'z') || 
          (*ptr >= 'A' && *ptr <= 'Z') || 
          *ptr == '_')) {
        return 0;
    }
    
    return 1;
}

// Handle variable assignment
int handle_variable_assignment(const char* cmdline) {
    if (!is_variable_assignment(cmdline)) {
        return 0;
    }
    
    // Parse name and value
    char* copy = strdup(cmdline);
    if (copy == NULL) return 0;
    
    char* equal_sign = strchr(copy, '=');
    
    if (equal_sign == NULL) {
        free(copy);
        return 0;
    }
    
    // Split into name and value
    *equal_sign = '\0';
    char* name = copy;
    char* value = equal_sign + 1;
    
    // Trim whitespace from name
    char* name_end = name + strlen(name) - 1;
    while (name_end > name && (*name_end == ' ' || *name_end == '\t')) {
        *name_end = '\0';
        name_end--;
    }
    
    // Trim leading whitespace from name
    while (*name == ' ' || *name == '\t') {
        name++;
    }
    
    // Set the variable
    set_variable(name, value);
    
    printf("Variable set: %s=%s\n", name, value); // Debug output
    
    free(copy);
    return 1;
}

// Expand variables in a string (replace $VAR with value)
char* expand_variables(const char* str) {
    if (str == NULL) return NULL;
    
    char* result = malloc(MAX_LEN);
    if (result == NULL) return NULL;
    result[0] = '\0';
    
    const char* ptr = str;
    
    while (*ptr != '\0') {
        if (*ptr == '$') {
            // Found a variable reference
            ptr++; // Skip the '$'
            
            if (*ptr == '\0') {
                // $ at end of string
                strcat(result, "$");
                break;
            }
            
            // Extract variable name
            char var_name[VAR_NAME_LEN] = "";
            int name_len = 0;
            
            if (*ptr == '{') {
                // ${VAR} syntax
                ptr++; // Skip '{'
                while (*ptr != '\0' && *ptr != '}' && name_len < VAR_NAME_LEN - 1) {
                    var_name[name_len++] = *ptr++;
                }
                if (*ptr == '}') ptr++; // Skip '}'
            } else {
                // $VAR syntax
                while ((*ptr >= 'a' && *ptr <= 'z') || 
                       (*ptr >= 'A' && *ptr <= 'Z') || 
                       (*ptr >= '0' && *ptr <= '9') || 
                       *ptr == '_') {
                    if (name_len < VAR_NAME_LEN - 1) {
                        var_name[name_len++] = *ptr;
                    }
                    ptr++;
                }
            }
            
            var_name[name_len] = '\0';
            
            // Get variable value
            char* var_value = get_variable(var_name);
            if (var_value != NULL) {
                strcat(result, var_value);
            } else {
                // If variable not found, keep the original reference
                strcat(result, "$");
                strcat(result, var_name);
            }
        } else {
            // Regular character
            int len = strlen(result);
            if (len < MAX_LEN - 2) {
                result[len] = *ptr;
                result[len + 1] = '\0';
            }
            ptr++;
        }
    }
    
    return result;
}

// Print all variables
void print_variables() {
    printf("Shell variables:\n");
    for (int i = 0; i < variable_count; i++) {
        printf("  %s=%s\n", variables[i].name, variables[i].value);
    }
    
    // Also print some important environment variables
    printf("\nEnvironment variables:\n");
    char* env_vars[] = {"PATH", "HOME", "USER", "SHELL", "PWD", NULL};
    for (int i = 0; env_vars[i] != NULL; i++) {
        char* value = getenv(env_vars[i]);
        if (value != NULL) {
            printf("  %s=%s\n", env_vars[i], value);
        }
    }
}
