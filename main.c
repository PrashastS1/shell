// #include <stdio.h>
// #include <stdlib.h>
// #include <errno.h>
// #include <string.h>
// #include "shell.h"
// #include "source.h"
// #include "parser.h"
// #include "executor.h"

// extern struct node_s *parse_pipeline(struct token_s **tok_p);
// extern int execute_node(struct node_s *node);

// int main(int argc, char **argv)
// {
//     char *cmd;

//     (void)argc; // Suppress unused parameter warnings if not used
//     (void)argv;

//     initsh();

//     init_job_table();

//     do
//     {

//         update_job_statuses();

//         print_prompt1();

//         cmd = read_cmd();

//         if(!cmd)
//         {
//             fprintf(stdout, "exit\n"); // Optional: Print exit like other shells
//             cleanup_job_table(); 

//             exit(EXIT_SUCCESS);
//         }

//         if(cmd[0] == '\0' || strcmp(cmd, "\n") == 0)
//         {
//             free(cmd);
//             continue;
//         }

//         if(strcmp(cmd, "exit\n") == 0)
//         {
//             free(cmd);
//             break;
//         }

//         // printf("%s\n", cmd);

//         struct source_s src;
//         src.buffer   = cmd;
//         src.bufsize  = strlen(cmd);
//         src.curpos   = INIT_SRC_POS;

//         parse_and_execute(&src);

//         free(cmd);

//     } while(1);

//     cleanup_job_table(); // ---> ADD: Cleanup job table before normal exit <---

//     exit(EXIT_SUCCESS);
// }

// char *read_cmd(void)
// {
//     char buf[1024];
//     char *ptr = NULL;
//     char ptrlen = 0;

//     while(fgets(buf, 1024, stdin))
//     {
//         int buflen = strlen(buf);

//         if(!ptr)
//         {
//             ptr = malloc(buflen+1);
//         }
//         else
//         {
//             char *ptr2 = realloc(ptr, ptrlen+buflen+1);

//             if(ptr2)
//             {
//                 ptr = ptr2;
//             }
//             else
//             {
//                 free(ptr);
//                 ptr = NULL;
//             }
//         }

//         if(!ptr)
//         {
//             fprintf(stderr, "error: failed to alloc buffer: %s\n", strerror(errno));
//             return NULL;
//         }

//         strcpy(ptr+ptrlen, buf);

//         if(buf[buflen-1] == '\n')
//         {
//             if(buflen == 1 || buf[buflen-2] != '\\')
//             {
//                 return ptr;
//             }

//             ptr[ptrlen+buflen-2] = '\0';
//             buflen -= 2;
//             print_prompt2();
//         }

//         ptrlen += buflen;
//     }

//     return ptr;
// }

// int parse_and_execute(struct source_s *src)
// {
//     skip_white_spaces(src);

//     // struct token_s *tok = tokenize(src);

//     // Use a pointer-to-pointer for the token, so parse_pipeline can update it
//     struct token_s *current_token = tokenize(src);



//     // if(tok == &eof_token)
//     // {
//     //     return 0;
//     // }

//     if(!current_token || current_token == &eof_token)
//      {
//          return 0;
//      }

//     // while(tok && tok != &eof_token)
//     // {
//     //     struct node_s *cmd = parse_simple_command(tok);

//     //     if(!cmd)
//     //     {
//     //         break;
//     //     }

//     //     do_simple_command(cmd);
//     //     free_node_tree(cmd);
//     //     tok = tokenize(src);
//     // }

//     int execution_result = 1; // Default to success (continue shell)

//     // --- Modified loop: Parse one pipeline/command per call ---
//     // (This assumes no ';' or other separators yet)

//     if(current_token && current_token != &eof_token){
//         // Call the pipeline parser, passing the address of the token pointer
//         struct node_s *cmd_node = parse_pipeline(¤t_token);

//         if(!cmd_node){
//             break;
//             // Syntax error, message printed by parser. Stop processing line.
//             execution_result = 0; // Indicate failure maybe? Or 1 to continue shell? Let's use 1.
//         }


        
//     }


//     return 1;
// }

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "shell.h"
#include "source.h"    // Defines source_s, skip_white_spaces
#include "parser.h"    // Defines parse_pipeline (now)
#include "executor.h"  // Defines execute_node (now), init/cleanup_job_table, update_job_statuses
#include "scanner.h"   // Defines tokenize, free_token, eof_token
#include "node.h"      // Defines node_s, free_node_tree

// Assuming these utility functions are declared elsewhere (e.g., prompt.c, initsh.c)
extern void print_prompt1(void);
extern void print_prompt2(void);
extern void initsh(void);

// Forward declaration for function within this file
char *read_cmd(void);
int parse_and_execute(struct source_s *src);


int main(int argc, char **argv)
{
    char *cmd;

    (void)argc; // Suppress unused parameter warnings if not used
    (void)argv;

    initsh();           // Initialize shell state (e.g., signals, pwd)
    init_job_table();   // Initialize the job control table

    do
    {
        update_job_statuses(); // Check background job statuses before prompt

        print_prompt1();       // Print the primary prompt

        cmd = read_cmd();      // Read command line input

        if(!cmd)               // Handle EOF (Ctrl+D)
        {
            // EOF detected, perform cleanup before exiting
            fprintf(stdout, "exit\n"); // Optional: Print exit like other shells
            cleanup_job_table();       // Cleanup job table before exit
            exit(EXIT_SUCCESS);
        }

        // Handle empty input line
        if(cmd[0] == '\0' || strcmp(cmd, "\n") == 0)
        {
            free(cmd);
            continue;
        }

        // Handle built-in exit command explicitly for quick exit
        // Note: `parse_and_execute` might also handle it via the executor
        if(strcmp(cmd, "exit\n") == 0)
        {
            free(cmd);
            break; // Exit the main loop
        }

        // Set up source buffer for parser
        struct source_s src;
        src.buffer   = cmd;
        src.bufsize  = strlen(cmd);
        src.curpos   = INIT_SRC_POS; // Assuming INIT_SRC_POS is defined (e.g., as -1 or 0)

        // Parse and execute the command(s) read from the line
        parse_and_execute(&src);

        // Free the command buffer read by read_cmd
        free(cmd);

    } while(1); // Infinite loop, broken by "exit" or EOF

    cleanup_job_table(); // Cleanup job table before normal exit via "break"

    exit(EXIT_SUCCESS);
}


// Function to read command input (handles line continuation)
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
    // (Likely scanner.h, node.h, executor.h, parser.h)
    extern void skip_white_spaces(struct source_s *src);
    extern struct token_s *tokenize(struct source_s *src);
    extern void free_token(struct token_s *tok);
    extern struct node_s *parse_pipeline(struct token_s **tok_p); // Expects pointer-to-pointer
    extern int execute_node(struct node_s *node);
    extern void free_node_tree(struct node_s *node);
    extern struct token_s eof_token;

    skip_white_spaces(src); // Consume leading whitespace

    struct token_s *current_token = tokenize(src); // Get the first token

    if (!current_token || current_token == &eof_token) {
        return 1; // Empty line or EOF is not an execution error
    }

    int execution_result = 1; // Default to success (continue shell)
    struct node_s *cmd_node = NULL;

    // Loop to handle potential multiple commands per line (e.g. if ';' is added later)
    // For now, with only pipes, it likely parses just one pipeline tree per call.
    while (current_token && current_token != &eof_token)
    {
        // Call the pipeline parser, passing the address of the token pointer
        // This allows parse_pipeline to update current_token as it consumes tokens.
        cmd_node = parse_pipeline(current_token);

        if (!cmd_node) {
            // Syntax error, message should be printed by the parser.
            // We need to decide how to recover. For now, stop processing this line.
            fprintf(stderr, "shell: syntax error near token: %s\n",
                (current_token && current_token != &eof_token) ? current_token->text : "(end of line)");

            // Consume remaining tokens on the line to avoid infinite loops if parser failed early
            while(current_token && current_token != &eof_token && current_token->text[0] != '\n') {
                 free_token(current_token);
                 current_token = tokenize(src);
            }
            execution_result = 0; // Indicate an error occurred on this line
            break; // Stop processing this line
        }
        else {
            // Successfully parsed a command/pipeline, execute it
            execution_result = execute_node(cmd_node);
            free_node_tree(cmd_node); // Free the parsed tree

            // If parse_pipeline finished correctly but didn't reach EOF or newline,
            // it means there might be an operator we don't handle yet (like ';').
            // For now, we'll treat extra tokens as an error after a successful parse/execute.
            if (current_token && current_token != &eof_token && current_token->text[0] != '\n') {
                 fprintf(stderr, "shell: unexpected token after command: %s\n", current_token->text);
                 // Consume the rest of the line
                 while(current_token && current_token != &eof_token && current_token->text[0] != '\n') {
                      free_token(current_token);
                      current_token = tokenize(src);
                 }
                 execution_result = 0; // Indicate error
                 break; // Stop processing
            }
        }
         // If current_token is now newline or EOF, the loop condition will handle it
    } // End while loop over tokens

    // Free the last token if it wasn't EOF or consumed
    if (current_token && current_token != &eof_token) {
       free_token(current_token);
    }

    return execution_result; // Return status of the last command/pipeline attempted
}