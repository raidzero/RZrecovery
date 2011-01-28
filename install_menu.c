#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/limits.h>
#include <dirent.h>
#include <linux/input.h>

#include "common.h"
#include "recovery_lib.h"
#include "minui/minui.h"
#include "recovery_ui.h"
#include "roots.h"
#include "firmware.h"
#include "install.h"
#include <string.h>

#include "nandroid_menu.h"

int install_rom_from_tar(char* filename)
{
    if (ui_key_pressed(KEY_SPACE)) {
	ui_print("Backing up before installing...\n");

	nandroid_backup("preinstall",BSD|PROGRESS);
    }

    ui_print("Attempting to install ROM from ");
    ui_print(filename);
    ui_print("...\n");
  
    char* argv[] = { "/sbin/nandroid-mobile.sh",
		     "--install-rom",
		     filename,
		     "--progress",
		     NULL };

    char* envp[] = { NULL };
  
    int status = runve("/sbin/nandroid-mobile.sh",argv,envp,1);
    if(!WIFEXITED(status) || WEXITSTATUS(status)!=0) {
	ui_printf_int("ERROR: install exited with status %d\n",WEXITSTATUS(status));
	return WEXITSTATUS(status);
    }
    else {
	ui_print("(done)\n");
    }
    ui_reset_progress();
    return 0;
}



void show_choose_tar_menu()
{
    static char* headers[] = { "Choose a ROM or press POWER to return",
			       "",
			       NULL };
    
    char path[PATH_MAX] = "";
    DIR *dir;
    struct dirent *de;
    int total = 0;
    int i;
    char** files;
    char** list;

    if (ensure_root_path_mounted("SDCARD:") != 0) {
	LOGE ("Can't mount /sdcard\n");
	return;
    }

    dir = opendir("/sdcard");
    if (dir == NULL) {
	LOGE("Couldn't open /sdcard");
	return;
    }
    
    while ((de=readdir(dir)) != NULL) {
	if (de->d_name[0] != '.' && strlen(de->d_name) > 4 && (strcmp(de->d_name+strlen(de->d_name)-8,".rom.tar")==0 || strcmp(de->d_name+strlen(de->d_name)-11,".rom.tar.gz")==0) || strcmp(de->d_name+strlen(de->d_name)-8,".rom.tgz")==0) {
	    total++;
	}
    }

    if (total==0) {
	LOGE("No tar archives found\n");
	if(closedir(dir) < 0) {
	    LOGE("Failed to close directory /sdcard");
	    return;
	}
    }
    else {
	files = (char**) malloc((total+1)*sizeof(char*));
	files[total]=NULL;

	list = (char**) malloc((total+1)*sizeof(char*));
	list[total]=NULL;
	
	rewinddir(dir);

	i = 0;
	while ((de = readdir(dir)) != NULL) {
	    if (de->d_name[0] != '.' && strlen(de->d_name) > 4 && (strcmp(de->d_name+strlen(de->d_name)-8,".rom.tar")==0 || strcmp(de->d_name+strlen(de->d_name)-11,".rom.tar.gz")==0) || strcmp(de->d_name+strlen(de->d_name)-8,".rom.tgz")==0) {
		files[i] = (char*) malloc(strlen("/sdcard/")+strlen(de->d_name)+1);
		strcpy(files[i], "/sdcard/");
		strcat(files[i], de->d_name);
		
		list[i] = (char*) malloc(strlen(de->d_name)+1);
		strcpy(list[i], de->d_name);

		i++;
	    }
	}

	if (closedir(dir) <0) {
	    LOGE("Failure closing directory /sdcard\n");
	    return;
	}

	int chosen_item = -1;
	while (chosen_item < 0) {
	    chosen_item = get_menu_selection(headers, list, 1, chosen_item<0?0:chosen_item);
	    if (chosen_item >= 0 && chosen_item != ITEM_BACK) {
		install_rom_from_tar(files[chosen_item]);
	    }
	}
    }
}

void show_choose_zip_menu(char* sdpath)
{
    static char* headers[] = { "Choose an update file or press POWER to return",
			       "",
			       NULL };
    
    char path[PATH_MAX] = "";
    DIR *dir;
    struct dirent *de;
    int total = 0;
    int i;
    char** files;
    char** list;

    if (ensure_root_path_mounted("SDCARD:") != 0) {
	LOGE ("Can't mount path\n");
	return;
    }

    dir = opendir(sdpath);
    if (dir == NULL) {
	LOGE("Couldn't open directory!");
	return;
    }
    
    while ((de=readdir(dir)) != NULL) {
	if (de->d_name[0] != '.' && strlen(de->d_name) > 4 && (strcmp(de->d_name+strlen(de->d_name)-11,"-update.zip")==0 || strcmp(de->d_name+strlen(de->d_name)-4,".zip")==0)) {
	    total++;
	}
    }

    if (total==0) {
	LOGE("No zip archives found\n");
	if(closedir(dir) < 0) {
	    LOGE("Failed to close directory");
	    return;
	}
    }
    else {
	files = (char**) malloc((total+1)*sizeof(char*));
	files[total]=NULL;

	list = (char**) malloc((total+1)*sizeof(char*));
	list[total]=NULL;
	
	rewinddir(dir);

	i = 0;
	while ((de = readdir(dir)) != NULL) {
	    if (de->d_name[0] != '.' && strlen(de->d_name) > 4 && (strcmp(de->d_name+strlen(de->d_name)-11,"-update.zip")==0 || strcmp(de->d_name+strlen(de->d_name)-4,".zip")==0)) {
		files[i] = (char*) malloc(strlen(sdpath)+strlen(de->d_name)+1);
		strcpy(files[i], sdpath);
		strcat(files[i], de->d_name);
		
		list[i] = (char*) malloc(strlen(de->d_name)+1);
		strcpy(list[i], de->d_name);

		i++;
	    }
	}

	if (closedir(dir) <0) {
	    LOGE("Failure closing directory \n");
	    return;
	}

	int chosen_item = -1;
	while (chosen_item < 0) {
	    chosen_item = get_menu_selection(headers, list, 1, chosen_item<0?0:chosen_item);
	    if (chosen_item >= 0 && chosen_item != ITEM_BACK) {
		install_update_zip(files[chosen_item]);
	    }
	}
    }
}

void show_kernel_menu()
{
    static char* headers[] = { "Choose a kernel file or press POWER to return",
			       "",
			       NULL };
    
    char path[PATH_MAX] = "";
    DIR *dir;
    struct dirent *de;
    int total = 0;
    int i;
    char** files;
    char** list;

    if (ensure_root_path_mounted("SDCARD:") != 0) {
	LOGE ("Can't mount /sdcard\n");
	return;
    }

    dir = opendir("/sdcard/kernels");
    if (dir == NULL) {
	LOGE("Couldn't open /sdcard/kernels");
	return;
    }
    
    while ((de=readdir(dir)) != NULL) {
	if (de->d_name[0] != '.' && strlen(de->d_name) > 4 && (strcmp(de->d_name+strlen(de->d_name)-8,"boot.img")==0 || strcmp(de->d_name+strlen(de->d_name)-16,"-kernel-boot.img")==0) || strcmp(de->d_name+strlen(de->d_name)-10,"kernel.img")==0) {
	    total++;
	}
    }

    if (total==0) {
	LOGE("No kernel images found\n");
	if(closedir(dir) < 0) {
	    LOGE("Failed to close directory /sdcard/kernels");
	    return;
	}
    }
    else {
	files = (char**) malloc((total+1)*sizeof(char*));
	files[total]=NULL;

	list = (char**) malloc((total+1)*sizeof(char*));
	list[total]=NULL;
	
	rewinddir(dir);

	i = 0;
	while ((de = readdir(dir)) != NULL) {
	    if (de->d_name[0] != '.' && strlen(de->d_name) > 4 && (strcmp(de->d_name+strlen(de->d_name)-8,"boot.img")==0 || strcmp(de->d_name+strlen(de->d_name)-16,"-kernel-boot.img")==0) || strcmp(de->d_name+strlen(de->d_name)-10,"kernel.img")==0) {
		files[i] = (char*) malloc(strlen("/sdcard/kernels/")+strlen(de->d_name)+1);
		strcpy(files[i], "/sdcard/kernels/");
		strcat(files[i], de->d_name);
		
		list[i] = (char*) malloc(strlen(de->d_name)+1);
		strcpy(list[i], de->d_name);

		i++;
	    }
	}

	if (closedir(dir) <0) {
	    LOGE("Failure closing directory /sdcard/kernels\n");
	    return;
	}

	int chosen_item = -1;
	while (chosen_item < 0) {
	    chosen_item = get_menu_selection(headers, list, 1, chosen_item<0?0:chosen_item);
	    if (chosen_item >= 0 && chosen_item != ITEM_BACK) {
		install_kernel_img(files[chosen_item]);
	    }
	}
    }
}

void show_rec_menu()
{
    static char* headers[] = { "Choose a recovery file or press POWER to return",
			       "",
			       NULL };
    
    char path[PATH_MAX] = "";
    DIR *dir;
    struct dirent *de;
    int total = 0;
    int i;
    char** files;
    char** list;

    if (ensure_root_path_mounted("SDCARD:") != 0) {
	LOGE ("Can't mount /sdcard\n");
	return;
    }

    dir = opendir("/sdcard/recovery");
    if (dir == NULL) {
	LOGE("Couldn't open /sdcard/recovery");
	return;
    }
    
    while ((de=readdir(dir)) != NULL) {
	if (de->d_name[0] != '.' && strlen(de->d_name) > 4 && (strcmp(de->d_name+strlen(de->d_name)-8,"-rec.img")==0 || strcmp(de->d_name+strlen(de->d_name)-8,"_rec.img")==0) || strcmp(de->d_name+strlen(de->d_name)-8,".rec.img")==0) {
	    total++;
	}
    }

    if (total==0) {
	LOGE("No recovery images found\n");
	if(closedir(dir) < 0) {
	    LOGE("Failed to close directory /sdcard/recovery");
	    return;
	}
    }
    else {
	files = (char**) malloc((total+1)*sizeof(char*));
	files[total]=NULL;

	list = (char**) malloc((total+1)*sizeof(char*));
	list[total]=NULL;
	
	rewinddir(dir);

	i = 0;
	while ((de = readdir(dir)) != NULL) {
	    if (de->d_name[0] != '.' && strlen(de->d_name) > 4 && (strcmp(de->d_name+strlen(de->d_name)-8,"-rec.img")==0 || strcmp(de->d_name+strlen(de->d_name)-8,"_rec.img")==0) || strcmp(de->d_name+strlen(de->d_name)-8,".rec.img")==0) {
		files[i] = (char*) malloc(strlen("/sdcard/recovery/")+strlen(de->d_name)+1);
		strcpy(files[i], "/sdcard/recovery/");
		strcat(files[i], de->d_name);
		
		list[i] = (char*) malloc(strlen(de->d_name)+1);
		strcpy(list[i], de->d_name);

		i++;
	    }
	}

	if (closedir(dir) <0) {
	    LOGE("Failure closing directory /sdcard/recovery\n");
	    return;
	}

	int chosen_item = -1;
	while (chosen_item < 0) {
	    chosen_item = get_menu_selection(headers, list, 1, chosen_item<0?0:chosen_item);
	    if (chosen_item >= 0 && chosen_item != ITEM_BACK) {
		install_rec_img(files[chosen_item]);
	    }
	}
    }
}
void append(char* s, char c)
{
        int len = strlen(s);
        s[len] = c;
        s[len+1] = '\0';
}


void show_choose_folder_menu(char* sdpath)
{
    static char* headers[] = { "Choose update folder or press POWER to return",
			       "",
			       NULL };
    
    char path[PATH_MAX] = "";
    DIR *dir;
    struct dirent *de;
    int total = 0;
    int i;
    char** files;
    char** list;

    if (ensure_root_path_mounted("SDCARD:") != 0) {
	LOGE ("Can't mount path\n");
	return;
    }

    dir = opendir(sdpath);
    if (dir == NULL) {
	LOGE("\nCouldn't open directory!");
	LOGE("\nPlease make sure directory exists!\n");
	return;
    }
    
    while ((de=readdir(dir)) != NULL) {
	if (de->d_name[0] != '.' && strlen(de->d_name) > 4 &&(strcmp(de->d_name+strlen(de->d_name)-0,"")==0 )) {
	    total++;
	}
    }

    if (total==0) {
	LOGE("No directories found\n");
	if(closedir(dir) < 0) {
	    LOGE("Failed to close directory");
	    return;
	}
    }
    else {
	files = (char**) malloc((total+1)*sizeof(char*));
	files[total]=NULL;

	list = (char**) malloc((total+1)*sizeof(char*));
	list[total]=NULL;
	
	rewinddir(dir);

	i = 0;
	while ((de = readdir(dir)) != NULL) {
	    if (de->d_name[0] != '.' && strlen(de->d_name) > 4 && (strcmp(de->d_name+strlen(de->d_name)-0,"")==0 )) {
		files[i] = (char*) malloc(strlen(sdpath)+strlen(de->d_name)+1);
		strcpy(files[i], sdpath);
		strcat(files[i], de->d_name);
		
		list[i] = (char*) malloc(strlen(de->d_name)+1);
		strcpy(list[i], de->d_name);

		i++;
	    }
	}

	if (closedir(dir) <0) {
	    LOGE("Failure closing directory \n");
	    return;
	}

	int chosen_item = -1;
	while (chosen_item < 0) {
		char* folder;
	    chosen_item = get_menu_selection(headers, list, 1, chosen_item<0?0:chosen_item);
	    if (chosen_item >= 0 && chosen_item != ITEM_BACK) {
		folder = files[chosen_item];
        append(folder, '/'); // add forward slash to string	
		show_choose_zip_menu(folder);
	    }
	}
    }
}

char *replace(const char *s, const char *old, const char *new)
{
char *ret;
int i, count = 0;
size_t newlen = strlen(new);
size_t oldlen = strlen(old);

for (i = 0; s[i] != '\0'; i++) {
if (strstr(&s[i], old) == &s[i]) {
count++;
i += oldlen - 1;
}
}

ret = malloc(i + count * (newlen - oldlen));
if (ret == NULL)
exit(EXIT_FAILURE);

i = 0;
while (*s) {
if (strstr(s, old) == s) {
strcpy(&ret[i], new);
i += newlen;
s += oldlen;
} else
ret[i++] = *s++;
}
ret[i] = '\0';

return ret;
}

int install_kernel_img(char* filename) {

    ui_print("\n-- Install kernel img from...\n");
	ui_print(filename);
	ui_print("\n");
	ensure_root_path_mounted("SDCARD:");	
	
    char* argv[] = { "/sbin/flash_image",
		     "boot",
		     filename,
		     NULL };

    char* envp[] = { NULL };
  
    int status = runve("/sbin/flash_image",argv,envp,1);

  
	ui_print("\nKernel flash from sdcard complete.\n");
	ui_print("\nThanks for using RZrecovery.\n");
	return 0;
}

int install_rec_img(char* filename) {

    ui_print("\n-- Install recovery img from...\n");
	ui_print(filename);
	ui_print("\n");
	ensure_root_path_mounted("SDCARD:");	
	
    char* argv[] = { "/sbin/flash_image",
		     "recovery",
		     filename,
		     NULL };

    char* envp[] = { NULL };
  
    int status = runve("/sbin/flash_image",argv,envp,1);

  
	ui_print("\nRecovery flash from sdcard complete.\n");
	ui_print("\nThanks for using RZrecovery.\n");
	return 0;
}

int install_update_zip(char* filename) {

char *path = NULL;

puts(filename);
path = replace(filename, "/sdcard/", "SDCARD:");

    ui_print("\n-- Install update.zip from sdcard...\n");
	    set_sdcard_update_bootloader_message();
		ui_print("Attempting update from...\n");
		ui_print(filename);
		ui_print("\n");
	    int status = install_package(path);
	    if (status != INSTALL_SUCCESS) {
		ui_set_background(BACKGROUND_ICON_ERROR);
		ui_print("Installation aborted.\n");
	    } else if (!ui_text_visible()) {
		return 0;  // reboot if logs aren't visible
	    } else {
		if (firmware_update_pending()) {
		    ui_print("\nReboot via menu to complete\n"
			     "installation.\n");
		} else {
		    ui_print("\nInstall from sdcard complete.\n");
			ui_print("\nThanks for using RZrecovery.\n");
		}
	    }
	return 0;
}



void show_install_menu()
{
    char* headers[] = { "Choose an install option or press",
			"POWER to exit",
			"",
			NULL };
  
    char* items[] = { "Install ROM tar from SD card",
		      "Install update.zip from SD card",
			  "Install update.zip from updates folder",
			  "Install kernel img from /sdcard/kernels",
			  "Install recovery img from /sdcard/recovery",
		      NULL };
  
#define ITEM_TAR 	0
#define ITEM_ZIP 	1
#define ITEM_FOLDER 2
#define ITEM_KERNEL 3
#define ITEM_REC	4

	char* sdloc = "/sdcard/";
	char* updateloc = "/sdcard/updates/";
    int chosen_item = -1;
    while (chosen_item != ITEM_BACK) {
	chosen_item = get_menu_selection(headers,items,1,chosen_item<0?0:chosen_item);
	switch(chosen_item) {
		case ITEM_TAR:
	    show_choose_tar_menu();
	    break;
	case ITEM_ZIP:
		show_choose_zip_menu(sdloc);
	    break;
	case ITEM_FOLDER:
		show_choose_folder_menu(updateloc);
	    break;
	case ITEM_KERNEL:
		show_kernel_menu();
		break;
	case ITEM_REC:
		show_rec_menu();
		break;
	}
    }
}
