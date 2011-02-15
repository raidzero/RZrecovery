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

void nandroid_backup(char* subname, char partitions)
{
    ui_print("Attempting Nandroid backup.\n");
    
    int boot  = partitions&BOOT;
    int data  = partitions&DATA;
	int asecure = partitions&ASECURE;
    int system = partitions&SYSTEM;
    int progress = partitions&PROGRESS;

    int args = 4;
    if (!boot) args++;
	if (!asecure) args++;
    if (!system) args++;
    if (!data) args++;
    if (progress) args++;
    if (!strcmp(subname, "")) args+=2;  // if a subname is specified, we need 2 more arguments
  
    char** argv = malloc(args*sizeof(char*));
    argv[0]="/sbin/nandroid-mobile.sh";
    argv[1]="--backup";
    argv[2]="--defaultinput";
    argv[args]=NULL;
  
    int i = 3;
    if(!boot) {
	argv[i++]="--noboot";
    }
	if(!asecure) {
	argv[i++]="--nosecure";
	}
    if(!system) {
	argv[i++]="--nosystem";
    }
    if(!data) {
	argv[i++]="--nodata";
    }
    if(progress) {
	argv[i++]="--progress";
    }
    if(strcmp(subname,"")) {
	argv[i++]="--subname";
	argv[i++]=subname;
    }
    argv[i++]=NULL;

    char* envp[] = { NULL };

    int status = runve("/sbin/nandroid-mobile.sh",argv,envp,60);
    if (!WIFEXITED(status) || WEXITSTATUS(status)!=0) {
	ui_printf_int("ERROR: Nandroid exited with status %d\n",WEXITSTATUS(status));
    }
    else {
	ui_print("(done)\n");
    }
    ui_reset_progress();
}

void nandroid_restore(char* subname, char partitions)
{
    int boot  = partitions&BOOT;
    int data  = partitions&DATA;
	int asecure = partitions&ASECURE;
    int system = partitions&SYSTEM;
    int progress = partitions&PROGRESS;

    int args = 4;
    if (!boot) args++;
	if (!asecure) args++;
    if (!system) args++;
    if (!data) args++;
    if (progress) args++;
    if (!strcmp(subname, "")) args+=2;  // if a subname is specified, we need 2 more arguments
  
    char** argv = malloc(args*sizeof(char*));
    argv[0]="/sbin/nandroid-mobile.sh";
    argv[1]="--restore";
    argv[2]="--defaultinput";
    argv[args]=NULL;
  
    int i = 3;
    if(!boot) {
	argv[i++]="--noboot";
    }
	if(!asecure) {
	argv[i++]="--nosecure";
	}
    if(!system) {
	argv[i++]="--nosystem";
    }
    if(!data) {
	argv[i++]="--nodata";
    }
    if(progress) {
	argv[i++]="--progress";
    }
    if(strcmp(subname,"")) {
	argv[i++]="--subname";
	argv[i++]=subname;
    }
    argv[i++]=NULL;

    char* envp[] = { NULL };

    int status = runve("/sbin/nandroid-mobile.sh",argv,envp,80);
    if (!WIFEXITED(status) || WEXITSTATUS(status)!=0) {
	ui_printf_int("ERROR: Nandroid exited with status %d\n",WEXITSTATUS(status));
    }
    else {
	ui_print("(done)\n");
    }
    ui_reset_progress();
}

static void nandroid_simple_backup()
{
    ui_print("Attempting Nandroid default backup.\n");
    
    char* argv[] = { "/sbin/nandroid-mobile.sh",
		     "--backup",
		     "--defaultinput",
		     "--progress",
		     NULL };
    char* envp[] = { NULL };
    int status = runve("/sbin/nandroid-mobile.sh",argv,envp,60);
    if (!WIFEXITED(status) || WEXITSTATUS(status)!=0) {
	ui_printf_int("ERROR: Nandroid exited with status %d\n",WEXITSTATUS(status));
    }
    else {
	ui_print("(done)\n");
    }
    ui_reset_progress();
}

static void nandroid_simple_restore()
{
    ui_print("This should do a default nandroid restore.\n");
    
    ui_print("Attempting Nandroid default restore.\n");
    
    char* argv[] = { "/sbin/nandroid-mobile.sh",
		     "--restore",
		     "--defaultinput",
		     "--progress",
		     NULL };
    char* envp[] = { NULL };
    int status = runve("/sbin/nandroid-mobile.sh",argv,envp,80);
    if (!WIFEXITED(status) || WEXITSTATUS(status)!=0) {
	ui_printf_int("ERROR: Nandroid exited with status %d\n",WEXITSTATUS(status));
    }
    else {
	ui_print("(done)\n");
    }
    ui_reset_progress();
}

void nandroid_adv_r_choose_file(char* filename, char* nandroid_folder)
{
    static char* headers[] = { "Choose a backup prefix or press",
			       "POWER or DEL to return",
			       "",
			       NULL };
    char path[PATH_MAX] = "";
    DIR *dir;
    struct dirent *de;
    int total = 0;
    int i;
    char** files;
    char** list;

    if (ensure_root_path_mounted(nandroid_folder) != 0) {
	LOGE("Can't mount %s\n", nandroid_folder);
	return;
    }

    if (translate_root_path(nandroid_folder,path,sizeof(path))==NULL) {
	LOGE("Bad path %s\n", path);
	return;
    }

    dir = opendir(path);
    if (dir == NULL) {
	LOGE("Couldn't open directory %s\n", path);
	return;
    }
  
    while ((de=readdir(dir)) != NULL) {
	if (de->d_name[0] != '.') {
	    total++;
	}
    }

    if (total==0) {
	LOGE("No nandroid backups found\n");
	if(closedir(dir) < 0) {
	    LOGE("Failed to close directory %s\n", path);
	    goto out; // eww, maybe this can be done better
	}
    }
    else {
	files = (char**) malloc((total+1)*sizeof(char*));
	files[total]= NULL;
    
	list = (char**) malloc((total+1)*sizeof(char*));
	list[total] = NULL;
    
	rewinddir(dir);
    
	i = 0;
	while ((de = readdir(dir)) != NULL) {
	    if (de->d_name[0] != '.') {
		files[i] = (char*) malloc(strlen(nandroid_folder)+strlen(de->d_name)+1);
		strcpy(files[i], nandroid_folder);
		strcat(files[i], de->d_name);
	
		list[i] = (char *) malloc(strlen(de->d_name)+1);
		strcpy(list[i], de->d_name);
	
		i++;
	    }
	}
    
	if (closedir(dir) <0) {
	    LOGE("Failure closing directory %s\n", path);
	    goto out; // again, eww
	}
    
	int chosen_item = -1;
	while (chosen_item < 0) {
	    chosen_item = get_menu_selection(headers, list, 1, chosen_item<0?0:chosen_item);
      
	    if(chosen_item >= 0 && chosen_item != ITEM_BACK) {
		strcpy(filename,list[chosen_item]);
	    }
	}
    }
out:
    for (i = 0; i < total; i++) {
	free(files[i]);
	free(list[i]);
    }
    free(files);
    free(list);
}


void get_nandroid_adv_menu_opts(char** list, char p, char* br)
{

    char** tmp = malloc(8*sizeof(char*));
    int i;
    for (i=0; i<5; i++) {
	tmp[i]=malloc((strlen("(*)  ANDROID-SECURE")+strlen(br)+1)*sizeof(char));
    }
	
    sprintf(tmp[0],"(%c) %s BOOT",     p&BOOT?    '*':' ', br);
    sprintf(tmp[1],"(%c) %s DATA",     p&DATA?    '*':' ', br);
	sprintf(tmp[2],"(%c) %s ANDROID-SECURE",   p&ASECURE?  '*':' ', br);
    sprintf(tmp[3],"(%c) %s SYSTEM",   p&SYSTEM?  '*':' ', br);
    tmp[4]=NULL;

    char** h = list;
    char** j = tmp;

    for(;*j;j++,h++) *h=*j;
    
}

void show_nandroid_adv_r_menu()
{
    char* headers[] = { "Choose an option or press POWER to return",
			"prefix:",
			"",
			"",
			NULL };

    char* items[] = { "Choose backup",
		      "Perform restore",
		      NULL,
		      NULL,
		      NULL,
		      NULL,
			  NULL	};
  
#define ITEM_CHSE 0
#define ITEM_PERF 1
#define ITEM_B    2
#define ITEM_D    3
#define ITEM_A	  4
#define ITEM_S    5


    char filename[PATH_MAX];
    filename[0]=NULL;
    char partitions = (char) BSDA;
    int chosen_item = -1;

    while(chosen_item!=ITEM_BACK) {
	get_nandroid_adv_menu_opts(items+2,partitions,"restore"); // put the menu options in items[] starting at index 2
	chosen_item=get_menu_selection(headers,items,1,chosen_item<0?0:chosen_item);

	switch(chosen_item) {
	case ITEM_CHSE:
	    nandroid_adv_r_choose_file(filename,"SDCARD:/nandroid");
	    headers[2]=filename;
	    break;
	case ITEM_PERF:
	    ui_print("Restoring...\n");
	    nandroid_restore(filename,partitions|PROGRESS);
	    break;
	case ITEM_B:
	    partitions^=BOOT;
	    break;
	case ITEM_D:
	    partitions^=DATA;
	    break;
	case ITEM_A:
	    partitions^=ASECURE;
	    break;	
	case ITEM_S:
	    partitions^=SYSTEM;
	    break;	
	}
    }
}

void show_nandroid_adv_b_menu()
{
    char* headers[] = { "Choose an option or press POWER to return",
			"prefix:",
			"",
			"",
			NULL };
    char* items[] = { "Set backup name",
		      "Perform backup",
		      NULL,
		      NULL,
		      NULL,
		      NULL,
		      NULL };
  
#define ITEM_NAME 0
#define ITEM_PERF 1
#define ITEM_B    2
#define ITEM_D    3
#define ITEM_A	  4
#define ITEM_S    5

    char filename[PATH_MAX];
    filename[0]=NULL;
    int chosen_item = -1;

    char partitions = BSDA;

    while(chosen_item!=ITEM_BACK) {
	get_nandroid_adv_menu_opts(items+2,partitions,"backup"); // put the menu options in items[] starting at index 2
	chosen_item=get_menu_selection(headers,items,1,chosen_item<0?0:chosen_item);

	switch(chosen_item) {
	case ITEM_NAME:
		ui_print("\nType special characters by holding down\nthe Alt & Shift keys.\n");
	    ui_print("Type a prefix to name your backup:\n> ");
	    ui_read_line_n(filename,PATH_MAX);
	    headers[2]=filename;
	    break;
	case ITEM_PERF:
	    nandroid_backup(filename,partitions|PROGRESS);
	    break;
	case ITEM_B:
	    partitions^=BOOT;
	    break;
	case ITEM_D:
	    partitions^=DATA;
	    break;
	case ITEM_A:
	    partitions^=ASECURE;
	    break;
	case ITEM_S:
	    partitions^=SYSTEM;
	    break;
	}
    }
}

void show_nandroid_menu()
{
    static char* headers[] = { "Select an option or press POWER to return",
			       "",
			       NULL };
    static char* items[] = { /*"Simple Nandroid backup",
			     "Simple Nandroid restore (latest)",*/
			     "Nandroid Backup",
			     "Nandroid Restore",
				 "Clockwork Nandroid Restore",
			     NULL };

//#define ITEM_DEF_BACKUP  0
//#define ITEM_DEF_RESTORE 1
#define ITEM_ADV_BACKUP  0
#define ITEM_ADV_RESTORE 1
#define ITEM_CW_RESTORE	 2

  
    
    int chosen_item = -1;

    while(chosen_item!=ITEM_BACK) {
	chosen_item = get_menu_selection(headers,items,1,chosen_item<0?0:chosen_item);
      
	switch(chosen_item) {
//	case ITEM_DEF_BACKUP:
//	    nandroid_simple_backup();
//	    break;
//	case ITEM_DEF_RESTORE:
//	    nandroid_simple_restore();
//	    break;
	case ITEM_ADV_BACKUP:
	    show_nandroid_adv_b_menu();
	    break;
	case ITEM_ADV_RESTORE:
	    show_nandroid_adv_r_menu();
	    break;
	case ITEM_CW_RESTORE:
	    show_choose_cwnand_menu();
	    break;
	}
    }
}
