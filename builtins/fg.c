#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include "shell.h" // Use quotes for local include

// Match the function signature in struct builtin_s
int shell_fg(int argc, char **argv) {
    if (argc < 2) { // Need at least "fg" and "%N"
        fprintf(stderr, "shell: fg: usage: fg %%job_id\n");
        return 1;
    }

    char *arg_str = argv[1]; // Job ID is argv[1]
    if (arg_str[0] == '%') { // Skip optional '%'
        arg_str++;
    }

    // Parse JID
    char *endptr;
    long val = strtol(arg_str, &endptr, 10);
    if (errno == ERANGE || endptr == arg_str || *endptr != '\0' || val <= 0) {
         fprintf(stderr, "shell: fg: invalid job ID: %s\n", argv[1]);
         return 1;
    }
    int jid = (int)val;

    // Find PID for the JID
    update_job_statuses(); // Update status first
    pid_t pid = find_pid_by_jid(jid);
    if (pid == 0) {
        fprintf(stderr, "shell: fg: job not found: %%%d\n", jid);
        return 1;
    }

    // Find the command string for printing
    int slot = find_job_slot_by_pid(pid);
    char *cmd_str = (slot != -1 && jobs_table[slot].cmd) ? jobs_table[slot].cmd : "";
    fprintf(stdout, "%s\n", cmd_str); // Print command being foregrounded
    fflush(stdout);

    // Remove from background job table *before* waiting
    job_state current_state = (slot != -1) ? jobs_table[slot].state : JOB_STATE_UNKNOWN;
    remove_job_by_pid(pid);

    // --- Add terminal control and SIGCONT here for full job control ---

    // Wait for the now-foreground process
    int status;
    do {
        waitpid(pid, &status, WUNTRACED); // Use WUNTRACED
         // Add WIFSTOPPED handling here
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));

    // --- Add restoring terminal control here for full job control ---

    update_job_statuses(); // Check other jobs

    return 1; // Indicate success (continue shell)
}