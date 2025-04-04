#include <unistd.h>
#include <string.h> // For strcmp
#include <stdio.h>  // For fprintf
#include "shell.h"
#include "parser.h"
#include "scanner.h"
#include "node.h"
#include "source.h"

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
    while ((*tok_p) != &eof_token && (*tok_p)->text[0] != '\n' && strcmp((*tok_p)->text, "|") != 0)
    {
        struct node_s *word = new_node(NODE_VAR);
        if (!word) {
            free_node_tree(cmd);
            // Don't free the current token (*tok_p), let caller handle
            return NULL;
        }
        set_node_val_str(word, (*tok_p)->text);
        add_child_node(cmd, word);

        // Consume the current token and get the next one
        free_token(*tok_p);
        *tok_p = tokenize(src);
        if (!(*tok_p)) { // Check if tokenize returned NULL (error?)
             fprintf(stderr, "shell: error tokenizing input\n");
             free_node_tree(cmd);
             return NULL; // Indicate error
        }
    }

    // If we parsed no words, it's an empty command node (or error)
    if (cmd->children == 0) {
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
        // Consume the '|' token
        free_token(*tok_p);
        *tok_p = tokenize((*tok_p)->src); // Get token after pipe
         if (!(*tok_p) || (*tok_p) == &eof_token) { // Check for error or EOF after '|'
             fprintf(stderr, "shell: syntax error: expected command after '|'\n");
             free_node_tree(left_cmd);
             if (*tok_p) free_token(*tok_p); // Free EOF token if exists
             return NULL;
         }
         if ((*tok_p)->text[0] == '\n') { // Check for newline after '|'
              fprintf(stderr, "shell: syntax error: expected command after '|'\n");
              free_node_tree(left_cmd);
              free_token(*tok_p);
              return NULL;
         }


        // 3. Recursively parse the rest of the pipeline
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