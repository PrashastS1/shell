// #ifndef SHELL_H
// #define SHELL_H
// #define MAX_JOBS 16

// #include <stddef.h>     /* size_t */
// #include <glob.h>
// #include "source.h"
// #include <sys/types.h>

// void print_prompt1(void);
// void print_prompt2(void);
// char *read_cmd(void);
// int  parse_and_execute(struct source_s *src);

// void initsh(void);

// /* shell builtin utilities */
// int dump(int argc, char **argv);

// struct node_s;

// /* struct for builtin utilities */
// struct builtin_s
// {
//     char *name;    /* utility name */
//     int (*func)(int argc, char **argv); /* function to call to execute the utility */
// };

// typedef enum {
//     JOB_STATE_UNKNOWN,
//     JOB_STATE_RUNNING,
//     JOB_STATE_DONE,
//     JOB_STATE_TERMINATED
//     // Add JOB_STATE_STOPPED later if implementing full Ctrl+Z
// } job_state;

// typedef struct {
//     pid_t pid;
//     int jid;          // Job ID
//     job_state state;
//     char *cmd;        // Command line string (malloc'd)
// } job_entry;

// extern job_entry jobs_table[MAX_JOBS];

// // Declare job helper functions (defined in executor.c)
// void init_job_table(void);
// void cleanup_job_table(void);
// int add_job(pid_t pid, char **argv); // Takes argv to reconstruct cmd
// void remove_job_by_pid(pid_t pid);
// pid_t find_pid_by_jid(int jid);
// int find_job_slot_by_pid(pid_t pid); // Helper for fg/jobs message
// void update_job_statuses(void);

// // Declare NEW built-in functions (defined in builtins/jobs.c, builtins/fg.c)
// int shell_jobs(int argc, char **argv);
// int shell_fg(int argc, char **argv);
// // ---> End Job Control Declarations <---



// /* the list of builtin utilities */
// extern struct builtin_s builtins[];

// /* and their count */
// extern int builtins_count;

// /* struct to represent the words resulting from word expansion */
// struct word_s
// {
//     char  *data;
//     int    len;
//     struct word_s *next;
// };

// /* word expansion functions */
// struct  word_s *make_word(char *word);
// void    free_all_words(struct word_s *first);

// size_t  find_closing_quote(char *data);
// size_t  find_closing_brace(char *data);
// void    delete_char_at(char *str, size_t index);
// char   *substitute_str(char *s1, char *s2, size_t start, size_t end);
// char   *wordlist_to_str(struct word_s *word);

// struct  word_s *word_expand(char *orig_word);
// char   *word_expand_to_str(char *word);
// char   *tilde_expand(char *s);
// char   *command_substitute(char *__cmd);
// char   *var_expand(char *__var_name);
// char   *pos_params_expand(char *tmp, int in_double_quotes);
// struct  word_s *pathnames_expand(struct word_s *words);
// struct  word_s *field_split(char *str);
// void    remove_quotes(struct word_s *wordlist);

// char   *arithm_expand(char *__expr);

// /* some string manipulation functions */
// char   *strchr_any(char *string, char *chars);
// char   *quote_val(char *val, int add_quotes);
// int     check_buffer_bounds(int *count, int *len, char ***buf);
// void    free_buffer(int len, char **buf);

// /* pattern matching functions */
// int     has_glob_chars(char *p, size_t len);
// int     match_prefix(char *pattern, char *str, int longest);
// int     match_suffix(char *pattern, char *str, int longest);
// char  **get_filename_matches(char *pattern, glob_t *matches);

// #endif

#ifndef SHELL_H
#define SHELL_H

#define MAX_JOBS 16

#include <stddef.h>     /* size_t */
#include <glob.h>       /* For glob_t */
#include "source.h"     /* Assuming source_s is defined here */
#include <sys/types.h>  /* For pid_t */

// Forward declarations for structures potentially defined elsewhere
struct node_s;
struct token_s;

// --- Core Shell Functions ---
void print_prompt1(void);
void print_prompt2(void);
char *read_cmd(void);
int  parse_and_execute(struct source_s *src);
void initsh(void);

// --- Parser & Executor Entry Points ---
// (These might be better placed in dedicated parser.h/executor.h)
struct node_s *parse_pipeline(struct token_s **tok_p); // Parses pipelines
int execute_node(struct node_s *node);                // Executes any node type

// --- Builtin Utilities ---
/* struct for builtin utilities */
struct builtin_s
{
    char *name;    /* utility name */
    int (*func)(int argc, char **argv); /* function to call */
};

/* the list of builtin utilities */
extern struct builtin_s builtins[];
/* and their count */
extern int builtins_count;

/* Example declaration for a builtin */
int dump(int argc, char **argv);

// --- Job Control ---
typedef enum {
    JOB_STATE_UNKNOWN,
    JOB_STATE_RUNNING,
    JOB_STATE_DONE,
    JOB_STATE_TERMINATED
    // Add JOB_STATE_STOPPED later if implementing full Ctrl+Z
} job_state;

typedef struct {
    pid_t pid;
    int jid;          // Job ID
    job_state state;
    char *cmd;        // Command line string (malloc'd)
} job_entry;

extern job_entry jobs_table[MAX_JOBS];

// Job helper function declarations
void init_job_table(void);
void cleanup_job_table(void);
int add_job(pid_t pid, char **argv);
void remove_job_by_pid(pid_t pid);
pid_t find_pid_by_jid(int jid);
int find_job_slot_by_pid(pid_t pid);
void update_job_statuses(void);

// Job control built-in declarations
int shell_jobs(int argc, char **argv);
int shell_fg(int argc, char **argv);
// --- End Job Control ---


// --- Word Expansion ---
/* struct to represent the words resulting from word expansion */
struct word_s
{
    char  *data;
    int    len;
    struct word_s *next;
};

/* word expansion functions */
struct  word_s *make_word(char *word);
void    free_all_words(struct word_s *first);

size_t  find_closing_quote(char *data);
size_t  find_closing_brace(char *data);
void    delete_char_at(char *str, size_t index);
char   *substitute_str(char *s1, char *s2, size_t start, size_t end);
char   *wordlist_to_str(struct word_s *word);

struct  word_s *word_expand(char *orig_word);
char   *word_expand_to_str(char *word);
char   *tilde_expand(char *s);
char   *command_substitute(char *__cmd);
char   *var_expand(char *__var_name);
char   *pos_params_expand(char *tmp, int in_double_quotes);
struct  word_s *pathnames_expand(struct word_s *words);
struct  word_s *field_split(char *str);
void    remove_quotes(struct word_s *wordlist);

char   *arithm_expand(char *__expr);
// --- End Word Expansion ---


// --- String & Buffer Utils ---
char   *strchr_any(char *string, char *chars);
char   *quote_val(char *val, int add_quotes);
int     check_buffer_bounds(int *count, int *len, char ***buf);
void    free_buffer(int len, char **buf);
// --- End String & Buffer Utils ---


// --- Pattern Matching ---
int     has_glob_chars(char *p, size_t len);
int     match_prefix(char *pattern, char *str, int longest);
int     match_suffix(char *pattern, char *str, int longest);
char  **get_filename_matches(char *pattern, glob_t *matches);
// --- End Pattern Matching ---


#endif // SHELL_H