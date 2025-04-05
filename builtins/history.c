#include <stdio.h>
#include <readline/readline.h> 
#include <readline/history.h>
#include "shell.h"

int shell_history(int argc, char **argv) {
    (void)argc;
    (void)argv;

    HIST_ENTRY **the_list;
    int i;

    the_list = history_list();

    if (the_list) {
        for (i = 0; the_list[i]!=NULL; i++) {
            printf("  %d  %s\n", i + history_base, the_list[i]->line);
        }
    } else {
    }

    return 1;
}