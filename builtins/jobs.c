#include <stdio.h>
#include <string.h>
#include "shell.h" // Use quotes for local include

// Match the function signature in struct builtin_s
int shell_jobs(int argc, char **argv) {
    (void)argc; // Suppress unused warning
    (void)argv;

    // Update statuses before listing - important!
    update_job_statuses();

    int jobs_listed = 0;
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs_table[i].pid != 0) { // If the slot is in use
            jobs_listed = 1;
            const char *state_str = "Unknown";
            char current_marker = ' '; // For '+' or '-'

            // Determine state string based on the enum in shell.h
            switch(jobs_table[i].state) {
                case JOB_STATE_RUNNING:    state_str = "Running"; break;
                case JOB_STATE_DONE:       state_str = "Done"; break;
                case JOB_STATE_TERMINATED: state_str = "Terminated"; break;
                // Add JOB_STATE_STOPPED later
                default:                   state_str = "Unknown"; break;
            }

            fprintf(stdout, "[%d]%c %-12s %s\n",
                    jobs_table[i].jid,
                    current_marker,
                    state_str,
                    jobs_table[i].cmd ? jobs_table[i].cmd : "(command string missing)");

            // Simple cleanup: If job is done/terminated, remove it after listing once
             if (jobs_table[i].state == JOB_STATE_DONE || jobs_table[i].state == JOB_STATE_TERMINATED) {
                 remove_job_by_pid(jobs_table[i].pid);
             }
        }
    }
    fflush(stdout);
    return 1; // Indicate success (continue shell)
}