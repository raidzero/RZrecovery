#define BOOT     0x01
#define DATA     0x04
#define ASECURE  0x08
#define SYSTEM   0x20
#define PROGRESS 0x40
#define BDAS     0x25


void nandroid_backup (char *subname, char partitions);

void show_nandroid_menu ();
