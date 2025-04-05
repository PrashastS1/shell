#include <stdio.h>
#include <stdlib.h>     
#include <string.h>     
#include <errno.h>      
#include <limits.h>     
#include "shell.h"

int shell_exit(int argc, char **argv){
    int exit_code = 0;

    if (argc == 1) {
        return 0; 
    }

    else if (argc == 2) {
        char *endptr;
        errno = 0; 
        long val = strtol(argv[1], &endptr, 10); 

       
        if (errno == ERANGE || (errno != 0 && val == 0)) {
            fprintf(stderr, "shell: exit: %s: numeric argument required\n", argv[1]);
            return 1; 
        }
        if (endptr == argv[1]) {
            
            fprintf(stderr, "shell: exit: %s: numeric argument required\n", argv[1]);
            return 1; 
        }
        if (*endptr != '\0') {
            
            fprintf(stderr, "shell: exit: %s: numeric argument required\n", argv[1]);
            return 1; 
        }
        
        
        
        exit_code = (int)val; 
        cleanup_job_table(); 
        
        exit(exit_code); 
        
        perror("shell: exit call failed"); 
        return 1;

    }

    else {
        fprintf(stderr, "shell: exit: too many arguments\n");
        return 1; 
    }

    return 0;


}