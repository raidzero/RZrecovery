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
#include <sys/reboot.h>

#include "nandroid_menu.h"

char *replace_str(char *str, char *orig, char *rep) // Helper function - search and replace within in string
{
  static char buffer[4096];
  char *p;

  if(!(p = strstr(str, orig)))  // Is 'orig' even in 'str'?
    return str;

  strncpy(buffer, str, p-str); // Copy characters from 'str' start to 'orig' st$
  buffer[p-str] = '\0';

  sprintf(buffer+(p-str), "%s%s", rep, p+strlen(orig));

  return buffer;
}

void choose_file_menu(char* sdpath) {
    static char* headers[] = { "Choose item or press POWER to return",
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

    dir = opendir(sdpath);
    if (dir == NULL) {
		LOGE("Couldn't open directory");
		LOGE("Please make sure it exists!");
		return;
    }
    //count the nummber of valid files:
    while ((de=readdir(dir)) != NULL) {
		if (de->d_type == DT_DIR) {
				total++;
		} else if (de->d_type == DT_REG) {
			if (strcmp(de->d_name+strlen(de->d_name)-4,".zip")==0) {
				total++;
			}
			if (strcmp(de->d_name+strlen(de->d_name)-4,".tar")==0) {
				total++;
			}
			if (strcmp(de->d_name+strlen(de->d_name)-4,".tgz")==0) {
				total++;
			}
			if (strcmp(de->d_name+strlen(de->d_name)-7,"rec.img")==0) {
				total++;
			}
			if (strcmp(de->d_name+strlen(de->d_name)-8,"boot.img")==0) {
				total++;
			}
		}
	}

    if (total==0) {
		LOGE("No valid files found!\n");
		if(closedir(dir) < 0) {
			LOGE("Failed to close directory\n");
	    return;
		}
    } else {
		files = (char**) malloc((total+1)*sizeof(char*));
		files[total]=NULL;

		list = (char**) malloc((total+1)*sizeof(char*));
		list[total]=NULL;
	
		rewinddir(dir);

		i = 0;
		while ((de = readdir(dir)) != NULL) {
			//display valid files
			if (((de->d_name[0] != '.') != 0 || de->d_name[1] != '.') && de->d_type == DT_DIR || (de->d_type == DT_REG && (strcmp(de->d_name+strlen(de->d_name)-4,".zip")==0 || strcmp(de->d_name+strlen(de->d_name)-4,".tar")==0 || strcmp(de->d_name+strlen(de->d_name)-4,".tgz")==0 || strcmp(de->d_name+strlen(de->d_name)-7,"rec.img")==0 || strcmp(de->d_name+strlen(de->d_name)-8,"boot.img")==0) || strcmp(de->d_name,"..") == 0 )) {
				
				files[i] = (char*) malloc(strlen(sdpath)+strlen(de->d_name)+1);
				strcpy(files[i], sdpath);
				strcat(files[i], de->d_name);
				list[i] = (char*) malloc(strlen(de->d_name)+1);
				
				if (de->d_type == DT_DIR) { //if is a dir, add / to it
					strcat(de->d_name, "/");
				}
				strcpy(list[i], de->d_name);				
					i++;				
			}
		}

		if (closedir(dir) <0) {
			LOGE("Failure closing directory\n");
			return;
		}

		int chosen_item = -1;
		while (chosen_item < 0) {
			chosen_item = get_menu_selection(headers, list, 1, chosen_item<0?0:chosen_item);
			if (chosen_item >= 0 && chosen_item != ITEM_BACK ) {
				if (opendir(files[chosen_item]) == NULL) {
					preinstall_menu(files[chosen_item]);
				}
				if (opendir(files[chosen_item]) != NULL) { 					
					char actualpath[PATH_MAX];
					char* folder = realpath(files[chosen_item], actualpath);
					strcat(folder, "/"); //append "/" to end 
					choose_file_menu(folder);			 
				} 
			} 
		} 
	}
}


int install_img(char* filename, char* partition) {

    ui_print("\n-- Flash image from...\n");
	ui_print(filename);
	ui_print("\n");
	ensure_root_path_mounted("SDCARD:");	
	
    char* argv[] = { "/sbin/flash_image",
		     partition,
		     filename,
		     NULL };

    char* envp[] = { NULL };

    int status = runve("/sbin/flash_image",argv,envp,1);
  
	ui_print("\nFlash from sdcard complete.");
	ui_print("\nThanks for using RZrecovery.\n");
	return 0;
}

void install_update_package(char* filename) {
	char* extension = (filename+strlen(filename)-3);
	char* boot_extension = (filename+strlen(filename)-8);
	char* rec_extension = (filename+strlen(filename)-7);
	if (strcmp(extension, "zip") == 0 ) {
		ui_print("\nZIP detected.\n");
		remove("/block_update");
		install_update_zip(filename);
		return;
	} 
	if (strcmp(extension, "tar") == 0 || strcmp(extension, "tgz") == 0 ) { 
		ui_print("\nTAR detected.\n");
		install_rom_from_tar(filename);
		return;
	}
	if (strcmp(rec_extension, "rec.img") == 0 ) {
		ui_print("\nRECOVERY IMAGE detected.\n");
		install_img(filename, "recovery");
		return;
	}
	if (strcmp(boot_extension, "boot.img") == 0 ) {
		ui_print("\nBOOT IMAGE detected.\n");
		install_img(filename, "boot");
		return;
	}
}
	
int install_update_zip(char* filename) {

	char *path = NULL;
	
	puts(filename);
	path = replace_str(filename, "/sdcard/", "SDCARD:");
	ui_print("\n-- Install update.zip from sdcard...\n");
	set_sdcard_update_bootloader_message();
	ui_print("Attempting update from...\n");
	ui_print(filename);
	ui_print("\n");
	int status = install_package(path);
	if (status != INSTALL_SUCCESS) {
		ui_set_background(BACKGROUND_ICON_ERROR);
		ui_print("Installation aborted.\n");
		return 0;
	} else if (!ui_text_visible()) {
		return 0;  // reboot if logs aren't visible
	} else {
	if (firmware_update_pending()) {
	    ui_print("\nReboot via menu to complete\ninstallation.\n");
		return 0;
	} else {
	    ui_print("\nInstall from sdcard complete.\n");
		ui_print("\nThanks for using RZrecovery.\n");
		}
	}   
	return 0;
}

int install_rom_from_tar(char* filename)
{
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
    } else {
		ui_print("(done)\n");
    }
		ui_reset_progress();
		return 0;
}

void preinstall_menu(char* filename) {

	char* basename = strrchr(filename, '/') +1;
	char install_string[PATH_MAX];
	strcpy(install_string, "Install ");
	strcat(install_string, basename);	
	
    char* headers[] = { "Preinstall Menu",
			"Please make your selections.",
			" ",
			NULL };
  
    char* items[] = { "Abort Install",
			  "Backup Before Install",
			  "Wipe /data",
			  install_string,
		      NULL };
#define ITEM_NO 		0
#define ITEM_BACKUP 	1
#define ITEM_WIPE 		2
#define ITEM_INSTALL 	3

		int chosen_item = -1;
		while (chosen_item != ITEM_BACK) {
		chosen_item = get_menu_selection(headers,items,1,chosen_item<0?0:chosen_item);
		switch(chosen_item) {
			case ITEM_NO:
			chosen_item = ITEM_BACK;
			return;
		case ITEM_BACKUP:
			ui_print("Backing up before installing...\n");
			nandroid_backup("preinstall",BSDA|PROGRESS);
			break;
		case ITEM_WIPE:
			wipe_partition(ui_text_visible(), "Are you sure?", "Yes - wipe DATA", "data");
			break;
		case ITEM_INSTALL:
			install_update_package(filename);		
			return;
		}
	}
}
