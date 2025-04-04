#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "shell.h"     // Includes declarations for many functions/structs used
#include "source.h"    // Defines source_s, skip_white_spaces
#include "parser.h"    // Declares parse_pipeline
#include "executor.h"  // Declares execute_node, init/cleanup_job_table, update_job_statuses
#include "scanner.h"   // Declares tokenize, free_token, eof_token
#include "node.h"      // Declares node_s, free_node_tree
#include "history.h"

// Assuming these utility functions are declared elsewhere (e.g., prompt.c, initsh.c)
extern void print_prompt1(void);
extern void print_prompt2(void);
extern void initsh(void);

// Forward declaration for functions within this file
char *read_cmd(void);
int parse_and_execute(struct source_s *src);


int main(int argc, char **argv)
{
    char *cmd;

    (void)argc; // Suppress unused parameter warnings if not used
    (void)argv;

    initsh();           // Initialize shell state (e.g., signals, pwd)
    init_job_table();   // Initialize the job control table
    init_history();

    do
    {
        update_job_statuses(); // Check background job statuses before prompt

        print_prompt1();       // Print the primary prompt

        cmd = read_cmd();      // Read command line input

        if(!cmd)               // Handle EOF (Ctrl+D)
        {
            // EOF detected, perform cleanup before exiting
            fprintf(stdout, "exit\n"); // Optional: Print exit like other shells
            clear_history(); 
            cleanup_job_table();       // Cleanup job table before exit
            exit(EXIT_SUCCESS);
        }

        // ---> ADD to History (Handle potential newline) <---
        char *cmd_to_add = cmd;
        size_t len = strlen(cmd);
        if (len > 0 && cmd[len-1] == '\n') {
             // Add without newline for consistency? Or store with newline?
             // Let's store *with* newline for simplicity now.
             // If storing without, need strndup or temp buffer.
             add_to_history(cmd);
        } else if (len > 0) {
             add_to_history(cmd); // Add if not empty and no newline
        }
        // -----------------------------------------------




        // Handle empty input line (just newline or totally empty)
        if(cmd[0] == '\0' || strcmp(cmd, "\n") == 0)
        {
            free(cmd);
            continue; // Go to next prompt iteration
        }

        if (cmd[0] == '\0' || strcmp(cmd, "\n") == 0) { free(cmd); continue; }

        // Handle built-in exit command explicitly for quick exit
        if(strcmp(cmd, "exit\n") == 0)
        {
            free(cmd);
            break; // Exit the main loop
            // continue;
        }

        // Set up source buffer for parser
        struct source_s src;
        src.buffer   = cmd;
        src.bufsize  = strlen(cmd);
        src.curpos   = INIT_SRC_POS; // Assuming INIT_SRC_POS is defined (e.g., as -1 or 0)

        // Parse and execute the command(s) read from the line
        parse_and_execute(&src); // Result ignored for now, shell continues

        // Free the command buffer read by read_cmd
        free(cmd);

    } while(1); // Infinite loop, broken by "exit" or EOF

    clear_history(); // ---> ADD Clear History <---

    cleanup_job_table(); // Cleanup job table before normal exit via "break"

    exit(EXIT_SUCCESS);
}


// Function to read command input (handles line continuation)
// (This function remains the same as provided previously)
char *read_cmd(void)
{
    char buf[1024];
    char *ptr = NULL;
    size_t ptrlen = 0; // Use size_t for lengths

    while(fgets(buf, sizeof(buf), stdin)) // Use sizeof(buf)
    {
        size_t buflen = strlen(buf); // Use size_t

        // Allocate or reallocate buffer
        if(!ptr)
        {
            ptr = malloc(buflen + 1);
            ptrlen = 0; // Initialize length
        }
        else
        {
            // Prevent potential overflow on massive inputs (though unlikely with fgets limit)
            size_t required_size = ptrlen + buflen + 1;
            if (required_size < ptrlen) {
                 fprintf(stderr, "shell: error: command too long, allocation overflow\n");
                 free(ptr);
                 return NULL;
            }

            char *ptr2 = realloc(ptr, required_size);
            if(!ptr2)
            {
                // Realloc failed, free original buffer
                free(ptr);
                ptr = NULL;
            }
            else
            {
                ptr = ptr2;
            }
        }

        // Check allocation result
        if(!ptr)
        {
            fprintf(stderr, "shell: error: failed to alloc buffer: %s\n", strerror(errno));
            return NULL; // Allocation failed
        }

        // Copy new data using memcpy for safety, includes null terminator implicitly
        memcpy(ptr + ptrlen, buf, buflen + 1);
        ptrlen += buflen;

        // Check for line continuation or end of command
        if(buf[buflen-1] == '\n')
        {
            if(buflen == 1 || buf[buflen-2] != '\\')
            {
                // End of command line detected
                return ptr;
            }

            // Handle line continuation: remove backslash-newline
            ptr[ptrlen - 2] = '\0'; // Overwrite backslash
            ptrlen -= 2;            // Adjust stored length
            print_prompt2();        // Print secondary prompt
        }
        // If buffer filled without newline (e.g., piped input), loop continues
    }

    // Handle EOF after having read some input without a final newline
    if (ptr && feof(stdin)) {
        // Make sure the possibly incomplete line is null-terminated
        ptr[ptrlen] = '\0';
        return ptr;
    }

    // Handle EOF at the very beginning or other read error on fgets
    if (ptr) {
        // We might have allocated memory but fgets failed before completion
        free(ptr);
    }
    return NULL; // Return NULL for EOF or error on fgets entry
}


// Function to parse and execute one line of command(s)
int parse_and_execute(struct source_s *src)
{
    // These functions/variables are assumed defined/declared elsewhere
    // (Likely scanner.h, node.h, executor.h, parser.h, source.h)
    extern void skip_white_spaces(struct source_s *src);
    extern struct token_s *tokenize(struct source_s *src);
    extern void free_token(struct token_s *tok);
    extern struct node_s *parse_pipeline(struct token_s **tok_p); // Expects pointer-to-pointer
    extern int execute_node(struct node_s *node);
    extern void free_node_tree(struct node_s *node);
    extern struct token_s eof_token; // Global EOF token marker

    // Optional: Skip leading whitespace in the source buffer if tokenizer doesn't
    // skip_white_spaces(src);

    struct token_s *current_token = tokenize(src); // Get the first potentially meaningful token

    // Check if the line was empty or only whitespace resulted in EOF
    if (!current_token || current_token == &eof_token) {
        if (current_token == &eof_token) {
             // It's the static EOF token, don't free it
        } else if (current_token) {
            // If tokenize returned non-EOF but non-command token (e.g., only newline)
            free_token(current_token);
        }
        return 1; // Treat empty/whitespace line as success (continue shell)
    }

    // If we got a valid starting token, proceed to parse
    int execution_result = 1; // Default to success
    struct node_s *cmd_node = NULL;

    // Parse one command structure (simple command or pipeline) from the current position
    cmd_node = parse_pipeline(&current_token); // Pass address to update current_token

    if (!cmd_node) {
        // Syntax error occurred during parsing.
        // Parser should have printed a specific error message.
        fprintf(stderr, "shell: syntax error near token: %s\n",
            (current_token && current_token != &eof_token) ? current_token->text : "(end of input)");

        // Consume the rest of the line to prevent issues on next prompt
        while(current_token && current_token != &eof_token && current_token->text[0] != '\n') {
             free_token(current_token);
             current_token = tokenize(src);
        }
        execution_result = 0; // Indicate error for this command line
    }
    else {
        // Successfully parsed a command structure, now execute it
        execution_result = execute_node(cmd_node);
        free_node_tree(cmd_node); // Free the parsed AST

        // After successful execution, check if parser stopped before newline/EOF
        // This indicates unexpected tokens or syntax not yet supported (like ';')
        if (current_token && current_token != &eof_token && current_token->text[0] != '\n') {
             fprintf(stderr, "shell: unexpected token after command processing: %s\n", current_token->text);
             // Consume the rest of the line
             while(current_token && current_token != &eof_token && current_token->text[0] != '\n') {
                  free_token(current_token);
                  current_token = tokenize(src);
             }
             // Consider setting execution_result = 0 here as well? Depends on desired behavior.
        }
    }

    // Clean up the last token we might be holding (often the newline)
    if (current_token && current_token != &eof_token) {
       free_token(current_token);
    }

    return execution_result; // Return status of the command execution
}