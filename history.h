#define MAX_HISTORY_SIZE 100 // Or make it dynamic

// Declare history storage (defined globally in main.c or history.c)
extern char *history_list[MAX_HISTORY_SIZE];
extern int history_count; // Number of items currently in history
extern int history_base_index; // Starting index if using a circular buffer (optional)

// Declare history functions (implemented in main.c or history.c)
void init_history(void);      // Initialize history (maybe load from file)
void add_to_history(const char *cmd); // Add a command
void clear_history(void);     // Free memory on exit (maybe save to file)
char *get_history_entry(int index); // Get entry by logical index (1-based)
int get_history_count(void); // Get current count

// Declare history built-in command (implemented in builtins/history.c)
int shell_history(int argc, char **argv);