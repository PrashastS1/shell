// Create builtins/history.c
#include <stdio.h>
#include "../history.h" // Includes history function declarations

int shell_history(int argc, char **argv) {
    (void)argc; // Unused
    (void)argv;

    int count = get_history_count();
    for (int i = 0; i < count; i++) {
        // Print 1-based index and the command
        // Assuming commands are stored WITH newline
        printf("%5d  %s", i + 1, history_list[i]);
        // If stored WITHOUT newline, use: printf("%5d %s\n", i + 1, history_list[i]);
    }
    return 1; // Success, continue shell
}