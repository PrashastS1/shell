// #include <unistd.h>
// #include <string.h> // For strcmp
// #include <stdio.h>  // For fprintf
// #include "shell.h"
// #include "parser.h"
// #include "scanner.h"
// #include "node.h"
// #include "source.h"
// #include <fcntl.h>
// #include <stdlib.h>

// // Helper function prototype (implement below or use existing if available)
// struct token_s *consume_token(struct token_s **tok_p, struct source_s *src);

// // Forward declaration for the new function
// struct node_s *parse_pipeline(struct token_s **tok_p);

// struct node_s *parse_simple_command(struct token_s **tok_p)
// {

//     if (!tok_p || !(*tok_p) || (*tok_p) == &eof_token) {
//         return NULL;
//     }

//     struct node_s *cmd = new_node(NODE_COMMAND);
//     if (!cmd) {
//         return NULL;
//     }

//     struct source_s *src = (*tok_p)->src;

//     while ((*tok_p) != &eof_token && (*tok_p)->text[0] != '\n' && strcmp((*tok_p)->text, "|") != 0 )
//      {
//         // ---> Check for redirection operators <---
//         if (strcmp((*tok_p)->text, "<") == 0) {
//             // Input Redirection
//             free_token(*tok_p); // Consume '<'
//             *tok_p = tokenize(src);
//             if (!(*tok_p) || (*tok_p) == &eof_token || (*tok_p)->text[0] == '\n' || strcmp((*tok_p)->text, "|") == 0 || strcmp((*tok_p)->text, ">") == 0 || strcmp((*tok_p)->text, ">>") == 0 || strcmp((*tok_p)->text, "<") == 0) {
//                 fprintf(stderr, "shell: syntax error: expected filename after '<'\n");
//                 free_node_tree(cmd);
//                 if(*tok_p) free_token(*tok_p);
//                 return NULL;
//             }
//             // Store infile name (handle potential previous infile?)
//             if (cmd->infile) free(cmd->infile); // Overwrite previous if any
//             cmd->infile = strdup((*tok_p)->text);
//             if (!cmd->infile) { perror("shell: strdup infile"); free_node_tree(cmd); free_token(*tok_p); return NULL; }
//             // Consume filename token
//             free_token(*tok_p);
//             *tok_p = tokenize(src);

//         } else if (strcmp((*tok_p)->text, ">") == 0 || strcmp((*tok_p)->text, ">>") == 0) {
//             // Output Redirection (Overwrite or Append)
//             int is_append = (strcmp((*tok_p)->text, ">>") == 0);
//             free_token(*tok_p); // Consume '>' or '>>'
//             *tok_p = tokenize(src);
//              if (!(*tok_p) || (*tok_p) == &eof_token || (*tok_p)->text[0] == '\n' || strcmp((*tok_p)->text, "|") == 0 || strcmp((*tok_p)->text, ">") == 0 || strcmp((*tok_p)->text, ">>") == 0 || strcmp((*tok_p)->text, "<") == 0) {
//                 fprintf(stderr, "shell: syntax error: expected filename after '%s'\n", is_append ? ">>" : ">");
//                 free_node_tree(cmd);
//                 if(*tok_p) free_token(*tok_p);
//                 return NULL;
//             }

//             if (cmd->outfile) free(cmd->outfile); // Overwrite previous if any
//             cmd->outfile = strdup((*tok_p)->text);
//             if (!cmd->outfile) { perror("shell: strdup outfile"); free_node_tree(cmd); free_token(*tok_p); return NULL; }
//             cmd->append_mode = is_append;
//             // Consume filename token
//             free_token(*tok_p);
//             *tok_p = tokenize(src);

//         } else {
//             // ---> Regular command word (argument/command name) <---
//              struct node_s *word = new_node(NODE_VAR);
//              if (!word) {
//                  free_node_tree(cmd);
//                 // Don't free *tok_p, loop will exit or continue
//                  return NULL;
//              }
//              set_node_val_str(word, (*tok_p)->text);
//              add_child_node(cmd, word);

//              // Consume the current token and get the next one
//              free_token(*tok_p);
//              *tok_p = tokenize(src);
//         } // End of if/else for redirection vs word

//         if (!(*tok_p)) { // Check if tokenize returned NULL (error?)
//             fprintf(stderr, "shell: error tokenizing input\n");
//             free_node_tree(cmd);
//             if(cmd) free_node_tree(cmd);
//             return NULL; // Indicate error
//         }
//     }

//     if (!cmd) return NULL;

//     if (cmd->children == 0 && cmd->infile == NULL && cmd->outfile == NULL) {
//           free_node_tree(cmd);
//           return NULL;
//      }

//     return cmd;
// }

// struct node_s *parse_pipeline(struct token_s **tok_p)
// {

//     if (!tok_p || !(*tok_p) || (*tok_p) == &eof_token) {
//         return NULL;
//     }

//     // 1. Parse the first command in the pipeline
//     struct node_s *left_cmd = parse_simple_command(tok_p);
//     if (!left_cmd) {
//         return NULL;
//     }

//     // 2. Check if a pipe follows
//     if ((*tok_p) != &eof_token && strcmp((*tok_p)->text, "|") == 0)
//     {

//         struct source_s *src = (*tok_p)->src;

//         // Consume the '|' token
//         // Ensure we don't free the static eof_token
//         if (*tok_p && *tok_p != &eof_token) {
//             free_token(*tok_p);
//             *tok_p = NULL;
//         }

//         // Consume the '|' token
//         // free_token(*tok_p);
//         // *tok_p = tokenize((*tok_p)->src); // Get token after pipe
//         *tok_p = tokenize(src);  

//          if (!(*tok_p) || (*tok_p) == &eof_token) { // Check for error or EOF after '|'
//              fprintf(stderr, "shell: syntax error: expected command after '|'\n");
//              fflush(stderr);
//              free_node_tree(left_cmd);
//             //  if (*tok_p) free_token(*tok_p); // Free EOF token if exists
//              return NULL;
//          }
//          if (!(*tok_p)->text || (*tok_p)->text[0] == '\n') { // Check for newline after '|'
//               fprintf(stderr, "shell: syntax error: expected command after '|'\n");
//               fflush(stderr);
//               free_node_tree(left_cmd);
//               free_token(*tok_p);
//               *tok_p = NULL;
//               return NULL;
//          }


//         // 3. Recursively parse the rest of the pipeline

//         // fprintf(stderr, "DEBUG: Calling parse_pipeline recursively for right side...\n"); // Add this
//         // fflush(stderr);

//         struct node_s *right_pipeline = parse_pipeline(tok_p);
//         if (!right_pipeline) {
//             fprintf(stderr, "shell: syntax error parsing command after '|'\n");
//             free_node_tree(left_cmd);
//             // *tok_p might point to problematic token, don't free here
//             return NULL;
//         }

//         // 4. Create the PIPE node
//         struct node_s *pipe_node = new_node(NODE_PIPE);
//         if (!pipe_node) {
//             free_node_tree(left_cmd);
//             free_node_tree(right_pipeline);
//             return NULL;
//         }

//         // 5. Link children: left_cmd is first, right_pipeline is second
//         add_child_node(pipe_node, left_cmd);
//         add_child_node(pipe_node, right_pipeline); // add_child handles linking siblings

//         return pipe_node; // Return the newly constructed pipe tree
//     }
//     else
//     {
//         // No pipe found, just return the single simple command
//         return left_cmd;
//     }
// }

#include <unistd.h>
#include <string.h> // For strcmp, strdup
#include <stdio.h>  // For fprintf, stderr, perror
#include <stdlib.h> // For malloc, free
#include <fcntl.h>  // May be needed if parser handles file descriptors directly (unlikely here)

#include "shell.h"
#include "parser.h"
#include "scanner.h" // Assumes declares tokenize, free_token, eof_token
#include "node.h"    // Assumes declares node types, node_s struct, node functions
#include "source.h"  // Assumes declares source_s

// Forward declarations for functions within this file
struct node_s *parse_simple_command(struct token_s **tok_p);
struct node_s *parse_pipeline(struct token_s **tok_p);
struct node_s *parse_and_or(struct token_s **tok_p);

/*
 * Parses words and redirections into a NODE_COMMAND structure.
 * Stops parsing and returns the node when it encounters:
 *  - End of File (EOF)
 *  - Newline ('\n')
 *  - Pipe ('|')
 *  - AND ('&&')
 *  - OR ('||')
 *  - (Future operators like ';', '&' could be added here)
 *
 * It consumes the tokens belonging to the simple command (words, redirection ops, filenames).
 * It does *not* consume the terminating token ('|', '&&', '||', '\n', EOF).
 * *tok_p is updated to point to the terminating token.
 * Returns NULL on error or if no command words/redirections are found.
 */
struct node_s *parse_simple_command(struct token_s **tok_p)
{
    // Check for invalid initial token
    if (!tok_p || !(*tok_p) || (*tok_p) == &eof_token) {
        return NULL;
    }

    struct node_s *cmd = new_node(NODE_COMMAND);
    if (!cmd) {
        perror("shell: new_node failed in parse_simple_command");
        return NULL; // Allocation failed
    }

    struct source_s *src = (*tok_p)->src; // Get source from the first token

    // Loop as long as we have a valid token that isn't a terminating operator/newline/EOF
    while ((*tok_p) && (*tok_p) != &eof_token && (*tok_p)->text && // Defensive checks
           (*tok_p)->text[0] != '\n' &&
           strcmp((*tok_p)->text, "|") != 0 &&
           strcmp((*tok_p)->text, "&&") != 0 &&
           strcmp((*tok_p)->text, "||") != 0 /* && Add future terminators like ';' */ )
    {
        // Check for redirection operators first
        if (strcmp((*tok_p)->text, "<") == 0) {
            // Input Redirection
            struct token_s *op_token = *tok_p; // Save '<' token
            *tok_p = tokenize(src);            // Get next token (potential filename)
            free_token(op_token);              // Consume '<'

            // Check for valid filename token after '<'
            if (!(*tok_p) || (*tok_p) == &eof_token || !(*tok_p)->text ||
                strchr("|&<>", (*tok_p)->text[0]) || (*tok_p)->text[0] == '\n' ) { // Basic check for operators/newline
                fprintf(stderr, "shell: syntax error: expected filename after '<'\n");
                free_node_tree(cmd);
                if (*tok_p && *tok_p != &eof_token) free_token(*tok_p); // Free bad token
                *tok_p = NULL; // Mark as error state
                return NULL;
            }

            // Store infile name (free previous if overwriting)
            if (cmd->infile) free(cmd->infile);
            cmd->infile = strdup((*tok_p)->text);
            if (!cmd->infile) {
                perror("shell: strdup infile failed");
                free_node_tree(cmd);
                free_token(*tok_p); // Free filename token
                *tok_p = NULL;
                return NULL;
            }

            // Consume filename token and get the next one
            struct token_s *filename_token = *tok_p;
            *tok_p = tokenize(src);
            free_token(filename_token);

        } else if (strcmp((*tok_p)->text, ">") == 0 || strcmp((*tok_p)->text, ">>") == 0) {
            // Output Redirection (Overwrite or Append)
            int is_append = (strcmp((*tok_p)->text, ">>") == 0);
            char *op_text = strdup((*tok_p)->text); // Save operator text for error msg
            struct token_s *op_token = *tok_p;      // Save '>' or '>>' token
            *tok_p = tokenize(src);                 // Get next token (potential filename)
            free_token(op_token);                   // Consume '>' or '>>'

            // Check for valid filename token
             if (!(*tok_p) || (*tok_p) == &eof_token || !(*tok_p)->text ||
                 strchr("|&<>", (*tok_p)->text[0]) || (*tok_p)->text[0] == '\n') {
                fprintf(stderr, "shell: syntax error: expected filename after '%s'\n", op_text ? op_text : "");
                if (op_text) free(op_text);
                free_node_tree(cmd);
                if (*tok_p && *tok_p != &eof_token) free_token(*tok_p);
                *tok_p = NULL;
                return NULL;
            }
            if(op_text) free(op_text); // Free temporary copy

            // Store outfile name
            if (cmd->outfile) free(cmd->outfile);
            cmd->outfile = strdup((*tok_p)->text);
            if (!cmd->outfile) {
                perror("shell: strdup outfile failed");
                free_node_tree(cmd);
                free_token(*tok_p); // Free filename token
                *tok_p = NULL;
                return NULL;
            }
            cmd->append_mode = is_append;

            // Consume filename token and get the next one
            struct token_s *filename_token = *tok_p;
            *tok_p = tokenize(src);
            free_token(filename_token);

        } else {
            // Regular command word (argument/command name)
             struct node_s *word = new_node(NODE_VAR);
             if (!word) {
                 perror("shell: new_node failed for command word");
                 free_node_tree(cmd);
                 // Don't free *tok_p, loop will exit or caller handles
                 return NULL;
             }
             set_node_val_str(word, (*tok_p)->text);
             add_child_node(cmd, word);

             // Consume the word token and get the next one
             struct token_s *word_token = *tok_p;
             *tok_p = tokenize(src);
             free_token(word_token);
        } // End of if/else for redirection vs word

        // Check if tokenization failed after consuming a token
        if (!(*tok_p)) {
            fprintf(stderr, "shell: error tokenizing input after parsing element\n");
            free_node_tree(cmd);
            return NULL; // Indicate error
        }
    } // End while loop over tokens for simple command

    // Check if we actually added anything (command words or redirections)
    if (cmd->children == 0 && cmd->infile == NULL && cmd->outfile == NULL) {
          // It was an empty command or only operators were found before termination
          free_node_tree(cmd);
          return NULL;
     }

    // Return the constructed command node. *tok_p points to the terminating token.
    return cmd;
}

/*
 * Parses pipelines: cmd1 | cmd2 | cmd3 ...
 * Calls parse_simple_command to get the individual command segments.
 * Updates *tok_p to point past the tokens consumed by the pipeline.
 * Returns the root node of the pipeline (either a simple command or a NODE_PIPE).
 */
struct node_s *parse_pipeline(struct token_s **tok_p)
{
    // Check for invalid initial token
    if (!tok_p || !(*tok_p) || (*tok_p) == &eof_token) {
        return NULL;
    }

    // 1. Parse the first command segment in the potential pipeline
    struct node_s *left_cmd = parse_simple_command(tok_p);
    if (!left_cmd) {
        // Error should have been printed by parse_simple_command or tokenizer
        return NULL;
    }

    // 2. Check if a pipe symbol follows
    if ((*tok_p) && (*tok_p) != &eof_token && (*tok_p)->text && strcmp((*tok_p)->text, "|") == 0)
    {
        struct source_s *src = (*tok_p)->src; // Get source before freeing token
        struct token_s *pipe_token = *tok_p; // Save pointer to the '|' token

        // Get the token AFTER the pipe symbol
        *tok_p = tokenize(src);

        // Free the saved '|' token *after* getting the next one
        if (pipe_token && pipe_token != &eof_token) {
            free_token(pipe_token);
        }

        // Check for syntax error: missing command after pipe
        if (!(*tok_p) || (*tok_p) == &eof_token || !(*tok_p)->text || (*tok_p)->text[0] == '\n') {
             fprintf(stderr, "shell: syntax error: expected command after '|'\n");
             free_node_tree(left_cmd);
             if (*tok_p && *tok_p != &eof_token) free_token(*tok_p); // Free bad/newline token
             *tok_p = NULL;
             return NULL;
         }

        // 3. Recursively parse the rest of the pipeline (which might start with another command)
        struct node_s *right_pipeline = parse_pipeline(tok_p); // Recursive call
        if (!right_pipeline) {
            // Error parsing the right side
            fprintf(stderr, "shell: syntax error parsing command after '|'\n"); // Generic error
            free_node_tree(left_cmd);
            // Don't free *tok_p, it might point to the error location
            return NULL;
        }

        // 4. Create the PIPE node
        struct node_s *pipe_node = new_node(NODE_PIPE);
        if (!pipe_node) {
            perror("shell: new_node failed for pipe");
            free_node_tree(left_cmd);
            free_node_tree(right_pipeline);
            return NULL;
        }

        // 5. Link children: left_cmd is first, right_pipeline is second
        add_child_node(pipe_node, left_cmd);
        add_child_node(pipe_node, right_pipeline); // add_child handles linking siblings correctly

        return pipe_node; // Return the newly constructed pipe tree
    }
    else
    {
        // No pipe found after the first command, just return the simple command node
        return left_cmd;
    }
}


/*
 * Parses sequences connected by AND ('&&') and OR ('||').
 * Calls parse_pipeline to get the operands for AND/OR.
 * Handles left-associativity.
 * Updates *tok_p to point past the tokens consumed.
 * Returns the root node of the parsed structure.
 */
struct node_s *parse_and_or(struct token_s **tok_p) {
    // Check for invalid initial token
    if (!tok_p || !(*tok_p) || (*tok_p) == &eof_token) {
        return NULL;
    }

    // 1. Parse the first pipeline (left operand)
    struct node_s *left_node = parse_pipeline(tok_p);
    if (!left_node) {
        return NULL; // Error in parsing the first part
    }

    // 2. Loop while the next token is '&&' or '||'
    while ((*tok_p) && (*tok_p) != &eof_token && (*tok_p)->text &&
           (strcmp((*tok_p)->text, "&&") == 0 || strcmp((*tok_p)->text, "||") == 0))
    {
        enum node_type_e node_type = (strcmp((*tok_p)->text, "&&") == 0) ? NODE_AND : NODE_OR;
        char *op_text = strdup((*tok_p)->text); // Save operator text for error msg
        struct token_s *operator_token = *tok_p; // Save the operator token
        struct source_s *src = operator_token->src; // Get source before freeing

        // Consume the '&&' or '||' token
        *tok_p = tokenize(src);
        if (operator_token && operator_token != &eof_token) {
            free_token(operator_token);
        }

        // Check for syntax error (missing command after operator)
        if (!(*tok_p) || (*tok_p) == &eof_token || !(*tok_p)->text || (*tok_p)->text[0] == '\n') {
             fprintf(stderr, "shell: syntax error: expected command after '%s'\n",
                     op_text ? op_text : "");
            if (op_text) free(op_text);
            free_node_tree(left_node);
            if (*tok_p && *tok_p != &eof_token) free_token(*tok_p);
            *tok_p = NULL;
            return NULL;
        }
         if(op_text) free(op_text); // Free temporary copy

        // 3. Parse the next pipeline (right operand)
        struct node_s *right_node = parse_pipeline(tok_p);
        if (!right_node) {
            // Error parsing the right side
            free_node_tree(left_node);
            // Don't free *tok_p, may point to error location
            return NULL;
        }

        // 4. Create the AND/OR node
        struct node_s *new_node_op = new_node(node_type);
        if (!new_node_op) {
            perror("shell: new_node failed for AND/OR");
            free_node_tree(left_node);
            free_node_tree(right_node);
            return NULL;
        }

        // 5. Link children: Make the previous tree ('left_node') the left child
        //    and the newly parsed 'right_node' the right child.
        add_child_node(new_node_op, left_node);
        add_child_node(new_node_op, right_node);

        // 6. For the next iteration, the AND/OR node we just created becomes the 'left_node'.
        //    This implements left-associativity: (a && b) || c
        left_node = new_node_op;

    } // End while loop for && / ||

    // Return the final constructed tree root
    return left_node;
}