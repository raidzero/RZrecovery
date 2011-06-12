void show_install_menu();

void choose_file_menu(char* sdpath);
int install_rom_from_tar(const char* filename);
int install_file(const char* path);
int install_rom_from_zip(char* filename);