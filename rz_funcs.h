int
__system(const char *command);

int
ask_question(char *question);

int
confirm_selection(char *question, char *operation, int autoaccept);

//original replace() is in busybox
char *
__replace(char *st, char *orig, char *repl);

int
runve(char *filename, char **argv, char **envp, int secs);

int
get_key();

void
ui_printf_int(const char *format, int arg);

void
get_check_menu_opts(char **items, char **chk_items, int *flags);

void
show_check_menu(char **headers, char **chk_items, int *flags);
