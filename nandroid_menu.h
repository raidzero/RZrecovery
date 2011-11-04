#define BOOT     0x01
#define CACHE	 0x02
#define DATA     0x04
#define MISC	 0x08
#define RECOVERY 0x10
#define SYSTEM   0x20
#define ASECURE  0x40

#define PROGRESS 0x80

#define DEFAULT     0x67 // the sum of BCDAS

void nandroid_backup (char *subname, char partitions);

void show_nandroid_menu ();
