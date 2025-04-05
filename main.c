#include <stdio.h>       // For fprintf, stderr, stdout, fflush, NULL
#include <stdlib.h>      // For malloc, free, exit, EXIT_SUCCESS, EXIT_FAILURE, getenv
#include <errno.h>       // For errno, strerror
#include <string.h>      // For strcmp, strlen, strdup, strtok, memcpy, strcat, sprintf
#include <dirent.h>      // For opendir, readdir, closedir (command completion)
#include <unistd.h>      // For getenv (command completion) - likely already included
#include <sys/stat.h>    // For stat (future command completion enhancement)
#include <glob.h>        // For glob, globfree (filename completion)

// Include Readline headers FIRST before potentially conflicting project headers
#include <readline/readline.h>
#include <readline/history.h>

// Include project headers
#include "shell.h"       // Main header, declares many functions/structs
#include "source.h"      // Defines source_s, skip_white_spaces, INIT_SRC_POS
#include "parser.h"      // Declares parse_and_or (new top-level parser)
#include "executor.h"    // Declares execute_node, job functions, etc.
#include "scanner.h"     // Declares tokenize, free_token, eof_token
#include "node.h"        // Declares node_s, free_node_tree

// --- External Function Declarations ---
// (Assuming these are defined elsewhere, e.g., prompt.c, initsh.c)
extern void print_prompt1(void); // May not be used directly if prompt is simple
extern void print_prompt2(void);
extern void initsh(void);

// --- Completion Function Prototypes (Defined Below) ---
static char **attempted_completion_function(const char *text, int start, int end);
static char *command_generator(const char *text, int state);
static char *filename_generator(const char *text, int state);

// --- Forward Declaration for function in this file ---
static int parse_and_execute(struct source_s *src);


// --- Main Function ---
int main(int argc, char **argv)
{
    char *cmd;             // Buffer for command line from readline
    char *current_prompt;  // The prompt string to display
    int loop_status = 1;   // Controls the main loop (1=continue, 0=exit)

    (void)argc; // Suppress unused parameter warnings
    (void)argv;

    initsh();           // Initialize shell state (signals, etc.)
    init_job_table();   // Initialize job control structures

    // --- Readline Initialization ---
    using_history(); // Enable history management
    // read_history(NULL); // Optional: load history from default file (~/.history)
    rl_attempted_completion_function = attempted_completion_function; // Register completion function
    // -----------------------------

    // --- Main Read-Eval-Print Loop (REPL) ---
    while (loop_status)
    {
        update_job_statuses(); // Check background job statuses before showing prompt

        // --- Prompt Generation ---
        // TODO: Implement dynamic prompt generation if desired (pwd, user, etc.)
        // For now, use a static prompt.
        current_prompt = "$ ";
        // print_prompt1(); // No longer used for display with readline
        // -----------------------

        // --- Read Command using Readline ---
        // readline handles prompt display, line editing, history navigation, completion trigger
        cmd = readline(current_prompt);
        // ---------------------------------

        // Handle EOF (Ctrl+D returns NULL)
        if (!cmd)
        {
            fprintf(stdout, "exit\n"); // Mimic bash behavior on Ctrl+D
            loop_status = 0;           // Signal loop termination
            continue;                  // Skip rest of loop for cleanup
        }

        // Add non-empty, non-whitespace-only lines to history
        if (cmd && *cmd) { // Check if not NULL and not empty string ""
            // Optional: Check if it's just whitespace before adding?
            char *p = cmd;
            while (*p && isspace((unsigned char)*p)) p++; // Skip leading whitespace
            if (*p) { // If there's non-whitespace content
                 add_history(cmd);
                 // Optional: Save history after each command (less efficient)
                 // append_history(1, NULL);
            }
        }

        // Handle empty input line (user just pressed Enter)
        // readline returns non-NULL, empty string ""
        if (cmd[0] == '\0')
        {
            free(cmd); // Free the empty string readline allocated
            continue;  // Go to next prompt iteration
        }

        // --- Prepare for Parsing ---
        // cmd from readline does NOT have a trailing newline
        struct source_s src;
        src.buffer   = cmd;
        src.bufsize  = strlen(cmd);
        // Ensure INIT_SRC_POS is defined correctly (e.g., in source.h as -1 or 0)
        src.curpos   = INIT_SRC_POS;
        // -------------------------

        // --- Parse and Execute ---
        // parse_and_execute calls the parser and executor,
        // returns the exit status (0 for success, non-zero for fail).
        // The 'exit' built-in should return 0 from the executor.
        int exec_status = parse_and_execute(&src);
        if (exec_status == 0 && strcmp(cmd, "exit") == 0) { // Check if exit command succeeded
             loop_status = 0; // Signal loop termination
        }
        // TODO: Update $? based on exec_status
        // -------------------------

        // Free the command line buffer allocated by readline
        free(cmd);

    } // End main while loop

    // --- Cleanup ---
    // write_history(NULL); // Optional: Save history on normal exit
    cleanup_job_table();
    fprintf(stdout, "Shell exiting.\n"); // Optional exit message
    // ----------------

    return EXIT_SUCCESS; // Exit shell successfully
}


// --- Parse and Execute Function ---
// Parses one line of input and executes the resulting command tree.
// Returns the exit status of the executed command/pipeline/list.
static int parse_and_execute(struct source_s *src)
{
    // Required external functions/variables (declarations assumed in headers)
    extern void skip_white_spaces(struct source_s *src);
    extern struct token_s *tokenize(struct source_s *src);
    extern void free_token(struct token_s *tok);
    extern struct node_s *parse_and_or(struct token_s **tok_p); // Top-level parser
    extern int execute_node(struct node_s *node);
    extern void free_node_tree(struct node_s *node);
    extern struct token_s eof_token; // Global EOF token marker

    // skip_white_spaces(src); // Optional: If tokenizer doesn't handle leading space

    struct token_s *current_token = tokenize(src); // Get first token

    // Check if line was empty or only whitespace
    if (!current_token || current_token == &eof_token) {
        if (current_token && current_token != &eof_token) free_token(current_token);
        return 0; // Empty line is considered success (status 0)
    }

    int execution_result = 1; // Default to failure status if parse fails early
    struct node_s *cmd_node = NULL;

    // Parse one command structure (simple, pipe, and/or)
    cmd_node = parse_and_or(&current_token); // Pass address of token pointer

    if (!cmd_node) {
        // Syntax error occurred during parsing. Parser should print specifics.
        // We might still have the token that caused the error in current_token
        fprintf(stderr, "shell: syntax error near '%s'\n",
            (current_token && current_token != &eof_token) ? current_token->text : "(end of input)");

        // Consume rest of line safely to avoid issues
        while(current_token && current_token != &eof_token && current_token->text[0] != '\n') {
             struct token_s *next_tok = tokenize(src);
             if (current_token && current_token != &eof_token) free_token(current_token);
             current_token = next_tok;
        }
        execution_result = 2; // Specific syntax error status (like bash)
    }
    else {
        // Successfully parsed, now execute
        execution_result = execute_node(cmd_node);
        free_node_tree(cmd_node); // Free the AST

        // Check if parser stopped before end of input (e.g., unsupported syntax like ';')
        if (current_token && current_token != &eof_token && current_token->text[0] != '\n') {
             fprintf(stderr, "shell: unexpected token after command processing: %s\n", current_token->text);
             // Consume rest of line safely
             while(current_token && current_token != &eof_token && current_token->text[0] != '\n') {
                  struct token_s *next_tok = tokenize(src);
                  if (current_token && current_token != &eof_token) free_token(current_token);
                  current_token = next_tok;
             }
             execution_result = 2; // Syntax error
        }
    }

    // Clean up the very last token (often newline or EOF marker if not consumed)
    if (current_token && current_token != &eof_token) {
       free_token(current_token);
    }

    return execution_result; // Return exit status of executed command or error status
}


// --- Readline Tab Completion Functions ---

// Main entry point called by readline on Tab press
static char **attempted_completion_function(const char *text, int start, int end) {
    (void)end; // Suppress unused warning

    // Disable readline's default filename completion behavior
    rl_attempted_completion_over = 1;

    // If 'start' is 0, we are completing the command name itself
    if (start == 0) {
        // Use helper rl_completion_matches which calls the generator repeatedly
        return rl_completion_matches(text + start, command_generator);
    } else {
        // Otherwise, assume filename completion (simplification)
        // TODO: Enhance context detection (e.g., variables after $, dirs after cd)
        return rl_completion_matches(text + start, filename_generator);
    }
}

// Generator for command completion (builtins + PATH)
static char *command_generator(const char *text, int state) {
    // Static variables persist across calls for the *same* completion attempt
    static int list_index;
    static size_t len;
    static char **completion_list = NULL; // List of matching commands
    static size_t match_count = 0;        // ---> MOVED DECLARATION HERE (and init to 0) <---

    char *name = NULL; // The match to return

    // If state is 0, it's the first call for this completion attempt.
    // Generate the list of potential matches.
    if (state == 0) {
        // Free any previous list (important!)
        if (completion_list) {
            for(int i = 0; completion_list[i]; i++) free(completion_list[i]);
            free(completion_list);
            completion_list = NULL;
        }

        list_index = 0;
        match_count = 0; // ---> Reset match_count HERE <---
        len = strlen(text);

        // --- Generate Matches ---
        size_t num_builtins = builtins_count; // Assumes global access via shell.h
        size_t match_capacity = num_builtins + 128; // Initial capacity guess
        // match_count = 0; // Removed from here, reset above
        completion_list = malloc(match_capacity * sizeof(char *));
        if (!completion_list) return NULL; // Allocation failed

        // 1. Add matching builtins
        for (int i = 0; i < builtins_count; i++) {
            if (strncmp(builtins[i].name, text, len) == 0) {
                 if (match_count >= match_capacity - 1) { // Need space for NULL terminator
                     match_capacity *= 2;
                     char **temp = realloc(completion_list, match_capacity * sizeof(char *));
                     if (!temp) { /* Error */ break; } // Stop if realloc fails
                     completion_list = temp;
                 }
                completion_list[match_count] = strdup(builtins[i].name);
                if (!completion_list[match_count]) { /* Error */ break; } // Check strdup
                match_count++; // Increment count AFTER successful add
            }
        }

        // 2. Add matching executables from PATH
        char *path_env = getenv("PATH");
        if (path_env) {
            char *path_copy = strdup(path_env);
            if (path_copy) {
                char *path_token = strtok(path_copy, ":");
                while (path_token != NULL) {
                    DIR *dirp = opendir(path_token);
                    if (dirp) {
                        struct dirent *dp;
                        while ((dp = readdir(dirp)) != NULL) {
                            if (strncmp(dp->d_name, text, len) == 0 &&
                                dp->d_name[0] != '.')
                            {
                                // TODO: Check execute permission (access(full_path, X_OK))
                                // TODO: Check for duplicates
                                if (match_count >= match_capacity - 1) { /* Resize */
                                      match_capacity *= 2;
                                      char **temp = realloc(completion_list, match_capacity * sizeof(char *));
                                      if (!temp) goto path_cleanup; // Use goto for nested break
                                      completion_list = temp;
                                }
                                completion_list[match_count] = strdup(dp->d_name);
                                if (!completion_list[match_count]) goto path_cleanup; // strdup failed
                                match_count++; // Increment count AFTER successful add
                            }
                        }
                        closedir(dirp);
                    }
                    path_token = strtok(NULL, ":");
                }
path_cleanup: // Label for goto on error during resizing/strdup
                free(path_copy);
            }
        }
         completion_list[match_count] = NULL; // Null-terminate the list
        // --- End Match Generation ---
    } // End if (state == 0)

    // Return the next match from the generated list.
    // Use the persisted match_count.
    if (completion_list && list_index < match_count) { // Use '<' not '<='
        name = completion_list[list_index]; // Get pointer before incrementing
        list_index++;
        return name; // Return the strdup'd string (readline frees it)
    }


    // No more matches, or list is empty/null. Cleanup.
    if (completion_list) {
         // Free the list structure itself and *all* the strings we allocated.
         // Readline only frees the ones we actually RETURNED.
         for(int i = 0; completion_list[i]; i++) {
             // If we are mid-way through returning (list_index > 0), maybe
             // assume readline frees completion_list[0]..completion_list[list_index-1]?
             // Safer to just free everything we allocated here. Readline should handle
             // not double-freeing the ones it got. Let's free all.
             free(completion_list[i]);
         }
        free(completion_list);
        completion_list = NULL;
        match_count = 0; // Reset count for next time
        list_index = 0; // Reset index too
    }
    return NULL; // Signal no more matches
}


// Generator function for filename completion using glob()
static char *filename_generator(const char *text, int state) {
    // Static variables persist across calls for the same completion attempt
    static int list_index;
    static glob_t glob_results;
    char *match = NULL;

    // state == 0: First call, generate matches using glob
    if (state == 0) {
        list_index = 0;

        // Create pattern: text*
        char *pattern = malloc(strlen(text) + 2);
        if (!pattern) return NULL;
        sprintf(pattern, "%s*", text);

        // Perform globbing
        // GLOB_TILDE: Expand ~user
        // GLOB_MARK: Add / to directories
        // GLOB_NOESCAPE: Treat backslashes literally (often desired)
        int glob_flags = GLOB_TILDE | GLOB_MARK | GLOB_NOESCAPE;
        int ret = glob(pattern, glob_flags, NULL, &glob_results);
        free(pattern);

        // Handle glob errors (but GLOB_NOMATCH is okay)
        if (ret != 0 && ret != GLOB_NOMATCH) {
            // Optional: Print error? fprintf(stderr, "glob error: %d\n", ret);
            globfree(&glob_results); // Free partial results on error
            return NULL;
        }
    }

    // Return next match from glob results
    // gl_pathc contains the count of matches
    if (list_index < (int)glob_results.gl_pathc) { // Cast to int for comparison warning fix
        match = strdup(glob_results.gl_pathv[list_index]); // Return copy
        list_index++;
    } else { // No more matches
        // Free glob results ONLY when we're done returning matches
        if (list_index > 0 || state == 0) { // Only free if glob was called
             globfree(&glob_results);
        }
        match = NULL;
    }

    return match; // Return malloc'd string (readline frees) or NULL
}