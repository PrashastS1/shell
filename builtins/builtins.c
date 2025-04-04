#include "../shell.h"

extern int shell_history(int argc, char **argv);

struct builtin_s builtins[] =
{   
    { "dump"    , dump       },
    { "jobs",    shell_jobs  },
    { "fg",      shell_fg    },
    { "history", shell_history },
};

int builtins_count = sizeof(builtins)/sizeof(struct builtin_s);

