#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "history.h"

char *history_list[MAX_HISTORY_SIZE];
int history_count = 0;

void init_history() {
    // Simple version: just clear the array
    for (int i = 0; i < MAX_HISTORY_SIZE; i++) {
        history_list[i] = NULL;
    }
    history_count = 0;
    // TODO (Optional): Load history from ~/.your_shell_history
}

void add_to_history(const char *cmd) {
    if (!cmd || cmd[0] == '\0' || strcmp(cmd, "\n") == 0) {
        return; // Don't add empty commands
    }

    // Prevent adding consecutive duplicates (optional but nice)
    if (history_count > 0 && strcmp(history_list[history_count - 1], cmd) == 0) {
        return;
    }

    // If history is full, discard the oldest entry (simple array approach)
    if (history_count >= MAX_HISTORY_SIZE) {
        free(history_list[0]); // Free the oldest string
        // Shift all entries down
        for (int i = 0; i < MAX_HISTORY_SIZE - 1; i++) {
            history_list[i] = history_list[i + 1];
        }
        history_list[MAX_HISTORY_SIZE - 1] = NULL; // Clear the last slot
        history_count--; // Decrement count before adding new one
    }

    // Duplicate the command string and add it
    history_list[history_count] = strdup(cmd);
    if (history_list[history_count] == NULL) {
        perror("shell: strdup failed adding to history");
        return; // Failed to add
    }
    history_count++;
}

void clear_history() {
    for (int i = 0; i < history_count; i++) {
        if (history_list[i]) {
            free(history_list[i]);
            history_list[i] = NULL;
        }
    }
    history_count = 0;
    // TODO (Optional): Save history to ~/.your_shell_history
}

// Gets history entry by 1-based logical index
char *get_history_entry(int index) {
     if (index <= 0 || index > history_count) {
         return NULL;
     }
     // Indexing is 0-based internally, user provides 1-based
     return history_list[index - 1];
}

int get_history_count() {
    return history_count;
}