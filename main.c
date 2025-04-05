// #include <stdio.h>
// #include <stdlib.h>
// #include <errno.h>
// #include <string.h>
// #include "shell.h"     // Includes declarations for many functions/structs used
// #include "source.h"    // Defines source_s, skip_white_spaces
// #include "parser.h"    // Declares parse_pipeline
// #include "executor.h"  // Declares execute_node, init/cleanup_job_table, update_job_statuses
// #include "scanner.h"   // Declares tokenize, free_token, eof_token
// #include "node.h"      // Declares node_s, free_node_tree
// #include "history.h"
// #include <readline/readline.h> // Already included via shell.h but good practice
// #include <readline/history.h>  // Already included via shell.h but good practice

// // Assuming these utility functions are declared elsewhere (e.g., prompt.c, initsh.c)
// extern void print_prompt1(void);
// extern void print_prompt2(void);
// extern void initsh(void);

// // Forward declaration for functions within this file
// char *read_cmd(void);
// int parse_and_execute(struct source_s *src);


// int main(int argc, char **argv)
// {
//     char *cmd;

//     (void)argc; // Suppress unused parameter warnings if not used
//     (void)argv;

//     initsh();           // Initialize shell state (e.g., signals, pwd)
//     init_job_table();   // Initialize the job control table
//     init_history();

//     do
//     {
//         update_job_statuses(); // Check background job statuses before prompt

//         print_prompt1();       // Print the primary prompt

//         cmd = read_cmd();      // Read command line input

//         if(!cmd)               // Handle EOF (Ctrl+D)
//         {
//             // EOF detected, perform cleanup before exiting
//             fprintf(stdout, "exit\n"); // Optional: Print exit like other shells
//             clear_history(); 
//             cleanup_job_table();       // Cleanup job table before exit
//             exit(EXIT_SUCCESS);
//         }

//         // ---> ADD to History (Handle potential newline) <---
//         char *cmd_to_add = cmd;
//         size_t len = strlen(cmd);
//         if (len > 0 && cmd[len-1] == '\n') {
//              // Add without newline for consistency? Or store with newline?
//              // Let's store *with* newline for simplicity now.
//              // If storing without, need strndup or temp buffer.
//              add_to_history(cmd);
//         } else if (len > 0) {
//              add_to_history(cmd); // Add if not empty and no newline
//         }
//         // -----------------------------------------------




//         // Handle empty input line (just newline or totally empty)
//         if(cmd[0] == '\0' || strcmp(cmd, "\n") == 0)
//         {
//             free(cmd);
//             continue; // Go to next prompt iteration
//         }

//         if (cmd[0] == '\0' || strcmp(cmd, "\n") == 0) { free(cmd); continue; }

//         // Handle built-in exit command explicitly for quick exit
//         if(strcmp(cmd, "exit\n") == 0)
//         {
//             free(cmd);
//             break; // Exit the main loop
//             // continue;
//         }

//         // Set up source buffer for parser
//         struct source_s src;
//         src.buffer   = cmd;
//         src.bufsize  = strlen(cmd);
//         src.curpos   = INIT_SRC_POS; // Assuming INIT_SRC_POS is defined (e.g., as -1 or 0)

//         // Parse and execute the command(s) read from the line
//         parse_and_execute(&src); // Result ignored for now, shell continues

//         // Free the command buffer read by read_cmd
//         free(cmd);

//     } while(1); // Infinite loop, broken by "exit" or EOF

//     clear_history(); // ---> ADD Clear History <---

//     cleanup_job_table(); // Cleanup job table before normal exit via "break"

//     exit(EXIT_SUCCESS);
// }


// // Function to read command input (handles line continuation)
// // (This function remains the same as provided previously)
// char *read_cmd(void)
// {
//     char buf[1024];
//     char *ptr = NULL;
//     size_t ptrlen = 0; // Use size_t for lengths

//     while(fgets(buf, sizeof(buf), stdin)) // Use sizeof(buf)
//     {
//         size_t buflen = strlen(buf); // Use size_t

//         // Allocate or reallocate buffer
//         if(!ptr)
//         {
//             ptr = malloc(buflen + 1);
//             ptrlen = 0; // Initialize length
//         }
//         else
//         {
//             // Prevent potential overflow on massive inputs (though unlikely with fgets limit)
//             size_t required_size = ptrlen + buflen + 1;
//             if (required_size < ptrlen) {
//                  fprintf(stderr, "shell: error: command too long, allocation overflow\n");
//                  free(ptr);
//                  return NULL;
//             }

//             char *ptr2 = realloc(ptr, required_size);
//             if(!ptr2)
//             {
//                 // Realloc failed, free original buffer
//                 free(ptr);
//                 ptr = NULL;
//             }
//             else
//             {
//                 ptr = ptr2;
//             }
//         }

//         // Check allocation result
//         if(!ptr)
//         {
//             fprintf(stderr, "shell: error: failed to alloc buffer: %s\n", strerror(errno));
//             return NULL; // Allocation failed
//         }

//         // Copy new data using memcpy for safety, includes null terminator implicitly
//         memcpy(ptr + ptrlen, buf, buflen + 1);
//         ptrlen += buflen;

//         // Check for line continuation or end of command
//         if(buf[buflen-1] == '\n')
//         {
//             if(buflen == 1 || buf[buflen-2] != '\\')
//             {
//                 // End of command line detected
//                 return ptr;
//             }

//             // Handle line continuation: remove backslash-newline
//             ptr[ptrlen - 2] = '\0'; // Overwrite backslash
//             ptrlen -= 2;            // Adjust stored length
//             print_prompt2();        // Print secondary prompt
//         }
//         // If buffer filled without newline (e.g., piped input), loop continues
//     }

//     // Handle EOF after having read some input without a final newline
//     if (ptr && feof(stdin)) {
//         // Make sure the possibly incomplete line is null-terminated
//         ptr[ptrlen] = '\0';
//         return ptr;
//     }

//     // Handle EOF at the very beginning or other read error on fgets
//     if (ptr) {
//         // We might have allocated memory but fgets failed before completion
//         free(ptr);
//     }
//     return NULL; // Return NULL for EOF or error on fgets entry
// }


// // Function to parse and execute one line of command(s)
// int parse_and_execute(struct source_s *src)
// {
//     // These functions/variables are assumed defined/declared elsewhere
//     // (Likely scanner.h, node.h, executor.h, parser.h, source.h)
//     extern void skip_white_spaces(struct source_s *src);
//     extern struct token_s *tokenize(struct source_s *src);
//     extern void free_token(struct token_s *tok);
//     extern struct node_s *parse_pipeline(struct token_s **tok_p); // Expects pointer-to-pointer
//     extern int execute_node(struct node_s *node);
//     extern void free_node_tree(struct node_s *node);
//     extern struct token_s eof_token; // Global EOF token marker

//     // Optional: Skip leading whitespace in the source buffer if tokenizer doesn't
//     // skip_white_spaces(src);

//     struct token_s *current_token = tokenize(src); // Get the first potentially meaningful token

//     // Check if the line was empty or only whitespace resulted in EOF
//     if (!current_token || current_token == &eof_token) {
//         if (current_token == &eof_token) {
//              // It's the static EOF token, don't free it
//         } else if (current_token) {
//             // If tokenize returned non-EOF but non-command token (e.g., only newline)
//             free_token(current_token);
//         }
//         return 1; // Treat empty/whitespace line as success (continue shell)
//     }

//     // If we got a valid starting token, proceed to parse
//     int execution_result = 1; // Default to success
//     struct node_s *cmd_node = NULL;

//     // Parse one command structure (simple command or pipeline) from the current position
//     cmd_node = parse_pipeline(&current_token); // Pass address to update current_token

//     if (!cmd_node) {
//         // Syntax error occurred during parsing.
//         // Parser should have printed a specific error message.
//         fprintf(stderr, "shell: syntax error near token: %s\n",
//             (current_token && current_token != &eof_token) ? current_token->text : "(end of input)");

//         // Consume the rest of the line to prevent issues on next prompt
//         while(current_token && current_token != &eof_token && current_token->text[0] != '\n') {
//              free_token(current_token);
//              current_token = tokenize(src);
//         }
//         execution_result = 0; // Indicate error for this command line
//     }
//     else {
//         // Successfully parsed a command structure, now execute it
//         execution_result = execute_node(cmd_node);
//         free_node_tree(cmd_node); // Free the parsed AST

//         // After successful execution, check if parser stopped before newline/EOF
//         // This indicates unexpected tokens or syntax not yet supported (like ';')
//         if (current_token && current_token != &eof_token && current_token->text[0] != '\n') {
//              fprintf(stderr, "shell: unexpected token after command processing: %s\n", current_token->text);
//              // Consume the rest of the line
//              while(current_token && current_token != &eof_token && current_token->text[0] != '\n') {
//                   free_token(current_token);
//                   current_token = tokenize(src);
//              }
//              // Consider setting execution_result = 0 here as well? Depends on desired behavior.
//         }
//     }

//     // Clean up the last token we might be holding (often the newline)
//     if (current_token && current_token != &eof_token) {
//        free_token(current_token);
//     }

//     return execution_result; // Return status of the command execution
// }


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>      // For opendir, readdir, closedir (command completion)
#include <unistd.h>      // For getenv (command completion)
#include <sys/stat.h>    // For stat (future command completion enhancement)
#include <glob.h>        // For glob, globfree (filename completion)

#include "shell.h"       // Main header, includes readline now
#include "source.h"      // Defines source_s, skip_white_spaces
#include "parser.h"      // Declares parse_pipeline
#include "executor.h"    // Declares execute_node, init/cleanup_job_table, etc.
#include "scanner.h"     // Declares tokenize, free_token, eof_token
#include "node.h"        // Declares node_s, free_node_tree

// Required includes for readline functionality declared in shell.h
#include <readline/readline.h>
#include <readline/history.h>

// --- External Function Declarations ---
// (Assuming these are defined elsewhere)
extern void print_prompt1(void); // Still potentially useful for dynamic prompts
extern void print_prompt2(void);
extern void initsh(void);

// --- Completion Function Prototypes ---
static char **attempted_completion_function(const char *text, int start, int end);
static char *command_generator(const char *text, int state);
static char *filename_generator(const char *text, int state);

// --- Main Function ---
int main(int argc, char **argv)
{
    char *cmd;
    char *current_prompt;

    (void)argc; // Suppress unused parameter warnings
    (void)argv;

    initsh();           // Initialize shell state (signals, etc.)
    init_job_table();   // Initialize job control

    // --- Readline Initialization ---
    // using_history(); // Optionally tell readline we are using history
    // read_history(NULL); // Optionally load history from ~/.history
    rl_attempted_completion_function = attempted_completion_function; // Register completion function
    // -----------------------------

    do
    {
        update_job_statuses(); // Check background job statuses

        // --- Prompt Generation ---
        // Replace simple prompt with dynamic one if needed
        // For now, use a static prompt passed to readline
        current_prompt = "$ ";
        // print_prompt1(); // No longer used directly for display
        // -----------------------

        // --- Read Command using Readline ---
        cmd = readline(current_prompt);
        // ---------------------------------

        // Handle EOF (Ctrl+D on empty line returns NULL)
        if(!cmd)
        {
            fprintf(stdout, "exit\n"); // Behave like bash
            // write_history(NULL); // Optional: Save history on exit
            cleanup_job_table();
            exit(EXIT_SUCCESS);
        }

        // Add non-empty command to history
        if (cmd && *cmd) {
            add_history(cmd);
            // Optional: Append just this command to the history file
            // append_history(1, NULL);
        }

        // Handle empty input line (user just pressed Enter)
        // readline returns non-NULL, empty string ""
        if(cmd[0] == '\0')
        {
            free(cmd); // Free the empty string readline allocated
            continue;  // Go to next prompt iteration
        }

        if(strcmp(cmd, "exit") == 0)
        {
            free(cmd);
            break; // Exit the main loop
        }

        // Explicit "exit" check removed - let the parser/executor handle it
        // If executor doesn't handle "exit", the loop breaks only on Ctrl+D

        // --- Prepare for Parsing ---
        // cmd from readline does NOT have a trailing newline
        struct source_s src;
        src.buffer   = cmd;
        src.bufsize  = strlen(cmd);
        src.curpos   = INIT_SRC_POS; // Ensure this is correctly defined (e.g., -1 or 0)
        // -------------------------

        // --- Parse and Execute ---
        parse_and_execute(&src);
        // -------------------------

        // Free the command line buffer allocated by readline
        free(cmd);

    } while(1); // Loop until EOF or explicit break/exit

    // --- Cleanup ---
    // write_history(NULL); // Optional: Save history on normal exit
    cleanup_job_table();
    // ----------------

    exit(EXIT_SUCCESS); // Should ideally be reached only via explicit "exit" command
}


// --- Parse and Execute Function ---
int parse_and_execute(struct source_s *src)
{
    // Assumed external declarations (needed by this function)
    extern void skip_white_spaces(struct source_s *src);
    extern struct token_s *tokenize(struct source_s *src);
    extern void free_token(struct token_s *tok);
    extern struct node_s *parse_pipeline(struct token_s **tok_p); // Note: pointer-to-pointer
    extern int execute_node(struct node_s *node);
    extern void free_node_tree(struct node_s *node);
    extern struct token_s eof_token; // Global EOF token marker

    // Optional: Skip leading whitespace if tokenizer doesn't
    // skip_white_spaces(src);

    struct token_s *current_token = tokenize(src); // Get the first token

    // Check if the line was effectively empty after tokenization
    if (!current_token || current_token == &eof_token) {
        // Don't free &eof_token
        if (current_token && current_token != &eof_token) {
            free_token(current_token);
        }
        return 1; // Treat empty line as success
    }

    int execution_result = 1; // Default success
    struct node_s *cmd_node = NULL;

    // Parse one command structure (simple command or pipeline)
    cmd_node = parse_pipeline(&current_token); // Pass address of pointer

    if (!cmd_node) {
        // Syntax error during parsing. Parser should print specifics.
        fprintf(stderr, "shell: syntax error near token: %s\n",
            (current_token && current_token != &eof_token) ? current_token->text : "(end of input)");

        // Consume rest of line safely to avoid issues
        while(current_token && current_token != &eof_token && current_token->text[0] != '\n') { // Check for newline added if parser expects it
             struct token_s *next_tok = tokenize(src);
             if (current_token && current_token != &eof_token) free_token(current_token);
             current_token = next_tok;
        }
        execution_result = 0; // Indicate failure for this line
    }
    else {
        // Successfully parsed, now execute
        execution_result = execute_node(cmd_node);
        free_node_tree(cmd_node); // Free the AST

        // Check if parser stopped before end of input (e.g., unsupported syntax like ';')
        if (current_token && current_token != &eof_token && current_token->text[0] != '\n') { // Check for newline added if parser expects it
             fprintf(stderr, "shell: unexpected token after command processing: %s\n", current_token->text);
             // Consume rest of line safely
             while(current_token && current_token != &eof_token && current_token->text[0] != '\n') {
                  struct token_s *next_tok = tokenize(src);
                  if (current_token && current_token != &eof_token) free_token(current_token);
                  current_token = next_tok;
             }
             // Consider setting execution_result = 0;
        }
    }

    // Clean up the last token we might be holding (often newline or EOF marker)
    if (current_token && current_token != &eof_token) {
       free_token(current_token);
    }

    return execution_result; // Return status
}

// --- Readline Tab Completion Functions ---

// Main entry point for completion
char **attempted_completion_function(const char *text, int start, int end) {
    (void)end; // Suppress unused warning for 'end'

    // Disable readline's default filename completion behavior
    rl_attempted_completion_over = 1;

    // If 'start' is 0, we're completing the command name at the beginning of the line.
    if (start == 0) {
        // Use rl_completion_matches: it calls the generator and handles display/insertion.
        // text + start points to the beginning of the word being completed.
        return rl_completion_matches(text + start, command_generator);
    } else {
        // Otherwise, for simplicity, assume we are completing a filename/argument.
        // A more sophisticated implementation would parse rl_line_buffer up to 'start'
        // to determine context (e.g., after 'cd', after '$', etc.).
        return rl_completion_matches(text + start, filename_generator);
    }
    // Note: rl_completion_matches returns NULL if no matches found, or the array of matches.
}

// Generator for command completion (builtins + PATH)
static char *command_generator(const char *text, int state) {
    // Static variables persist across calls for the same completion attempt
    static int list_index;
    static size_t len;
    static char **completion_list = NULL; // List of matching commands

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
        len = strlen(text);

        // --- Generate Matches ---
        size_t num_builtins = builtins_count; // Assumes global access via shell.h
        size_t match_capacity = num_builtins + 128; // Initial capacity guess
        size_t match_count = 0;
        completion_list = malloc(match_capacity * sizeof(char *));
        if (!completion_list) return NULL; // Allocation failed

        // 1. Add matching builtins
        for (int i = 0; i < builtins_count; i++) {
            if (strncmp(builtins[i].name, text, len) == 0) {
                 if (match_count >= match_capacity -1) { // Need space for NULL terminator
                     match_capacity *= 2;
                     char **temp = realloc(completion_list, match_capacity * sizeof(char *));
                     if (!temp) { /* Error */ break; } // Stop if realloc fails
                     completion_list = temp;
                 }
                completion_list[match_count++] = strdup(builtins[i].name);
                if (!completion_list[match_count-1]) { /* Error */ match_count--; break; } // Check strdup
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
                            // Basic check: starts with text, not . or ..
                            if (strncmp(dp->d_name, text, len) == 0 &&
                                dp->d_name[0] != '.')
                            {
                                // TODO: Add check for executability (access() or stat())
                                // TODO: Add check for duplicates already in list

                                if (match_count >= match_capacity - 1) { /* Resize */
                                      match_capacity *= 2;
                                      char **temp = realloc(completion_list, match_capacity * sizeof(char *));
                                      if (!temp) goto cleanup_path; // Use goto for nested break
                                      completion_list = temp;
                                }
                                completion_list[match_count++] = strdup(dp->d_name);
                                if (!completion_list[match_count-1]) { /* Error */ match_count--; goto cleanup_path; }
                            }
                        }
                        closedir(dirp);
                    }
                    path_token = strtok(NULL, ":");
                }
cleanup_path:
                free(path_copy);
            }
        }
         completion_list[match_count] = NULL; // Null-terminate the list
        // --- End Match Generation ---
    } // End if (state == 0)

    // Return the next match from the generated list.
    // Readline expects dynamically allocated strings (it will free them).
    if (completion_list && (name = completion_list[list_index])) {
        list_index++;
        return name; // Return the strdup'd string
    }

    // No more matches, or list generation failed. Cleanup.
    if (completion_list) {
         // Readline frees the strings we returned, but we need to free the list array itself
         // and any strings NOT returned (if generation failed midway).
         // For simplicity now, assume readline gets all strings or none if error occurred earlier.
         // A more robust implementation tracks returned strings.
         // Let's just free the list pointer itself, assuming readline frees contents.
         // Correction: We need to free the strdup'd strings *we* allocated for the list!
         for(int i = 0; completion_list[i]; i++) free(completion_list[i]);
        free(completion_list);
        completion_list = NULL;
    }
    return NULL; // Signal no more matches
}


// Generator function for filename completion using glob()
static char *filename_generator(const char *text, int state) {
    static int list_index;
    static glob_t glob_results;
    char *match = NULL;

    // If state is 0, this is the first call for this completion. Generate matches.
    if (state == 0) {
        list_index = 0;

        // Create a pattern by appending '*' to the text being completed
        char *pattern = malloc(strlen(text) + 2); // text + '*' + null
        if (!pattern) return NULL;
        sprintf(pattern, "%s*", text);

        // Perform globbing. Flags used:
        // GLOB_TILDE: Enable ~user expansion
        // GLOB_MARK: Append / to directories
        // GLOB_NOCHECK: Return the pattern itself if no matches found (optional)
        // GLOB_NOMAGIC: Treat pattern literally if no special chars (optional)
        int result = glob(pattern, GLOB_TILDE | GLOB_MARK | GLOB_NOESCAPE, NULL, &glob_results);
        free(pattern);

        // Check if glob found any matches or returned an error
        if (result != 0 && result != GLOB_NOMATCH) {
            // Handle glob error (e.g., GLOB_NOSPACE)
            globfree(&glob_results); // Free memory even on error
            return NULL;
        }
         // If result is GLOB_NOMATCH, gl_pathc will be 0, loop below handles it.
    }

    // Return the next match from the glob results, if any.
    if (list_index < glob_results.gl_pathc) {
        // Readline expects dynamically allocated strings (it will free them)
        match = strdup(glob_results.gl_pathv[list_index]);
        list_index++;
        // We don't free glob_results here, only when no matches left
    } else {
        // No more matches found, or glob failed initially. Clean up.
        if (state == 0 && list_index == 0) {
            // Glob found no matches on first call, nothing to free yet
        } else {
             globfree(&glob_results); // Free memory allocated by glob
        }
         match = NULL; // Signal end of matches
    }

    return match;
}