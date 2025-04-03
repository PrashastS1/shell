#include "../shell.h"

struct builtin_s builtins[] =
{   
    { "dump"    , dump       },
    { "jobs",    shell_jobs  },
    { "fg",      shell_fg    },
};

int builtins_count = sizeof(builtins)/sizeof(struct builtin_s);

