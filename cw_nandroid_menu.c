#include <stdio.h>
#include <linux/input.h>
#include <sys/wait.h>
#include <sys/limits.h>
#include <dirent.h>

#include "common.h"
#include "recovery_lib.h"
#include "recovery_ui.h"
#include "minui/minui.h"
#include "roots.h"
#include "nandroid_menu.h"


void restore_cw_nandroid(char* filename) 
{
	ui_print("Preparing to restore clockwork backup");
	ui_print("\n");
	ui_print(filename);
	ui_print("\n...\n");
	ensure_root_path_mounted("SDCARD:");
	ensure_root_path_mounted("SYSTEM:");
	ensure_root_path_mounted("DATA:");
	
    char* argv[] = { "/sbin/cw_restore.sh",
		     filename,
		     NULL };

    char* envp[] = { NULL };
  
    int status = runve("/sbin/cw_restore.sh",argv,envp,1);
	sync();
	return;
}


void show_choose_cwnand_menu()
{
    static char* headers[] = { "Choose a CW backup or press POWER to return",
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

    dir = opendir("/sdcard/clockworkmod/backup/");
    if (dir == NULL) {
	LOGE("Couldn't open /sdcard/clockworkmod/backup/");
	return;
    }
    
    while ((de=readdir(dir)) != NULL) {
	if (de->d_name[0] != '.' && strlen(de->d_name) > 4 && (strcmp(de->d_name+strlen(de->d_name)-0,"")==0 )) {
	    total++;
	}
    }

    if (total==0) {
	LOGE("No backups found\n");
	if(closedir(dir) < 0) {
	    LOGE("Failed to close directory /sdcard/clockworkmod/backup/");
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
		files[i] = (char*) malloc(strlen("/sdcard/clockworkmod/backup/")+strlen(de->d_name)+1);
		strcpy(files[i], "/sdcard/clockworkmod/backup/");
		strcat(files[i], de->d_name);
		
		list[i] = (char*) malloc(strlen(de->d_name)+1);
		strcpy(list[i], de->d_name);

		i++;
	    }
	}

	if (closedir(dir) <0) {
	    LOGE("Failure closing directory /sdcard/clockworkmod/backup/\n");
	    return;
	}

	int chosen_item = -1;
	while (chosen_item < 0) {
	    chosen_item = get_menu_selection(headers, list, 1, chosen_item<0?0:chosen_item);
	    if (chosen_item >= 0 && chosen_item != ITEM_BACK) {
		restore_cw_nandroid(files[chosen_item]);
	    }
	}
    }
}


