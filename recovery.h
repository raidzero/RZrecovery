void set_sdcard_update_bootloader_message();
void copy_log_file(const char *destination, int append);
void finish_recovery(const char *send_intent);

int erase_volume(const char *volume);
char **prepend_title(const char **headers);

int get_menu_selection(char **headers, char **items, int menu_only,
		       int initial_selection);

int compare_string(const void *a, const void *b);
int install_file(const char *path);

void prompt_and_wait();
void print_property(const char *key, const char *name, void *cookie);

void create_fstab();
void process_volumes();
int volume_present(char *volume);

int get_load_silently();
int get_fail_silently();
