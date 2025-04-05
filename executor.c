// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <string.h>
// #include <errno.h>
// #include <sys/stat.h>
// #include <sys/wait.h>
// #include "shell.h"
// #include "node.h"
// #include "executor.h"
// #include <fcntl.h>
// #include <signal.h> // Needed for kill() and SIGTERM
// #include <sys/types.h> // Often needed for pid_t (likely already included via other headers, but good practice)

// job_entry jobs_table[MAX_JOBS];

// // --- Forward declaration for the dispatcher ---
// int execute_node(struct node_s *node);

// static char* reconstruct_cmd(int argc, char **argv) {
//     if (argc == 0) return NULL;
//     size_t total_len = 0;
//     for (int i = 0; i < argc; i++) { // Iterate up to argc
//         if (argv[i]) { // Check for NULL just in case
//              total_len += strlen(argv[i]) + 1; // +1 for space or null terminator
//         }
//     }
//     if (total_len == 0) return NULL;

//     char *cmd_line = malloc(total_len);
//     if (!cmd_line) return NULL;

//     cmd_line[0] = '\0';
//     for (int i = 0; i < argc; i++) {
//         if (argv[i]) {
//             strcat(cmd_line, argv[i]);
//             if (i < argc - 1) { // Add space if not the last argument
//                 strcat(cmd_line, " ");
//             }
//         }
//     }
//     return cmd_line;
// }

// void init_job_table(void) {
//     for (int i = 0; i < MAX_JOBS; i++) {
//         jobs_table[i].pid = 0;
//         jobs_table[i].jid = 0;
//         jobs_table[i].cmd = NULL;
//         jobs_table[i].state = JOB_STATE_UNKNOWN;
//     }
// }

// void cleanup_job_table(void) {
//     for (int i = 0; i < MAX_JOBS; i++) {
//         if (jobs_table[i].cmd != NULL) {
//             free(jobs_table[i].cmd);
//             jobs_table[i].cmd = NULL;
//         }
//          jobs_table[i].pid = 0;
//     }
// }

// static int get_next_job_slot() {
//     for (int i = 0; i < MAX_JOBS; i++) {
//         if (jobs_table[i].pid == 0) { // Slot is free
//             int current_jid = 1;
//             int jid_found = 0;
//             while(!jid_found) {
//                 int jid_in_use = 0;
//                 for(int j=0; j<MAX_JOBS; j++) {
//                     if(jobs_table[j].pid != 0 && jobs_table[j].jid == current_jid) {
//                         jid_in_use = 1;
//                         break;
//                     }
//                 }
//                 if (!jid_in_use) {
//                     jobs_table[i].jid = current_jid;
//                     return i; // Return the index of the free slot
//                 }
//                 current_jid++;
//                  if (current_jid > MAX_JOBS * 2) {
//                      fprintf(stderr, "shell: Error finding next JID\n");
//                      return -1;
//                  }
//             }
//         }
//     }
//     return -1; // No free slots
// }

// // Takes argv to reconstruct cmd, adds only if background
// int add_job(pid_t pid, char **argv) {
//     int slot = get_next_job_slot();
//     if (slot == -1) {
//         fprintf(stderr, "shell: maximum jobs (%d) reached\n", MAX_JOBS);
//         kill(pid, SIGTERM); // Can't track it
//         return 0;
//     }

//     // Find argc correctly
//     int argc = 0;
//     while(argv[argc] != NULL) argc++;

//     jobs_table[slot].pid = pid;
//     jobs_table[slot].state = JOB_STATE_RUNNING;
//     jobs_table[slot].cmd = reconstruct_cmd(argc, argv); // Use helper
//     if (jobs_table[slot].cmd == NULL) {
//          perror("shell: strdup error in add_job");
//          jobs_table[slot].pid = 0; // Mark slot free again
//          return 0;
//     }

//     fprintf(stdout, "[%d] %d\n", jobs_table[slot].jid, pid);
//     fflush(stdout);
//     return jobs_table[slot].jid;
// }

// void remove_job_by_pid(pid_t pid) {
//     for (int i = 0; i < MAX_JOBS; i++) {
//         if (jobs_table[i].pid == pid) {
//             if (jobs_table[i].cmd) {
//                 free(jobs_table[i].cmd);
//                 jobs_table[i].cmd = NULL;
//             }
//             jobs_table[i].pid = 0;
//             jobs_table[i].jid = 0;
//             if (jobs_table[i].state == JOB_STATE_RUNNING) {
//                  jobs_table[i].state = JOB_STATE_UNKNOWN;
//             }
//             return;
//         }
//     }
// }

// pid_t find_pid_by_jid(int jid) {
//      for (int i = 0; i < MAX_JOBS; i++) {
//          if (jobs_table[i].pid != 0 && jobs_table[i].jid == jid) {
//              return jobs_table[i].pid;
//          }
//      }
//      return 0; // Not found
// }

// int find_job_slot_by_pid(pid_t pid) {
//      for (int i = 0; i < MAX_JOBS; i++) {
//          if (jobs_table[i].pid == pid) {
//              return i;
//          }
//      }
//      return -1;
// }

// void update_job_statuses() {
//     int status;
//     pid_t child_pid;
//     int job_updated = 0; // Flag to see if we need to flush stdout

//     for (int i = 0; i < MAX_JOBS; i++) {
//         if (jobs_table[i].pid != 0 && jobs_table[i].state == JOB_STATE_RUNNING) {
//             child_pid = waitpid(jobs_table[i].pid, &status, WNOHANG | WUNTRACED); // Add WUNTRACED

//             if (child_pid == jobs_table[i].pid) {
//                 if (WIFEXITED(status)) {
//                     fprintf(stdout, "[%d]+ Done\t\t%s\n", jobs_table[i].jid, jobs_table[i].cmd);
//                     jobs_table[i].state = JOB_STATE_DONE;
//                     job_updated = 1;
//                     // Don't remove here - let jobs/fg handle cleanup for simplicity
//                 } else if (WIFSIGNALED(status)) {
//                      fprintf(stdout, "[%d]+ Terminated\t%s (Signal %d)\n", jobs_table[i].jid, jobs_table[i].cmd, WTERMSIG(status));
//                      jobs_table[i].state = JOB_STATE_TERMINATED;
//                      job_updated = 1;
//                     // Don't remove here
//                 }
//                 // else if (WIFSTOPPED(status)) { // Add later for full job control
//                 //     fprintf(stdout, "[%d]+ Stopped\t%s\n", jobs_table[i].jid, jobs_table[i].cmd);
//                 //     jobs_table[i].state = JOB_STATE_STOPPED;
//                 //     job_updated = 1;
//                 // }
//             } else if (child_pid == -1 && errno != ECHILD && errno != EINTR) {
//                 perror("shell: waitpid error checking job status");
//                 jobs_table[i].state = JOB_STATE_UNKNOWN; // Mark as error
//                 // Consider removing if waitpid persistently fails for this job
//                  remove_job_by_pid(jobs_table[i].pid); // Remove to prevent loops
//                  job_updated = 1; // Error message is output
//             }
//         }
//     }
//     if (job_updated) {
//         fflush(stdout); // Flush if any status change was printed
//     }
// }

// // --- End Job Helper Implementations ---

// char *search_path(char *file)
// {
//     char *PATH = getenv("PATH");
//     char *p    = PATH;
//     char *p2;
    
//     while(p && *p)
//     {
//         p2 = p;

//         while(*p2 && *p2 != ':')
//         {
//             p2++;
//         }
        
// 	int  plen = p2-p;
//         if(!plen)
//         {
//             plen = 1;
//         }
        
//         int  alen = strlen(file);
//         char path[plen+1+alen+1];
        
// 	strncpy(path, p, p2-p);
//         path[p2-p] = '\0';
        
// 	if(p2[-1] != '/')
//         {
//             strcat(path, "/");
//         }

//         strcat(path, file);
        
// 	struct stat st;
//         if(stat(path, &st) == 0)
//         {
//             if(!S_ISREG(st.st_mode))
//             {
//                 errno = ENOENT;
//                 p = p2;
//                 if(*p2 == ':')
//                 {
//                     p++;
//                 }
//                 continue;
//             }

//             p = malloc(strlen(path)+1);
//             if(!p)
//             {
//                 return NULL;
//             }
            
// 	    strcpy(p, path);
//             return p;
//         }
//         else    /* file not found */
//         {
//             p = p2;
//             if(*p2 == ':')
//             {
//                 p++;
//             }
//         }
//     }

//     errno = ENOENT;
//     return NULL;
// }


// int do_exec_cmd(int argc, char **argv)
// {
//     if(strchr(argv[0], '/'))
//     {
//         execv(argv[0], argv);
//     }
//     else
//     {
//         char *path = search_path(argv[0]);
//         if(!path)
//         {
//             return 0;
//         }
//         execv(path, argv);
//         free(path);
//     }
//     return 0;
// }


// static inline void free_argv(int argc, char **argv)
// {
//     if(!argc)
//     {
//         return;
//     }

//     while(argc--)
//     {
//         free(argv[argc]);
//     }
// }


// // int do_simple_command(struct node_s *node)
// // {
// //     if(!node)
// //     {
// //         return 0;
// //     }

// //     struct node_s *child = node->first_child;
// //     if(!child)
// //     {
// //         return 0;
// //     }
    
// //     int argc = 0;           /* arguments count */
// //     int targc = 0;          /* total alloc'd arguments count */
// //     char **argv = NULL;
// //     char *str;

// //     while(child)
// //     {
// //         str = child->val.str;
// //         /*perform word expansion */
// //         struct word_s *w = word_expand(str);
        
// //         /* word expansion failed */
// //         if(!w)
// //         {
// //             child = child->next_sibling;
// //             continue;
// //         }

// //         /* add the words to the arguments list */
// //         struct word_s *w2 = w;
// //         while(w2)
// //         {
// //             if(check_buffer_bounds(&argc, &targc, &argv))
// //             {
// //                 str = malloc(strlen(w2->data)+1);
// //                 if(str)
// //                 {
// //                     strcpy(str, w2->data);
// //                     argv[argc++] = str;
// //                 }
// //             }
// //             w2 = w2->next;
// //         }
        
// //         /* free the memory used by the expanded words */
// //         free_all_words(w);
        
// //         /* check the next word */
// //         child = child->next_sibling;
// //     }

// //     /* even if arc == 0, we need to alloc memory for argv */
// //     if(check_buffer_bounds(&argc, &targc, &argv))
// //     {
// //         /* NULL-terminate the array */
// //         argv[argc] = NULL;
// //     }

// //     int i = 0;
// //     for( ; i < builtins_count; i++)
// //     {
// //         if(strcmp(argv[0], builtins[i].name) == 0)
// //         {
// //             builtins[i].func(argc, argv);
// //             free_buffer(argc, argv);
// //             return 1;
// //         }
// //     }

// //     pid_t child_pid = 0;
// //     if((child_pid = fork()) == 0)
// //     {
// //         do_exec_cmd(argc, argv);
// //         fprintf(stderr, "error: failed to execute command: %s\n", strerror(errno));
// //         if(errno == ENOEXEC)
// //         {
// //             exit(126);
// //         }
// //         else if(errno == ENOENT)
// //         {
// //             exit(127);
// //         }
// //         else
// //         {
// //             exit(EXIT_FAILURE);
// //         }
// //     }
// //     else if(child_pid < 0)
// //     {
// //         fprintf(stderr, "error: failed to fork command: %s\n", strerror(errno));
// // 	free_buffer(argc, argv);
// //         return 0;
// //     }

// //     int status = 0;
// //     waitpid(child_pid, &status, 0);
// //     free_buffer(argc, argv);
    
// //     return 1;
// // }

// // --- Modify do_simple_command ---
// int do_simple_command(struct node_s *node)
// {
//     if(!node) return 0;
//     struct node_s *child = node->first_child;
//     if(!child) return 0;

//     int argc = 0;
//     int targc = 0;
//     char **argv = NULL;
//     char *str;

//     // Argument building loop (remains the same)
//     while(child) {
//         str = child->val.str;
//         struct word_s *w = word_expand(str);
//         if(!w) { child = child->next_sibling; continue; }
//         struct word_s *w2 = w;
//         while(w2) {
//             if(check_buffer_bounds(&argc, &targc, &argv)) {
//                 str = malloc(strlen(w2->data)+1);
//                 if(str) {
//                     strcpy(str, w2->data);
//                     argv[argc++] = str;
//                 }
//             }
//             w2 = w2->next;
//         }
//         free_all_words(w);
//         child = child->next_sibling;
//     }
//     if(!check_buffer_bounds(&argc, &targc, &argv)) {
//         // Handle allocation failure during final NULL termination if necessary
//         if (argv) free_buffer(argc, argv); // Use your cleanup function
//         return 0; // Indicate failure
//     }
//     argv[argc] = NULL;

//     // --- Check for NULL command after expansion ---
//     if (argc == 0 || argv[0] == NULL) {
//         free_buffer(argc, argv);
//         return 1; // Empty command is not an error, just do nothing
//     }

//     // ---> Check for background execution '&' <---
//     int background = 0;
//     if (argc > 0 && strcmp(argv[argc - 1], "&") == 0) {
//         background = 1;
//         free(argv[argc - 1]); // Free the memory for "&" string
//         argv[argc - 1] = NULL; // Remove "&" from arguments list
//         argc--; // Decrement argument count
//         if (argc == 0) { // Check if only "&" was entered after expansion
//              fprintf(stderr, "shell: syntax error near unexpected token `&'\n");
//              free_buffer(argc, argv); // Free the (now empty) argv buffer
//              return 1; // Continue shell loop
//         }
//     }
//     // ------------------------------------------

//     // Built-in check loop (remains the same)
//     for(int i = 0; i < builtins_count; i++) {
//         if(strcmp(argv[0], builtins[i].name) == 0) {
//             int result = builtins[i].func(argc, argv); // Call built-in
//             free_buffer(argc, argv); // Cleanup arguments
//             return result; // Return status from built-in (usually 1 to continue)
//         }
//     }

//     // --- External Command Execution ---
//     pid_t child_pid = 0;
//     if((child_pid = fork()) == 0) {
//         // Child process
//         // Add signal handling setup here if needed (e.g., reset SIGINT)
//         // Add setpgid() here for full job control

//         int input_fd = -1;
//         int output_fd = -1;
//         struct node_s *command_node = node;

//         if (command_node->infile) {
//             input_fd = open(command_node->infile, O_RDONLY);
//             if (input_fd < 0) {
//                 fprintf(stderr, "shell: failed to open input file '%s': %s\n",
//                         command_node->infile, strerror(errno));
//                 _exit(EXIT_FAILURE); // Use _exit in child
//             }
//             // Redirect stdin (0) to input_fd
//             if (dup2(input_fd, STDIN_FILENO) < 0) {
//                 perror("shell: dup2 stdin failed");
//                 close(input_fd); // Close original fd
//                 _exit(EXIT_FAILURE);
//             }
//             close(input_fd); // Close original fd after successful dup2
//         }

//         // 2. Output Redirection (>, >>)
//         if (command_node->outfile) {
//             int open_flags = O_WRONLY | O_CREAT;
//             if (command_node->append_mode) {
//                 open_flags |= O_APPEND; // Append mode >>
//             } else {
//                 open_flags |= O_TRUNC;  // Overwrite mode >
//             }
//             // Set standard file permissions (rw-r--r--)
//             mode_t file_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

//             output_fd = open(command_node->outfile, open_flags, file_mode);
//             if (output_fd < 0) {
//                  fprintf(stderr, "shell: failed to open output file '%s': %s\n",
//                          command_node->outfile, strerror(errno));
//                  // Don't need to close input_fd here as it's already closed/dup'd
//                  _exit(EXIT_FAILURE);
//             }
//             // Redirect stdout (1) to output_fd
//             if (dup2(output_fd, STDOUT_FILENO) < 0) {
//                 perror("shell: dup2 stdout failed");
//                 close(output_fd);
//                 _exit(EXIT_FAILURE);
//             }
//             close(output_fd); // Close original fd after successful dup2
//         }
//         // --- Redirection Setup Complete ---

//         // Use _exit() on failure inside child!
//         if (do_exec_cmd(argc, argv) == 0) { // do_exec_cmd tries execv
//              fprintf(stderr, "shell: command not found or failed to execute: %s (%s)\n", argv[0], strerror(errno));
//               if(errno == ENOEXEC) _exit(126);
//               else if(errno == ENOENT) _exit(127);
//               else _exit(EXIT_FAILURE);
//         }
//          // Should not be reached if exec succeeds
//          _exit(EXIT_FAILURE);

//     } else if(child_pid < 0) {
//         // Fork failed
//         perror("shell: fork failed");
//         free_buffer(argc, argv);
//         return 0; // Indicate serious error? Or 1 to continue shell? Let's try 1.
//     } else {
//         // Parent process
//         if (background) {
//             // ---> Add job to table <---
//             add_job(child_pid, argv);
//             // Don't wait
//         } else {
//             // ---> Wait for foreground job <---
//             int status = 0;
//             // Add tcsetpgrp() here for full job control
//             waitpid(child_pid, &status, WUNTRACED); // Use WUNTRACED
//              // Add status checking / reporting here if needed (WIFEXITED, WIFSIGNALED, WIFSTOPPED)
//              // Add tcsetpgrp() back to shell here for full job control
//         }
//     }

//     // Cleanup arguments regardless of foreground/background
//     free_buffer(argc, argv);
//     return 1; // Indicate success, continue shell loop
// } // --- End of do_simple_command ---

// // ---> NEW Function to handle pipe execution <---
// int do_pipe_command(struct node_s *node) {
//     if (!node || node->type != NODE_PIPE || node->children != 2 || !node->first_child || !node->first_child->next_sibling) {
//         fprintf(stderr, "shell: invalid pipe node structure for execution\n");
//         return 0; // Indicate failure
//     }

//     struct node_s *cmd1_node = node->first_child;
//     struct node_s *cmd2_node = node->first_child->next_sibling;

//     int pipefd[2]; // pipefd[0] = read end, pipefd[1] = write end
//     pid_t pid1, pid2;
//     int status1 = 0, status2 = 0; // Initialize statuses

//     // 1. Create the pipe
//     if (pipe(pipefd) == -1) {
//         perror("shell: pipe");
//         return 0;
//     }

//     // 2. Fork for cmd1 (left side)
//     pid1 = fork();
//     if (pid1 < 0) {
//         perror("shell: fork (cmd1)");
//         close(pipefd[0]);
//         close(pipefd[1]);
//         return 0;
//     }

//     if (pid1 == 0) {
//         // --- Child 1 (cmd1) ---
//         close(pipefd[0]); // Child 1 doesn't read from the pipe

//         // Redirect stdout to pipe write end, checking for error
//         if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
//             perror("shell: dup2 stdout");
//             close(pipefd[1]); // Close original write end
//             _exit(EXIT_FAILURE);
//         }
//         close(pipefd[1]); // Close original write end after successful dup2

//         // Execute the left-hand command node
//         execute_node(cmd1_node); // Recursive call

//         // If execute_node returns, it means exec failed in do_simple_command
//         _exit(EXIT_FAILURE);
//     }

//     // 3. Fork for cmd2 (right side)
//     pid2 = fork();
//     if (pid2 < 0) {
//         perror("shell: fork (cmd2)");
//         close(pipefd[0]); // Close pipe fds
//         close(pipefd[1]);
//         // Attempt to clean up child 1
//         kill(pid1, SIGTERM);
//         waitpid(pid1, NULL, 0);
//         return 0;
//     }

//     if (pid2 == 0) {
//         // --- Child 2 (cmd2) ---
//         close(pipefd[1]); // Child 2 doesn't write to the pipe

//         // Redirect stdin to pipe read end, checking for error
//         if (dup2(pipefd[0], STDIN_FILENO) == -1) {
//             perror("shell: dup2 stdin");
//             close(pipefd[0]); // Close original read end
//             _exit(EXIT_FAILURE);
//         }
//         close(pipefd[0]); // Close original read end after successful dup2

//         // Execute the right-hand command node
//         execute_node(cmd2_node); // Recursive call

//         // If execute_node returns, it means exec failed
//         _exit(EXIT_FAILURE);
//     }

//     // 4. Parent Process
//     // Close BOTH pipe ends in the parent *immediately* after forking children.
//     close(pipefd[0]);
//     close(pipefd[1]);

//     // 5. Wait for both children
//     // It's generally better to wait in order, but waiting for the
//     // second one first helps determine the overall pipeline status ($?) easier.
//     waitpid(pid2, &status2, 0);
//     waitpid(pid1, &status1, 0);

//     // TODO: Implement proper exit status ($?) handling based on status2
//     // For now, return 1 indicating the shell should continue
//     return 1;
// }

// // ---> NEW Top-level executor dispatcher function <---
// int execute_node(struct node_s *node) {
//     if (!node) {
//         return 1; // Nothing to execute is considered success
//     }

//     switch (node->type) {
//         case NODE_COMMAND:
//             // Call your existing function for simple commands
//             return do_simple_command(node);

//         case NODE_PIPE:
//             // Call the new pipe handling function
//             return do_pipe_command(node);

//         // Add cases for other node types later
//         // case NODE_REDIRECT_OUT: ...
//         // case NODE_SEQUENCE: ... // For ';'

//         default:
//             fprintf(stderr, "shell: unknown node type in execute_node: %d\n", node->type);
//             return 0; // Indicate failure
//     }
// }

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>      // For pipe, dup2, close, fork, _exit, exec*, access, chdir, getcwd
#include <string.h>      // For strcmp, strerror, strdup, strchr, strcat, strncpy, strlen, memcpy
#include <errno.h>       // For errno
#include <sys/stat.h>    // For stat, S_ISREG, S_IRUSR, etc.
#include <sys/wait.h>    // For waitpid, WIFEXITED, WEXITSTATUS, etc.
#include <fcntl.h>       // For open, O_RDONLY, O_WRONLY, O_CREAT, O_TRUNC, O_APPEND
#include <signal.h>      // For kill, SIGTERM

#include "shell.h"       // Main header
#include "node.h"        // Node structure definitions
#include "executor.h"    // Self header (optional)


// --- Global Job Table (Defined Here) ---
job_entry jobs_table[MAX_JOBS];
// ---------------------------------------

// --- Forward Declarations ---
static char* reconstruct_cmd(int argc, char **argv);
static int get_next_job_slot(void);
int execute_node(struct node_s *node); // Main dispatcher
int do_simple_command(struct node_s *node);
int do_pipe_command(struct node_s *node);
char *search_path(char *file);
int do_exec_cmd(int argc, char **argv);


// --- Job Helper Function Implementations ---

// Helper to rebuild command string for job table display
static char* reconstruct_cmd(int argc, char **argv) {
    if (argc == 0 || !argv || !argv[0]) return NULL;
    size_t total_len = 0;
    for (int i = 0; i < argc; i++) {
        if (argv[i]) {
             total_len += strlen(argv[i]) + 1; // +1 for space or null terminator
        } else {
            break; // Stop if we hit NULL early (shouldn't happen with argc)
        }
    }
    if (total_len == 0) return strdup(""); // Return empty string if no args somehow

    char *cmd_line = malloc(total_len);
    if (!cmd_line) return NULL;

    cmd_line[0] = '\0';
    for (int i = 0; i < argc; i++) {
         if (argv[i]) {
            strcat(cmd_line, argv[i]);
            if (i < argc - 1) { // Add space only between args
                strcat(cmd_line, " ");
            }
        } else {
            break;
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
    // Optional: could send SIGHUP or SIGTERM to remaining jobs?
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs_table[i].cmd != NULL) {
            free(jobs_table[i].cmd);
            jobs_table[i].cmd = NULL;
        }
         jobs_table[i].pid = 0; // Mark as free
    }
}

// Finds the next available slot index and assigns the lowest available JID
static int get_next_job_slot(void) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs_table[i].pid == 0) { // Slot is free
            int current_jid = 1;
            while(1) { // Loop to find the next available JID
                int jid_in_use = 0;
                for(int j = 0; j < MAX_JOBS; j++) {
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
                 // Safety break for extremely fragmented JIDs (unlikely)
                 if (current_jid > MAX_JOBS * 5) {
                     fprintf(stderr, "shell: Error finding next available JID\n");
                     return -1;
                 }
            } // End while(1)
        } // End if slot free
    } // End for loop
    return -1; // No free slots
}

// Adds a background job to the table
int add_job(pid_t pid, char **argv) {
    int slot = get_next_job_slot();
    if (slot == -1) {
        fprintf(stderr, "shell: maximum background jobs (%d) reached\n", MAX_JOBS);
        kill(pid, SIGTERM); // Can't track it, try to terminate
        return 0;
    }

    // Find argc correctly (needed for reconstruct_cmd)
    int argc = 0;
    if (argv) {
         while(argv[argc] != NULL) argc++;
    }

    jobs_table[slot].pid = pid;
    jobs_table[slot].state = JOB_STATE_RUNNING;
    jobs_table[slot].cmd = reconstruct_cmd(argc, argv); // Use helper
    if (jobs_table[slot].cmd == NULL) {
         // Use command name if reconstruction fails
         jobs_table[slot].cmd = (argc > 0 && argv[0]) ? strdup(argv[0]) : strdup("(unknown)");
         if (!jobs_table[slot].cmd) { // If even that fails
             perror("shell: strdup failed in add_job");
             jobs_table[slot].pid = 0; // Mark slot free again
             kill(pid, SIGTERM);      // Terminate untrackable job
             return 0;
         }
    }

    fprintf(stdout, "[%d] %d\n", jobs_table[slot].jid, pid);
    fflush(stdout); // Ensure output is seen immediately
    return jobs_table[slot].jid;
}

// Marks a job slot as free, freeing associated command string
void remove_job_by_pid(pid_t pid) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs_table[i].pid == pid) {
            if (jobs_table[i].cmd) {
                free(jobs_table[i].cmd);
                jobs_table[i].cmd = NULL;
            }
            jobs_table[i].pid = 0;
            jobs_table[i].jid = 0;
            // Don't reset state here, let update_job_statuses set DONE/TERMINATED
            // If called by fg, state doesn't matter much after removal.
            return; // Found and removed
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
     return -1; // Not found
}

// Checks status of background jobs, prints messages for completed ones
void update_job_statuses() {
    int status;
    pid_t child_pid;
    int job_updated = 0; // Flag to see if we need to flush stdout

    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs_table[i].pid != 0 && jobs_table[i].state == JOB_STATE_RUNNING) {
            // Check status without blocking, check for stopped state too
            child_pid = waitpid(jobs_table[i].pid, &status, WNOHANG | WUNTRACED | WCONTINUED);

            if (child_pid == jobs_table[i].pid) { // Status changed
                if (WIFEXITED(status)) {
                    fprintf(stdout, "[%d]+ Done\t\t%s\n", jobs_table[i].jid, jobs_table[i].cmd);
                    jobs_table[i].state = JOB_STATE_DONE;
                    job_updated = 1;
                    // Let 'jobs' or 'fg' handle actual removal from table
                } else if (WIFSIGNALED(status)) {
                     fprintf(stdout, "[%d]+ Terminated\t%s (Signal %d)\n", jobs_table[i].jid, jobs_table[i].cmd, WTERMSIG(status));
                     jobs_table[i].state = JOB_STATE_TERMINATED;
                     job_updated = 1;
                } else if (WIFSTOPPED(status)) {
                     fprintf(stdout, "[%d]+ Stopped\t%s (Signal %d)\n", jobs_table[i].jid, jobs_table[i].cmd, WSTOPSIG(status));
                     // jobs_table[i].state = JOB_STATE_STOPPED; // Need to add this enum state
                     job_updated = 1;
                } else if (WIFCONTINUED(status)) {
                     fprintf(stdout, "[%d]+ Continued\t%s\n", jobs_table[i].jid, jobs_table[i].cmd);
                     jobs_table[i].state = JOB_STATE_RUNNING; // Ensure state is running
                     job_updated = 1;
                }
            } else if (child_pid == -1 && errno != ECHILD && errno != EINTR) {
                // Error other than "No child processes" or "Interrupted system call"
                perror("shell: waitpid error checking job status");
                jobs_table[i].state = JOB_STATE_UNKNOWN; // Mark as error
                 remove_job_by_pid(jobs_table[i].pid); // Remove problematic job
                 job_updated = 1; // Error message is output
            }
            // If child_pid == 0, job is still running normally
        }
    }
    if (job_updated) {
        fflush(stdout); // Flush if any status change was printed
    }
}
// --- End Job Helper Implementations ---


// --- Command Execution Logic ---

// Searches for executable in PATH
char *search_path(char *file)
{
    char *PATH = getenv("PATH");
    if (!PATH) return NULL; // Handle case where PATH is not set

    char *path_copy = strdup(PATH); // Need a copy for strtok
    if (!path_copy) return NULL;

    char *p = path_copy;
    char *token;
    char *result_path = NULL;

    token = strtok(p, ":");
    while(token != NULL)
    {
        int plen = strlen(token);
        int alen = strlen(file);
        // Allocate enough space: dir + '/' + filename + '\0'
        char *full_path = malloc(plen + 1 + alen + 1);
        if (!full_path) {
             result_path = NULL; // Malloc failure
             break;
        }

        strcpy(full_path, token);

        // Append '/' if necessary
        if(full_path[plen-1] != '/') {
            strcat(full_path, "/");
        }
        strcat(full_path, file);

        // Check if file exists and is executable
        if(access(full_path, X_OK) == 0) { // access checks real user permissions
             struct stat st;
             if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode)) { // Check it's a regular file
                  result_path = full_path; // Found it! Keep the malloc'd path.
                  break; // Stop searching
             }
             // If not a regular file, ignore this match
        }

        // File not found or not executable, free path and try next token
        free(full_path);
        token = strtok(NULL, ":");
    }

    free(path_copy); // Free the copy of PATH
    return result_path; // Return malloc'd path if found, NULL otherwise
}

// Executes external command (called by child process)
int do_exec_cmd(int argc, char **argv)
{
    (void)argc; // Usually only argv[0] and NULL termination matter for execv

    if (!argv || !argv[0]) {
        errno = EINVAL; // Invalid argument (no command)
        return 0; // Indicate failure
    }

    if(strchr(argv[0], '/')) { // Path specified
        execv(argv[0], argv);
    } else { // Search in PATH
        char *path = search_path(argv[0]);
        if(!path) {
            // errno should be set by search_path (usually ENOENT)
            return 0; // Command not found in PATH
        }
        execv(path, argv);
        free(path); // Free path only if execv fails
    }

    // If execv returns, an error occurred. errno is set by execv.
    return 0; // Indicate failure
}


// Executes a simple command node (handling builtins, externals, redirection, background)
// Returns the exit status of the command (0 for success, non-zero for failure).
int do_simple_command(struct node_s *node)
{
    if (!node || node->type != NODE_COMMAND) {
        return 1; // Or indicate error? Let's say non-zero is failure.
    }

    // --- Build argv ---
    struct node_s *child = node->first_child;
    int argc = 0;
    int targc = 0;
    char **argv = NULL;
    char *str;

    while(child) {
        if (child->type != NODE_VAR) { // Should only have VAR children for args
            fprintf(stderr, "shell: internal error: non-VAR child in COMMAND node\n");
            child = child->next_sibling; // Skip invalid node type?
            continue;
        }
        str = child->val.str;
        // Perform word expansion (essential!)
        struct word_s *w = word_expand(str);
        if (!w) { // Expansion returned nothing or failed
            child = child->next_sibling;
            continue;
        }
        // Add expanded words to argv
        struct word_s *w2 = w;
        while (w2) {
            if (check_buffer_bounds(&argc, &targc, &argv)) {
                // Use strdup for safety, as free_buffer will free these.
                // word_expand should return malloc'd strings, but strdup is safer.
                char *arg_copy = strdup(w2->data);
                if(arg_copy) {
                    argv[argc++] = arg_copy;
                } else {
                    perror("shell: strdup failed building argv");
                    // Partial cleanup might be needed here if error occurs mid-expansion
                }
            } else {
                 fprintf(stderr, "shell: argument buffer overflow\n");
                 // Stop adding args?
            }
            w2 = w2->next;
        }
        free_all_words(w); // Free the list returned by word_expand
        child = child->next_sibling;
    }
    // NULL-terminate argv
    if (!check_buffer_bounds(&argc, &targc, &argv)) {
        fprintf(stderr, "shell: failed final argv allocation\n");
        if (argv) free_buffer(argc, argv); // Use original argc before potential ++argc issue
        return 1; // Failure
    }
    argv[argc] = NULL;
    // --------------------


    // --- Handle Empty Command ---
    if (argc == 0) {
        // Only redirections? This might be valid (e.g., '> file').
        // If there are redirections, the fork/exec part handles them.
        // If no args AND no redirections, it was truly empty.
        if (node->infile == NULL && node->outfile == NULL) {
            if (argv) free(argv); // Free the NULL termination pointer holder
            return 0; // Empty command is success (does nothing)
        }
         // If only redirections, proceed to fork/exec block
    }
    // --------------------------


    // --- Check for Background '&' (Should be handled by parser ideally) ---
    // If parser doesn't handle it, add check here. Assuming parser handles it for now.
    int background = 0;
    // Example if parser didn't handle it:
    // if (argc > 0 && strcmp(argv[argc - 1], "&") == 0) {
    //     background = 1;
    //     free(argv[argc - 1]);
    //     argv[argc - 1] = NULL;
    //     argc--;
    //     if (argc == 0) { /* Handle error */ }
    // }
    // --------------------------------------------------------------------


    // --- Built-in Check ---
    if (argc > 0) { // Need at least one arg for built-in check
        for (int i = 0; i < builtins_count; i++) {
            if (strcmp(argv[0], builtins[i].name) == 0) {
                // Handle redirection for builtins if necessary (complex!)
                // Standard shells often handle this, but it requires saving/restoring std fds
                if (node->infile || node->outfile) {
                     fprintf(stderr, "shell: redirection not supported for built-in '%s' yet.\n", argv[0]);
                     // Decide: run builtin without redirection, or return error?
                     // return 1; // Return error for now
                }
                int result = builtins[i].func(argc, argv);
                free_buffer(argc, argv); // Free argv
                return result; // Return status from built-in (0=success, non-0=fail, except exit)
            }
        }
    }
    // --------------------


    // --- External Command Execution ---
    pid_t child_pid = 0;
    int status = 0;
    int exit_status = 1; // Default to failure

    child_pid = fork();

    if (child_pid < 0) {
        perror("shell: fork failed");
        exit_status = -1; // Indicate fork error
    } else if (child_pid == 0) {
        // --- Child Process ---

        // TODO: Handle signals (reset SIGINT, SIGQUIT to default, ignore SIGTSTP etc.)
        // TODO: Handle process groups (setpgid) for proper job control

        // 1. Input Redirection (<)
        if (node->infile) {
            int input_fd = open(node->infile, O_RDONLY);
            if (input_fd < 0) {
                fprintf(stderr, "shell: failed to open input file '%s': %s\n",
                        node->infile, strerror(errno));
                _exit(EXIT_FAILURE); // Use _exit in child!
            }
            if (dup2(input_fd, STDIN_FILENO) < 0) {
                perror("shell: dup2 stdin failed");
                close(input_fd);
                _exit(EXIT_FAILURE);
            }
            close(input_fd);
        }

        // 2. Output Redirection (>, >>)
        if (node->outfile) {
            int open_flags = O_WRONLY | O_CREAT;
            open_flags |= (node->append_mode) ? O_APPEND : O_TRUNC;
            mode_t file_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // 0644
            int output_fd = open(node->outfile, open_flags, file_mode);
            if (output_fd < 0) {
                 fprintf(stderr, "shell: failed to open output file '%s': %s\n",
                         node->outfile, strerror(errno));
                 _exit(EXIT_FAILURE);
            }
            if (dup2(output_fd, STDOUT_FILENO) < 0) {
                perror("shell: dup2 stdout failed");
                close(output_fd);
                _exit(EXIT_FAILURE);
            }
            close(output_fd);
        }
        // TODO: Add stderr redirection (2>, 2>>) logic here

        // Execute command
        if (argc > 0) { // Only execute if there's a command name
             do_exec_cmd(argc, argv);
             // If exec returns, it failed. Print error.
             fprintf(stderr, "shell: command not found or failed to execute: %s: %s\n", argv[0], strerror(errno));
              if(errno == ENOEXEC) _exit(126); // Exec format error
              else if(errno == EACCES) _exit(126); // Permission denied
              else if(errno == ENOENT) _exit(127); // Command not found
              else _exit(EXIT_FAILURE); // Other failure
        } else {
             // Only redirections were specified, successful execution is exit 0
             _exit(EXIT_SUCCESS);
        }

    } else {
        // --- Parent Process ---
        // TODO: Handle process groups (setpgid)

        if (background) {
            add_job(child_pid, argv);
            exit_status = 0; // Launching background job is success for the parent shell command
        } else {
            // Foreground process
            // TODO: Terminal control (tcsetpgrp)

            waitpid(child_pid, &status, WUNTRACED);

            // ---> Determine Exit Status <---
            if (WIFEXITED(status)) {
                exit_status = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                // Optionally print message here or let update_job_statuses handle it later?
                // fprintf(stderr, "\n[%d] Terminated by signal %d\n", child_pid, WTERMSIG(status));
                exit_status = 128 + WTERMSIG(status);
            } else if (WIFSTOPPED(status)) {
                // fprintf(stderr, "\n[%d] Stopped by signal %d\n", child_pid, WSTOPSIG(status));
                // TODO: Add job to table as stopped
                // jobs_table[slot].state = JOB_STATE_STOPPED;
                exit_status = 128 + WSTOPSIG(status); // Treat as failure for sequence/conditionals
            } else {
                fprintf(stderr, "shell: [%d] Unexpected status from waitpid: %d\n", child_pid, status);
                exit_status = 1; // Generic failure
            }
            // TODO: Restore terminal control (tcsetpgrp)
            // TODO: Update $? status variable
        }
    }

    // Cleanup argv buffer
    if (argv) free_buffer(argc, argv); // Use original argc before potential background modification decrement

    return exit_status;
} // --- End of do_simple_command ---


// Executes a pipe node (cmd1 | cmd2)
// Returns the exit status of the *last* command in the pipeline.
int do_pipe_command(struct node_s *node) {
    if (!node || node->type != NODE_PIPE || node->children != 2 || !node->first_child || !node->first_child->next_sibling) {
        fprintf(stderr, "shell: invalid pipe node structure for execution\n");
        return 1; // Failure
    }

    struct node_s *cmd1_node = node->first_child;
    struct node_s *cmd2_node = node->first_child->next_sibling;

    int pipefd[2];
    pid_t pid1, pid2;
    int status1 = 0, status2 = 0;

    if (pipe(pipefd) == -1) {
        perror("shell: pipe");
        return 1; // Failure
    }

    // Fork for cmd1 (left side)
    pid1 = fork();
    if (pid1 < 0) {
        perror("shell: fork (cmd1)");
        close(pipefd[0]); close(pipefd[1]);
        return 1;
    }

    if (pid1 == 0) { // Child 1 (cmd1 - writer)
        close(pipefd[0]); // Close unused read end
        // Redirect stdout to pipe write end
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror("shell: dup2 stdout");
            close(pipefd[1]); _exit(EXIT_FAILURE);
        }
        close(pipefd[1]); // Close original write end
        // Execute left command recursively
        int exec_status = execute_node(cmd1_node);
        _exit(exec_status); // Exit with status of left command if exec fails later
    }

    // Fork for cmd2 (right side)
    pid2 = fork();
    if (pid2 < 0) {
        perror("shell: fork (cmd2)");
        close(pipefd[0]); close(pipefd[1]);
        kill(pid1, SIGTERM); // Try to clean up first child
        waitpid(pid1, NULL, 0);
        return 1;
    }

    if (pid2 == 0) { // Child 2 (cmd2 - reader)
        close(pipefd[1]); // Close unused write end
        // Redirect stdin to pipe read end
        if (dup2(pipefd[0], STDIN_FILENO) == -1) {
            perror("shell: dup2 stdin");
            close(pipefd[0]); _exit(EXIT_FAILURE);
        }
        close(pipefd[0]); // Close original read end
        // Execute right command recursively
        int exec_status = execute_node(cmd2_node);
        _exit(exec_status); // Exit with status of right command
    }

    // Parent Process
    // Close BOTH pipe ends IMMEDIATELY in parent
    close(pipefd[0]);
    close(pipefd[1]);

    // Wait for both children
    waitpid(pid1, &status1, 0); // Wait for first child (order doesn't strictly matter for zombies)
    waitpid(pid2, &status2, 0); // Wait for second child (its status determines pipe status)

    // Determine overall pipeline exit status (status of last command)
    int pipe_exit_status = 1; // Default failure
    if (WIFEXITED(status2)) {
        pipe_exit_status = WEXITSTATUS(status2);
    } else if (WIFSIGNALED(status2)) {
        pipe_exit_status = 128 + WTERMSIG(status2);
    }
    // TODO: Update $? status variable

    return pipe_exit_status;
} // --- End of do_pipe_command ---


// Main Execution Dispatcher
// Takes a node from the parser and executes it recursively.
// Returns the exit status of the command/pipeline/list.
int execute_node(struct node_s *node) {
    if (!node) {
        return 0; // Treat no node as success (e.g., empty line parsed to NULL)
    }

    int exit_status = 0; // Default to success before execution

    switch (node->type) {
        case NODE_COMMAND:
            exit_status = do_simple_command(node);
            break;

        case NODE_PIPE:
            exit_status = do_pipe_command(node);
            break;

        case NODE_AND: // Handle cmd1 && cmd2
            if (node->children != 2 || !node->first_child || !node->first_child->next_sibling) {
                 fprintf(stderr, "shell: internal error: invalid AND node structure\n");
                 exit_status = 1; // Failure status
                 break;
            }
            // Execute left child
            exit_status = execute_node(node->first_child);
            // Execute right child ONLY if left child succeeded (status 0)
            if (exit_status == 0) {
                exit_status = execute_node(node->first_child->next_sibling);
            }
            break; // exit_status holds the final status

        case NODE_OR:  // Handle cmd1 || cmd2
             if (node->children != 2 || !node->first_child || !node->first_child->next_sibling) {
                 fprintf(stderr, "shell: internal error: invalid OR node structure\n");
                 exit_status = 1; // Failure status
                 break;
             }
            // Execute left child
            exit_status = execute_node(node->first_child);
            // Execute right child ONLY if left child FAILED (status != 0)
            if (exit_status != 0) {
                exit_status = execute_node(node->first_child->next_sibling);
            }
            break; // exit_status holds the final status

        // Add cases for other node types later (e.g., NODE_SEQUENCE for ';')
        // case NODE_SEQUENCE:
        //    execute_node(node->first_child); // Execute first part
        //    exit_status = execute_node(node->first_child->next_sibling); // Return status of second part
        //    break;

        default:
            fprintf(stderr, "shell: error: unknown node type encountered during execution: %d\n", node->type);
            exit_status = 1; // Generic failure status
            break;
    }

    // TODO: Update the shell's $? variable with exit_status here

    return exit_status;
} // --- End of execute_node ---