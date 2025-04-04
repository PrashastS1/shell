#include <unistd.h>
#include <string.h> // For strcmp
#include <stdio.h>  // For fprintf
#include "shell.h"
#include "parser.h"
#include "scanner.h"
#include "node.h"
#include "source.h"
#include <fcntl.h>
#include <stdlib.h>

// Helper function prototype (implement below or use existing if available)
struct token_s *consume_token(struct token_s **tok_p, struct source_s *src);

// Forward declaration for the new function
struct node_s *parse_pipeline(struct token_s **tok_p);

// struct node_s *parse_simple_command(struct token_s *tok)
// {
//     if(!tok)
//     {
//         return NULL;
//     }
    
//     struct node_s *cmd = new_node(NODE_COMMAND);
//     if(!cmd)
//     {
//         free_token(tok);
//         return NULL;
//     }
    
//     struct source_s *src = tok->src;
    
//     do
//     {
//         if(tok->text[0] == '\n')
//         {
//             free_token(tok);
//             break;
//         }

//         struct node_s *word = new_node(NODE_VAR);
//         if(!word)
//         {
//             free_node_tree(cmd);
//             free_token(tok);
//             return NULL;
//         }
//         set_node_val_str(word, tok->text);
//         add_child_node(cmd, word);

//         free_token(tok);

//     } while((tok = tokenize(src)) != &eof_token);

//     return cmd;
// }

struct node_s *parse_simple_command(struct token_s **tok_p)
{

    // fprintf(stderr, "DEBUG: parse_simple_command called with token: %s\n", (*tok_p) ? (*tok_p)->text : "(NULL)");


    if (!tok_p || !(*tok_p) || (*tok_p) == &eof_token) {
        return NULL;
    }

    struct node_s *cmd = new_node(NODE_COMMAND);
    if (!cmd) {
        // No need to free token here, caller handles it
        return NULL;
    }

    struct source_s *src = (*tok_p)->src;

    // Loop while the current token is not EOF, newline, or pipe
    // while ((*tok_p) != &eof_token && (*tok_p)->text[0] != '\n' && strcmp((*tok_p)->text, "|") != 0)
    // {
    //     struct node_s *word = new_node(NODE_VAR);
    //     if (!word) {
    //         free_node_tree(cmd);
    //         // Don't free the current token (*tok_p), let caller handle
    //         return NULL;
    //     }
    //     set_node_val_str(word, (*tok_p)->text);
    //     add_child_node(cmd, word);

    //     // Consume the current token and get the next one
    //     free_token(*tok_p);
    //     *tok_p = tokenize(src);
    //     if (!(*tok_p)) { // Check if tokenize returned NULL (error?)
    //          fprintf(stderr, "shell: error tokenizing input\n");
    //          free_node_tree(cmd);
    //          return NULL; // Indicate error
    //     }
    // }

    while ((*tok_p) != &eof_token && (*tok_p)->text[0] != '\n' && strcmp((*tok_p)->text, "|") != 0 )
     {
        // ---> Check for redirection operators <---
        if (strcmp((*tok_p)->text, "<") == 0) {
            // Input Redirection
            free_token(*tok_p); // Consume '<'
            *tok_p = tokenize(src);
            if (!(*tok_p) || (*tok_p) == &eof_token || (*tok_p)->text[0] == '\n' || strcmp((*tok_p)->text, "|") == 0 || strcmp((*tok_p)->text, ">") == 0 || strcmp((*tok_p)->text, ">>") == 0 || strcmp((*tok_p)->text, "<") == 0) {
                fprintf(stderr, "shell: syntax error: expected filename after '<'\n");
                free_node_tree(cmd);
                if(*tok_p) free_token(*tok_p);
                return NULL;
            }
            // Store infile name (handle potential previous infile?)
            if (cmd->infile) free(cmd->infile); // Overwrite previous if any
            cmd->infile = strdup((*tok_p)->text);
            if (!cmd->infile) { perror("shell: strdup infile"); free_node_tree(cmd); free_token(*tok_p); return NULL; }
            // Consume filename token
            free_token(*tok_p);
            *tok_p = tokenize(src);

        } else if (strcmp((*tok_p)->text, ">") == 0 || strcmp((*tok_p)->text, ">>") == 0) {
            // Output Redirection (Overwrite or Append)
            int is_append = (strcmp((*tok_p)->text, ">>") == 0);
            free_token(*tok_p); // Consume '>' or '>>'
            *tok_p = tokenize(src);
             if (!(*tok_p) || (*tok_p) == &eof_token || (*tok_p)->text[0] == '\n' || strcmp((*tok_p)->text, "|") == 0 || strcmp((*tok_p)->text, ">") == 0 || strcmp((*tok_p)->text, ">>") == 0 || strcmp((*tok_p)->text, "<") == 0) {
                fprintf(stderr, "shell: syntax error: expected filename after '%s'\n", is_append ? ">>" : ">");
                free_node_tree(cmd);
                if(*tok_p) free_token(*tok_p);
                return NULL;
            }
            // Store outfile name (handle potential previous outfile?)
            if (cmd->outfile) free(cmd->outfile); // Overwrite previous if any
            cmd->outfile = strdup((*tok_p)->text);
            if (!cmd->outfile) { perror("shell: strdup outfile"); free_node_tree(cmd); free_token(*tok_p); return NULL; }
            cmd->append_mode = is_append;
            // Consume filename token
            free_token(*tok_p);
            *tok_p = tokenize(src);

        } else {
            // ---> Regular command word (argument/command name) <---
             struct node_s *word = new_node(NODE_VAR);
             if (!word) {
                 free_node_tree(cmd);
                // Don't free *tok_p, loop will exit or continue
                 return NULL;
             }
             set_node_val_str(word, (*tok_p)->text);
             add_child_node(cmd, word);

             // Consume the current token and get the next one
             free_token(*tok_p);
             *tok_p = tokenize(src);
        } // End of if/else for redirection vs word

        if (!(*tok_p)) { // Check if tokenize returned NULL (error?)
             // This might happen after consuming a filename
            fprintf(stderr, "shell: error tokenizing input\n");
            free_node_tree(cmd);
             // Don't free cmd here, return what we have? Or signal error?
             // Let's return NULL for error.
            if(cmd) free_node_tree(cmd);
            return NULL; // Indicate error
        }
    }

    if (!cmd) return NULL;

    // // If we parsed no words, it's an empty command node (or error)
    // if (cmd->children == 0) {
    //     free_node_tree(cmd);
    //     return NULL;
    // }

    if (cmd->children == 0 && cmd->infile == NULL && cmd->outfile == NULL) {
          free_node_tree(cmd);
          return NULL;
     }

    return cmd;
}

/*
 * New function to parse pipelines: cmd1 | cmd2 | cmd3 ...
 * Updates *tok_p to point past the tokens consumed by the pipeline.
 */
struct node_s *parse_pipeline(struct token_s **tok_p)
{

    // fprintf(stderr, "DEBUG: parse_pipeline called with token: %s\n", (*tok_p) ? (*tok_p)->text : "(NULL)");

    if (!tok_p || !(*tok_p) || (*tok_p) == &eof_token) {
        return NULL;
    }

    // 1. Parse the first command in the pipeline
    struct node_s *left_cmd = parse_simple_command(tok_p);
    if (!left_cmd) {
        // Error message should come from parse_simple_command or tokenizer
        return NULL;
    }

    // 2. Check if a pipe follows
    if ((*tok_p) != &eof_token && strcmp((*tok_p)->text, "|") == 0)
    {

        // fprintf(stderr, "DEBUG: Found pipe symbol '|'\n"); // Add this
        // fflush(stderr);

        struct source_s *src = (*tok_p)->src;

        // Consume the '|' token
        // Ensure we don't free the static eof_token
        if (*tok_p && *tok_p != &eof_token) {
            free_token(*tok_p);
            *tok_p = NULL;
        } 
        
        // else if (*tok_p == &eof_token) {
        //      // Should not happen if pipe was matched, but safety check
        //     //  fprintf(stderr, "DEBUG: Unexpected EOF instead of pipe token!?\n");
        //     //  fflush(stderr);
        //      // Handle error appropriately - cannot proceed
        //      free_node_tree(left_cmd);
        //      return NULL;
        // }




        // Consume the '|' token
        // free_token(*tok_p);
        // *tok_p = tokenize((*tok_p)->src); // Get token after pipe
        *tok_p = tokenize(src);  

        // ---> Enhanced Debug Print <---
        // fprintf(stderr, "DEBUG: Token after '|' is: ");
        // if (!(*tok_p)) {
        //     fprintf(stderr, "(NULL token pointer)\n");
        // } else if ((*tok_p) == &eof_token) {
        //     fprintf(stderr, "(EOF token)\n");
        // } else if (!(*tok_p)->text) {
        //      fprintf(stderr, "(NULL text field)\n");
        // } else {
        //      fprintf(stderr, "'%s'\n", (*tok_p)->text);
        // }
        // fflush(stderr); // Force output

         if (!(*tok_p) || (*tok_p) == &eof_token) { // Check for error or EOF after '|'
             fprintf(stderr, "shell: syntax error: expected command after '|'\n");
             fflush(stderr);
             free_node_tree(left_cmd);
            //  if (*tok_p) free_token(*tok_p); // Free EOF token if exists
             return NULL;
         }
         if (!(*tok_p)->text || (*tok_p)->text[0] == '\n') { // Check for newline after '|'
              fprintf(stderr, "shell: syntax error: expected command after '|'\n");
              fflush(stderr);
              free_node_tree(left_cmd);
              free_token(*tok_p);
              *tok_p = NULL;
              return NULL;
         }


        // 3. Recursively parse the rest of the pipeline

        // fprintf(stderr, "DEBUG: Calling parse_pipeline recursively for right side...\n"); // Add this
        // fflush(stderr);

        struct node_s *right_pipeline = parse_pipeline(tok_p);
        if (!right_pipeline) {
            fprintf(stderr, "shell: syntax error parsing command after '|'\n");
            free_node_tree(left_cmd);
            // *tok_p might point to problematic token, don't free here
            return NULL;
        }

        // 4. Create the PIPE node
        struct node_s *pipe_node = new_node(NODE_PIPE);
        if (!pipe_node) {
            free_node_tree(left_cmd);
            free_node_tree(right_pipeline);
            return NULL;
        }

        // 5. Link children: left_cmd is first, right_pipeline is second
        add_child_node(pipe_node, left_cmd);
        add_child_node(pipe_node, right_pipeline); // add_child handles linking siblings

        return pipe_node; // Return the newly constructed pipe tree
    }
    else
    {
        // No pipe found, just return the single simple command
        return left_cmd;
    }
}