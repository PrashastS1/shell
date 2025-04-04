#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "shell.h"
#include "node.h"
#include "executor.h"
#include <signal.h> // Needed for kill() and SIGTERM
#include <sys/types.h> // Often needed for pid_t (likely already included via other headers, but good practice)

job_entry jobs_table[MAX_JOBS];

// --- Forward declaration for the dispatcher ---
int execute_node(struct node_s *node);

static char* reconstruct_cmd(int argc, char **argv) {
    if (argc == 0) return NULL;
    size_t total_len = 0;
    for (int i = 0; i < argc; i++) { // Iterate up to argc
        if (argv[i]) { // Check for NULL just in case
             total_len += strlen(argv[i]) + 1; // +1 for space or null terminator
        }
    }
    if (total_len == 0) return NULL;

    char *cmd_line = malloc(total_len);
    if (!cmd_line) return NULL;

    cmd_line[0] = '\0';
    for (int i = 0; i < argc; i++) {
        if (argv[i]) {
            strcat(cmd_line, argv[i]);
            if (i < argc - 1) { // Add space if not the last argument
                strcat(cmd_line, " ");
            }
        }
    }
    return cmd_line;
}

void init_job_table(void) {
    for (int i = 0; i < MAX_JOBS; i++) {
        jobs_table[i].pid = 0;
        jobs_table[i].jid = 0;
        jobs_table[i].cmd = NULL;
        jobs_table[i].state = JOB_STATE_UNKNOWN;
    }
}

void cleanup_job_table(void) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs_table[i].cmd != NULL) {
            free(jobs_table[i].cmd);
            jobs_table[i].cmd = NULL;
        }
         jobs_table[i].pid = 0;
    }
}

static int get_next_job_slot() {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs_table[i].pid == 0) { // Slot is free
            int current_jid = 1;
            int jid_found = 0;
            while(!jid_found) {
                int jid_in_use = 0;
                for(int j=0; j<MAX_JOBS; j++) {
                    if(jobs_table[j].pid != 0 && jobs_table[j].jid == current_jid) {
                        jid_in_use = 1;
                        break;
                    }
                }
                if (!jid_in_use) {
                    jobs_table[i].jid = current_jid;
                    return i; // Return the index of the free slot
                }
                current_jid++;
                 if (current_jid > MAX_JOBS * 2) {
                     fprintf(stderr, "shell: Error finding next JID\n");
                     return -1;
                 }
            }
        }
    }
    return -1; // No free slots
}

// Takes argv to reconstruct cmd, adds only if background
int add_job(pid_t pid, char **argv) {
    int slot = get_next_job_slot();
    if (slot == -1) {
        fprintf(stderr, "shell: maximum jobs (%d) reached\n", MAX_JOBS);
        kill(pid, SIGTERM); // Can't track it
        return 0;
    }

    // Find argc correctly
    int argc = 0;
    while(argv[argc] != NULL) argc++;

    jobs_table[slot].pid = pid;
    jobs_table[slot].state = JOB_STATE_RUNNING;
    jobs_table[slot].cmd = reconstruct_cmd(argc, argv); // Use helper
    if (jobs_table[slot].cmd == NULL) {
         perror("shell: strdup error in add_job");
         jobs_table[slot].pid = 0; // Mark slot free again
         return 0;
    }

    fprintf(stdout, "[%d] %d\n", jobs_table[slot].jid, pid);
    fflush(stdout);
    return jobs_table[slot].jid;
}

void remove_job_by_pid(pid_t pid) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs_table[i].pid == pid) {
            if (jobs_table[i].cmd) {
                free(jobs_table[i].cmd);
                jobs_table[i].cmd = NULL;
            }
            jobs_table[i].pid = 0;
            jobs_table[i].jid = 0;
            if (jobs_table[i].state == JOB_STATE_RUNNING) {
                 jobs_table[i].state = JOB_STATE_UNKNOWN;
            }
            return;
        }
    }
}

pid_t find_pid_by_jid(int jid) {
     for (int i = 0; i < MAX_JOBS; i++) {
         if (jobs_table[i].pid != 0 && jobs_table[i].jid == jid) {
             return jobs_table[i].pid;
         }
     }
     return 0; // Not found
}

int find_job_slot_by_pid(pid_t pid) {
     for (int i = 0; i < MAX_JOBS; i++) {
         if (jobs_table[i].pid == pid) {
             return i;
         }
     }
     return -1;
}

void update_job_statuses() {
    int status;
    pid_t child_pid;
    int job_updated = 0; // Flag to see if we need to flush stdout

    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs_table[i].pid != 0 && jobs_table[i].state == JOB_STATE_RUNNING) {
            child_pid = waitpid(jobs_table[i].pid, &status, WNOHANG | WUNTRACED); // Add WUNTRACED

            if (child_pid == jobs_table[i].pid) {
                if (WIFEXITED(status)) {
                    fprintf(stdout, "[%d]+ Done\t\t%s\n", jobs_table[i].jid, jobs_table[i].cmd);
                    jobs_table[i].state = JOB_STATE_DONE;
                    job_updated = 1;
                    // Don't remove here - let jobs/fg handle cleanup for simplicity
                } else if (WIFSIGNALED(status)) {
                     fprintf(stdout, "[%d]+ Terminated\t%s (Signal %d)\n", jobs_table[i].jid, jobs_table[i].cmd, WTERMSIG(status));
                     jobs_table[i].state = JOB_STATE_TERMINATED;
                     job_updated = 1;
                    // Don't remove here
                }
                // else if (WIFSTOPPED(status)) { // Add later for full job control
                //     fprintf(stdout, "[%d]+ Stopped\t%s\n", jobs_table[i].jid, jobs_table[i].cmd);
                //     jobs_table[i].state = JOB_STATE_STOPPED;
                //     job_updated = 1;
                // }
            } else if (child_pid == -1 && errno != ECHILD && errno != EINTR) {
                perror("shell: waitpid error checking job status");
                jobs_table[i].state = JOB_STATE_UNKNOWN; // Mark as error
                // Consider removing if waitpid persistently fails for this job
                 remove_job_by_pid(jobs_table[i].pid); // Remove to prevent loops
                 job_updated = 1; // Error message is output
            }
        }
    }
    if (job_updated) {
        fflush(stdout); // Flush if any status change was printed
    }
}

// --- End Job Helper Implementations ---

char *search_path(char *file)
{
    char *PATH = getenv("PATH");
    char *p    = PATH;
    char *p2;
    
    while(p && *p)
    {
        p2 = p;

        while(*p2 && *p2 != ':')
        {
            p2++;
        }
        
	int  plen = p2-p;
        if(!plen)
        {
            plen = 1;
        }
        
        int  alen = strlen(file);
        char path[plen+1+alen+1];
        
	strncpy(path, p, p2-p);
        path[p2-p] = '\0';
        
	if(p2[-1] != '/')
        {
            strcat(path, "/");
        }

        strcat(path, file);
        
	struct stat st;
        if(stat(path, &st) == 0)
        {
            if(!S_ISREG(st.st_mode))
            {
                errno = ENOENT;
                p = p2;
                if(*p2 == ':')
                {
                    p++;
                }
                continue;
            }

            p = malloc(strlen(path)+1);
            if(!p)
            {
                return NULL;
            }
            
	    strcpy(p, path);
            return p;
        }
        else    /* file not found */
        {
            p = p2;
            if(*p2 == ':')
            {
                p++;
            }
        }
    }

    errno = ENOENT;
    return NULL;
}


int do_exec_cmd(int argc, char **argv)
{
    if(strchr(argv[0], '/'))
    {
        execv(argv[0], argv);
    }
    else
    {
        char *path = search_path(argv[0]);
        if(!path)
        {
            return 0;
        }
        execv(path, argv);
        free(path);
    }
    return 0;
}


static inline void free_argv(int argc, char **argv)
{
    if(!argc)
    {
        return;
    }

    while(argc--)
    {
        free(argv[argc]);
    }
}


// int do_simple_command(struct node_s *node)
// {
//     if(!node)
//     {
//         return 0;
//     }

//     struct node_s *child = node->first_child;
//     if(!child)
//     {
//         return 0;
//     }
    
//     int argc = 0;           /* arguments count */
//     int targc = 0;          /* total alloc'd arguments count */
//     char **argv = NULL;
//     char *str;

//     while(child)
//     {
//         str = child->val.str;
//         /*perform word expansion */
//         struct word_s *w = word_expand(str);
        
//         /* word expansion failed */
//         if(!w)
//         {
//             child = child->next_sibling;
//             continue;
//         }

//         /* add the words to the arguments list */
//         struct word_s *w2 = w;
//         while(w2)
//         {
//             if(check_buffer_bounds(&argc, &targc, &argv))
//             {
//                 str = malloc(strlen(w2->data)+1);
//                 if(str)
//                 {
//                     strcpy(str, w2->data);
//                     argv[argc++] = str;
//                 }
//             }
//             w2 = w2->next;
//         }
        
//         /* free the memory used by the expanded words */
//         free_all_words(w);
        
//         /* check the next word */
//         child = child->next_sibling;
//     }

//     /* even if arc == 0, we need to alloc memory for argv */
//     if(check_buffer_bounds(&argc, &targc, &argv))
//     {
//         /* NULL-terminate the array */
//         argv[argc] = NULL;
//     }

//     int i = 0;
//     for( ; i < builtins_count; i++)
//     {
//         if(strcmp(argv[0], builtins[i].name) == 0)
//         {
//             builtins[i].func(argc, argv);
//             free_buffer(argc, argv);
//             return 1;
//         }
//     }

//     pid_t child_pid = 0;
//     if((child_pid = fork()) == 0)
//     {
//         do_exec_cmd(argc, argv);
//         fprintf(stderr, "error: failed to execute command: %s\n", strerror(errno));
//         if(errno == ENOEXEC)
//         {
//             exit(126);
//         }
//         else if(errno == ENOENT)
//         {
//             exit(127);
//         }
//         else
//         {
//             exit(EXIT_FAILURE);
//         }
//     }
//     else if(child_pid < 0)
//     {
//         fprintf(stderr, "error: failed to fork command: %s\n", strerror(errno));
// 	free_buffer(argc, argv);
//         return 0;
//     }

//     int status = 0;
//     waitpid(child_pid, &status, 0);
//     free_buffer(argc, argv);
    
//     return 1;
// }

// --- Modify do_simple_command ---
int do_simple_command(struct node_s *node)
{
    if(!node) return 0;
    struct node_s *child = node->first_child;
    if(!child) return 0;

    int argc = 0;
    int targc = 0;
    char **argv = NULL;
    char *str;

    // Argument building loop (remains the same)
    while(child) {
        str = child->val.str;
        struct word_s *w = word_expand(str);
        if(!w) { child = child->next_sibling; continue; }
        struct word_s *w2 = w;
        while(w2) {
            if(check_buffer_bounds(&argc, &targc, &argv)) {
                str = malloc(strlen(w2->data)+1);
                if(str) {
                    strcpy(str, w2->data);
                    argv[argc++] = str;
                }
            }
            w2 = w2->next;
        }
        free_all_words(w);
        child = child->next_sibling;
    }
    if(!check_buffer_bounds(&argc, &targc, &argv)) {
        // Handle allocation failure during final NULL termination if necessary
        if (argv) free_buffer(argc, argv); // Use your cleanup function
        return 0; // Indicate failure
    }
    argv[argc] = NULL;

    // --- Check for NULL command after expansion ---
    if (argc == 0 || argv[0] == NULL) {
        free_buffer(argc, argv);
        return 1; // Empty command is not an error, just do nothing
    }

    // ---> Check for background execution '&' <---
    int background = 0;
    if (argc > 0 && strcmp(argv[argc - 1], "&") == 0) {
        background = 1;
        free(argv[argc - 1]); // Free the memory for "&" string
        argv[argc - 1] = NULL; // Remove "&" from arguments list
        argc--; // Decrement argument count
        if (argc == 0) { // Check if only "&" was entered after expansion
             fprintf(stderr, "shell: syntax error near unexpected token `&'\n");
             free_buffer(argc, argv); // Free the (now empty) argv buffer
             return 1; // Continue shell loop
        }
    }
    // ------------------------------------------

    // Built-in check loop (remains the same)
    for(int i = 0; i < builtins_count; i++) {
        if(strcmp(argv[0], builtins[i].name) == 0) {
            int result = builtins[i].func(argc, argv); // Call built-in
            free_buffer(argc, argv); // Cleanup arguments
            return result; // Return status from built-in (usually 1 to continue)
        }
    }

    // --- External Command Execution ---
    pid_t child_pid = 0;
    if((child_pid = fork()) == 0) {
        // Child process
        // Add signal handling setup here if needed (e.g., reset SIGINT)
        // Add setpgid() here for full job control

        // Use _exit() on failure inside child!
        if (do_exec_cmd(argc, argv) == 0) { // do_exec_cmd tries execv
             fprintf(stderr, "shell: command not found or failed to execute: %s (%s)\n", argv[0], strerror(errno));
              if(errno == ENOEXEC) _exit(126);
              else if(errno == ENOENT) _exit(127);
              else _exit(EXIT_FAILURE);
        }
         // Should not be reached if exec succeeds
         _exit(EXIT_FAILURE);

    } else if(child_pid < 0) {
        // Fork failed
        perror("shell: fork failed");
        free_buffer(argc, argv);
        return 0; // Indicate serious error? Or 1 to continue shell? Let's try 1.
    } else {
        // Parent process
        if (background) {
            // ---> Add job to table <---
            add_job(child_pid, argv);
            // Don't wait
        } else {
            // ---> Wait for foreground job <---
            int status = 0;
            // Add tcsetpgrp() here for full job control
            waitpid(child_pid, &status, WUNTRACED); // Use WUNTRACED
             // Add status checking / reporting here if needed (WIFEXITED, WIFSIGNALED, WIFSTOPPED)
             // Add tcsetpgrp() back to shell here for full job control
        }
    }

    // Cleanup arguments regardless of foreground/background
    free_buffer(argc, argv);
    return 1; // Indicate success, continue shell loop
} // --- End of do_simple_command ---

// ---> NEW Function to handle pipe execution <---
int do_pipe_command(struct node_s *node) {
    if (!node || node->type != NODE_PIPE || node->children != 2 || !node->first_child || !node->first_child->next_sibling) {
        fprintf(stderr, "shell: invalid pipe node structure for execution\n");
        return 0; // Indicate failure
    }

    struct node_s *cmd1_node = node->first_child;
    struct node_s *cmd2_node = node->first_child->next_sibling;

    int pipefd[2]; // pipefd[0] = read end, pipefd[1] = write end
    pid_t pid1, pid2;
    int status1 = 0, status2 = 0; // Initialize statuses

    // 1. Create the pipe
    if (pipe(pipefd) == -1) {
        perror("shell: pipe");
        return 0;
    }

    // 2. Fork for cmd1 (left side)
    pid1 = fork();
    if (pid1 < 0) {
        perror("shell: fork (cmd1)");
        close(pipefd[0]);
        close(pipefd[1]);
        return 0;
    }

    if (pid1 == 0) {
        // --- Child 1 (cmd1) ---
        close(pipefd[0]); // Child 1 doesn't read from the pipe

        // Redirect stdout to pipe write end, checking for error
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror("shell: dup2 stdout");
            close(pipefd[1]); // Close original write end
            _exit(EXIT_FAILURE);
        }
        close(pipefd[1]); // Close original write end after successful dup2

        // Execute the left-hand command node
        execute_node(cmd1_node); // Recursive call

        // If execute_node returns, it means exec failed in do_simple_command
        _exit(EXIT_FAILURE);
    }

    // 3. Fork for cmd2 (right side)
    pid2 = fork();
    if (pid2 < 0) {
        perror("shell: fork (cmd2)");
        close(pipefd[0]); // Close pipe fds
        close(pipefd[1]);
        // Attempt to clean up child 1
        kill(pid1, SIGTERM);
        waitpid(pid1, NULL, 0);
        return 0;
    }

    if (pid2 == 0) {
        // --- Child 2 (cmd2) ---
        close(pipefd[1]); // Child 2 doesn't write to the pipe

        // Redirect stdin to pipe read end, checking for error
        if (dup2(pipefd[0], STDIN_FILENO) == -1) {
            perror("shell: dup2 stdin");
            close(pipefd[0]); // Close original read end
            _exit(EXIT_FAILURE);
        }
        close(pipefd[0]); // Close original read end after successful dup2

        // Execute the right-hand command node
        execute_node(cmd2_node); // Recursive call

        // If execute_node returns, it means exec failed
        _exit(EXIT_FAILURE);
    }

    // 4. Parent Process
    // Close BOTH pipe ends in the parent *immediately* after forking children.
    close(pipefd[0]);
    close(pipefd[1]);

    // 5. Wait for both children
    // It's generally better to wait in order, but waiting for the
    // second one first helps determine the overall pipeline status ($?) easier.
    waitpid(pid2, &status2, 0);
    waitpid(pid1, &status1, 0);

    // TODO: Implement proper exit status ($?) handling based on status2
    // For now, return 1 indicating the shell should continue
    return 1;
}

// ---> NEW Top-level executor dispatcher function <---
int execute_node(struct node_s *node) {
    if (!node) {
        return 1; // Nothing to execute is considered success
    }

    switch (node->type) {
        case NODE_COMMAND:
            // Call your existing function for simple commands
            return do_simple_command(node);

        case NODE_PIPE:
            // Call the new pipe handling function
            return do_pipe_command(node);

        // Add cases for other node types later
        // case NODE_REDIRECT_OUT: ...
        // case NODE_SEQUENCE: ... // For ';'

        default:
            fprintf(stderr, "shell: unknown node type in execute_node: %d\n", node->type);
            return 0; // Indicate failure
    }
}

