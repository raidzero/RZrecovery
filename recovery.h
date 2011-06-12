FILE* fopen_path(const char *path, const char *mode);
void check_and_fclose(FILE *fp, const char *name);
void set_sdcard_update_bootloader_message();
void copy_log_file(const char* destination, int append);
void finish_recovery(const char *send_intent);

int erase_volume(const char *volume);
char* copy_sideloaded_package(const char* original_path);
char** prepend_title(const char** headers) ;

int get_menu_selection(char** headers, char** items, int menu_only, int initial_selection);
int compare_string(const void* a, const void* b);
int install_file(const char* path);

void wipe_data();
void prompt_and_wait();
void print_property(const char *key, const char *name, void *cookie);

void write_files();
void read_files();

void reboot_android();
void reboot_recovery();
void power_off();