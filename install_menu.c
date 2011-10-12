#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/limits.h>
#include <dirent.h>
#include <linux/input.h>


#include "common.h"
#include "recovery.h"
#include "minui/minui.h"
#include "recovery_ui.h"
#include "roots.h"
#include "install.h"
#include <string.h>
#include <sys/reboot.h>

#include "nandroid_menu.h"


char** sortlist(char** list, int total) {
	int i = 0;
	if (list != NULL) {
		for (i = 0; i < total; i++) {
			int curMax = -1;
			int j;
			for (j = 0; j < total - i; j++) {
				if (curMax == -1 || strcmp(list[curMax], list[j]) < 0)
					curMax = j;
				}
				char* temp = list[curMax];
				list[curMax] = list[total - i - 1];
				list[total - i - 1] = temp;
		}	
	}
	return list;
}

void choose_file_menu(char* sdpath) {
    static char* headers[] = { "Choose item",
			       "",
			       NULL };
    
    char path[PATH_MAX] = "";
    DIR *dir;
    struct dirent *de;
    int total = 0;
	int ftotal = 0;
	int dtotal = 0;
	int i;
	int j;
    char** flist;
	char** dlist;
    char** list; 
    if (ensure_path_mounted("/sdcard") != 0) {
	LOGE ("Can't mount /sdcard\n");
	return;
    }

    dir = opendir(sdpath);
    if (dir == NULL) {
		LOGE("Couldn't open directory");
		LOGE("Please make sure it exists!");
		return;
    }
    //count the number of valid files:
    while ((de=readdir(dir)) != NULL) {
		if (de->d_type == DT_DIR && strcmp(de->d_name,".") != 0 && strcmp(de->d_name,"nandroid") != 0) {
				dtotal++;
		} else {
			if (strcmp(de->d_name+strlen(de->d_name)-4,".zip")==0) {
				ftotal++;
			}
			if (strcmp(de->d_name+strlen(de->d_name)-4,".apk")==0) {
				ftotal++;
			}
			if (strcmp(de->d_name+strlen(de->d_name)-4,".tar")==0) {
				ftotal++;
			}
			if (strcmp(de->d_name+strlen(de->d_name)-4,".tgz")==0) {
				ftotal++;
			}
			if (strcmp(de->d_name+strlen(de->d_name)-7,"rec.img")==0) {
				ftotal++;
			}
			if (strcmp(de->d_name+strlen(de->d_name)-8,"boot.img")==0) {
				ftotal++;
			}
		}
	}
	total = ftotal + dtotal;
    if (total==0) {
		ui_print("\nNo valid files found!\n");
    } else {
		flist = (char**) malloc((ftotal+1)*sizeof(char*));
		flist[ftotal]=NULL;

		dlist = (char**) malloc((dtotal+1)*sizeof(char*));
		dlist[dtotal]=NULL;

		list = (char**) malloc((total+1)*sizeof(char*));
		list[total]=NULL;
	
		rewinddir(dir);

		i = 0; //dir iterator
		j = 0; //file iterator
		while ((de = readdir(dir)) != NULL) {
			//create dirs list
			if (de->d_type == DT_DIR && strcmp(de->d_name,".") != 0 && strcmp(de->d_name,"nandroid") != 0) {
				dlist[i] = (char*) malloc(strlen(sdpath)+strlen(de->d_name)+1);
				strcpy(dlist[i], sdpath);
				strcat(dlist[i], de->d_name);
				dlist[i] = (char*) malloc(strlen(de->d_name)+2); //need one extra byte for the termnitaing char
				strcat(de->d_name, "/\0"); //add "/" followed by terminating character since these are dirs
				strcpy(dlist[i], de->d_name);				
					i++;				
			}
			//create files list
			if ((de->d_type == DT_REG && (strcmp(de->d_name+strlen(de->d_name)-4,".zip")==0 || strcmp(de->d_name+strlen(de->d_name)-4,".apk")==0 || strcmp(de->d_name+strlen(de->d_name)-4,".tar")==0 || strcmp(de->d_name+strlen(de->d_name)-4,".tgz")==0 || strcmp(de->d_name+strlen(de->d_name)-7,"rec.img")==0 || strcmp(de->d_name+strlen(de->d_name)-8,"boot.img")==0))) {			
				flist[j] = (char*) malloc(strlen(sdpath)+strlen(de->d_name)+1);
				strcpy(flist[j], sdpath);
				strcat(flist[j], de->d_name);
				flist[j] = (char*) malloc(strlen(de->d_name)+1);
				strcpy(flist[j], de->d_name);				
					j++;		
			}
		}
		
		

		if (closedir(dir) <0) {
			LOGE("Failure closing directory\n");
			return;
		}
		//sort lists
		sortlist(flist, ftotal);
		sortlist(dlist, dtotal);
		
		//join the file and dir list, with dirs on top - thanks cvpcs
		i = 0;
		j = 0;
		int k;
		for(k = 0; k < total; k++) {
			if(i < dtotal) {
				list[k] = strdup(dlist[i++]);
			} else {
				list[k] = strdup(flist[j++]);
			}
		}
		
		int chosen_item = -1;
		while (chosen_item < 0) {
			chosen_item = get_menu_selection(headers, list, 1, chosen_item<0?0:chosen_item);
			if (chosen_item >= 0 && chosen_item != ITEM_BACK ) {
			
				char install_string[PATH_MAX]; //create real path from selection
				strcpy(install_string, sdpath);
				strcat(install_string, list[chosen_item]);
				if (opendir(install_string) == NULL) { //handle selection
					preinstall_menu(install_string);
				} else {						
					choose_file_menu(install_string);					
				}
			}
		} 
	}
}


int install_img(char* filename, char* partition) {

    ui_print("\n-- Flash image from...\n");
	ui_print("%s",filename);
	ui_print("\n");
	ensure_path_mounted("/sdcard");	
	
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

int install_apk(char* filename, char* location) {
	
    ui_print("\n-- Installing APK from\n");
	ui_print("%s",filename);
	ui_print("\nTo\n");
	ensure_path_mounted("/sdcard");
	ensure_path_mounted(location);
	
	char locString[PATH_MAX];
	char* basename = strrchr(filename, '/') +1;

	strcpy(locString,location);
	strcat(locString,basename);
	
	ui_print("%s",locString);
	
	char* permString;
	
	if (strcmp(location,"/data/app/")==0) permString="1000:1000";
	if (strcmp(location,"/system/app/")==0) permString="0:0";
	
    char* argv[] = { "/sbin/busybox ",
		     "cp",
		     filename,
			 location,
		     NULL };

    char* envp[] = { NULL };
  
    runve("/sbin/busybox",argv,envp,1);
  
    char* argy[] = { "/sbin/busybox ",
		     "chown",
		     permString,
			 locString,
		     NULL };

    char* envy[] = { NULL };
  
    runve("/sbin/busybox",argy,envy,1);
	
	ui_print("\nAPK install from sdcard complete.");
	ui_print("\nThanks for using RZrecovery.\n");
	return 0;
}

void install_update_package(char* filename) {
	char* extension = (filename+strlen(filename)-3);
	char* boot_extension = (filename+strlen(filename)-8);
	char* rec_extension = (filename+strlen(filename)-7);
	char* apk_extension = (filename+strlen(filename)-3);
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
	if (strcmp(apk_extension, "apk") == 0 ) {
		ui_print("\nAPK detected.\n");
		preinstall_apk(filename, "recovery");
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
	ui_print("\n-- Install update.zip from sdcard...\n");
	set_sdcard_update_bootloader_message();
	ui_print("Attempting update from...\n");
	ui_print("%s",filename);
	ui_print("\n");
	int status = install_package(filename);
	if (status != INSTALL_SUCCESS) {
		ui_set_background(BACKGROUND_ICON_RZ);
		ui_print("Installation aborted.\n");
		return 0;
	} else {
	    ui_print("\nInstall from sdcard complete.\n");
		ui_print("\nThanks for using RZrecovery.\n");
	}   
	return 0;
}

int install_rom_from_tar(char* filename)
{
    ui_print("Attempting to install ROM from ");
    ui_print("%s",filename);
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

void preinstall_apk(char* filename) {
				
	char* basename = strrchr(filename, '/') +1;
	char install_string[PATH_MAX];
	char* location;
	strcpy(install_string, "Install ");
	strcat(install_string, basename);	
	
    char* headers[] = { "APK Install Menu",
			"Where will this APK go?",
			" ",
			NULL };
  
    char* items[] = { "Abort APK Install",
			  "/data/app",
			  "/system/app",
		      NULL };
#define ITEM_NO 		0
#define ITEM_DATA 		1
#define ITEM_SYSTEM 	2

		int chosen_item = -1;
		while (chosen_item != ITEM_BACK) {
		chosen_item = get_menu_selection(headers,items,1,chosen_item<0?0:chosen_item);
		switch(chosen_item) {
			case ITEM_NO:
			chosen_item = ITEM_BACK;
			return;
		case ITEM_DATA:
			ui_print("Installing to DATA...\n");
			location="/data/app/";
			install_apk(filename, location);	
			return;
		case ITEM_SYSTEM:
			ui_print("Installing to SYSTEM...\n");
			location="/system/app/";
			install_apk(filename, location);	
			return;
		}
	}
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
			nandroid_backup("preinstall",BDAS|PROGRESS);
			break;
		case ITEM_WIPE:
			wipe_partition("Are you sure?", "Yes - wipe DATA", "data");
			break;
		case ITEM_INSTALL:
			install_update_package(filename);		
			return;
		}
	}
}
