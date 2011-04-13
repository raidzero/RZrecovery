void check_and_fclose(FILE *fp, const char *name);

FILE* fopen_root_path(const char *root_path, const char *mode);

void finish_recovery(const char *send_intent);

char** prepend_title(char** headers);

void set_sdcard_update_bootloader_message();

int erase_root(const char *root);

int get_menu_selection(char** headers, char** items, int menu_only, int selected);

void wipe_data(int confirm);

void ui_printf_int(const char* format, int arg);

int runve(char* filename, char** argv, char** envp, int secs);
