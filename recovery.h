FILE *fopen_path (const char *path, const char *mode);
void check_and_fclose (FILE * fp, const char *name);
void set_sdcard_update_bootloader_message ();
void copy_log_file (const char *destination, int append);
void finish_recovery (const char *send_intent);

int erase_volume (const char *volume);
char **prepend_title (const char **headers);

int get_menu_selection (char **headers, char **items, int menu_only,
			int initial_selection);
int compare_string (const void *a, const void *b);
int install_file (const char *path);

void wipe_data ();
void prompt_and_wait ();
void print_property (const char *key, const char *name, void *cookie);

void create_fstab ();
void process_volumes ();
int volume_present(char *volume);
void write_files ();
void read_files ();

void reboot_fn(char* action);
void reboot_android();

void ui_printf_int (const char *format, int arg);
void show_check_menu (char **headers, char **chk_items, int *flags);
void get_check_menu_opts (char **items, char **chk_items, int *flags);

char* replace(char *st, char *orig, char *repl);
int runve (char *filename, char **argv, char **envp, int secs);

int ask_question(char* question);
int confirm_selection(char* question, char* operation, int autoaccept);
