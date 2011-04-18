#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <linux/input.h>
#include <limits.h>
#include <sys/wait.h>
#include <limits.h>

#include "common.h"
#include "roots.h"
#include "bootloader.h"
#include "minui/minui.h"
#include "recovery_ui.h"
#include "minzip/DirUtil.h"

static const char *INTENT_FILE = "CACHE:recovery/intent";
static const char *SAVE_LOG_FILE = "SDCARD:recovery.log";
static const char *COMMAND_FILE = "CACHE:recovery/command";
static const char *LOG_FILE = "CACHE:recovery/log";
static const char *TEMPORARY_LOG_FILE = "/tmp/recovery.log";

// clear the recovery command and prepare to boot a (hopefully working) system,
// copy our log file to cache as well (for the system to read), and
// record any intent we were asked to communicate back to the system.
// this function is idempotent: call it as many times as you like.
// close a file, log an error if the error indicator is set
void check_and_fclose(FILE *fp, const char *name) {
    fflush(fp);
    if (ferror(fp)) LOGE("Error in %s\n(%s)\n", name, strerror(errno));
    fclose(fp);
}

// open a file given in root:path format, mounting partitions as necessary
FILE* fopen_root_path(const char *root_path, const char *mode) {
    if (ensure_root_path_mounted(root_path) != 0) {
        LOGE("Can't mount %s\n", root_path);
        return NULL;
    }

    char path[PATH_MAX] = "";
    if (translate_root_path(root_path, path, sizeof(path)) == NULL) {
        LOGE("Bad path %s\n", root_path);
        return NULL;
    }

    // When writing, try to create the containing directory, if necessary.
    // Use generous permissions, the system (init.rc) will reset them.
    if (strchr("wa", mode[0])) dirCreateHierarchy(path, 0777, NULL, 1);

    FILE *fp = fopen(path, mode);
    //    if (fp == NULL) LOGE("Can't open %s\n", path);
    return fp;
}

void remove_command() {
    if (ensure_root_path_mounted(COMMAND_FILE) != 0) {
	    remove("/cache/recovery/command");
    }
}

void finish_recovery(const char *send_intent) {
    // By this point, we're ready to return to the main system...
    if (send_intent != NULL) {
        FILE *fp = fopen_root_path(INTENT_FILE, "w");
        if (fp == NULL) {
            LOGE("Can't open %s\n", INTENT_FILE);
        } else {
            fputs(send_intent, fp);
            check_and_fclose(fp, INTENT_FILE);
        }
    }

    // Copy logs to cache so the system can find out what happened.
    FILE *log = fopen_root_path(LOG_FILE, "a");
    if (log != NULL) {
        FILE *tmplog = fopen(TEMPORARY_LOG_FILE, "r");
        if (tmplog == NULL) {
            LOGE("Can't open %s\n", TEMPORARY_LOG_FILE);
        } else {
            static long tmplog_offset = 0;
            fseek(tmplog, tmplog_offset, SEEK_SET);  // Since last write
            char buf[4096];
            while (fgets(buf, sizeof(buf), tmplog)) fputs(buf, log);
            tmplog_offset = ftell(tmplog);
            check_and_fclose(tmplog, TEMPORARY_LOG_FILE);
        }
        check_and_fclose(log, LOG_FILE);
    }

#ifndef BOARD_HAS_NO_MISC_PARTITION
        // Reset to mormal system boot so recovery won't cycle indefinitely.
        struct bootloader_message boot;
        memset(&boot, 0, sizeof(boot));
        set_bootloader_message(&boot);
#endif
    sync();  // For good measure.
}

char** prepend_title(char** headers) {

    FILE* f = fopen("/recovery.version","r");
    char* vers = calloc(20,sizeof(char));
    fgets(vers, 20, f);

    strtok(vers," ");  // truncate vers to before the first space

    char* patch_line_ptr = calloc((strlen(headers[0])+20),sizeof(char));
    char* patch_line = patch_line_ptr;
    strcpy(patch_line,headers[0]);
    strcat(patch_line," (");
    strcat(patch_line,vers);
    strcat(patch_line,")");

    int c = 0;
    char ** p1;
    for (p1 = headers; *p1; ++p1, ++c);
    
    char** ver_headers = calloc((c+1),sizeof(char*));
    ver_headers[0]=patch_line;
    ver_headers[c]=NULL;
    char** v = ver_headers+1;
    for (p1 = headers+1; *p1; ++p1, ++v) *v = *p1;

    char* title[] = { "Android system recovery <"
		      EXPAND(RECOVERY_API_VERSION) "e>",
                      "",
                      NULL };

    // count the number of lines in our title, plus the
    // caller-provided headers.
    int count = 0;
    char** p;
    for (p = title; *p; ++p, ++count);
    for (p = ver_headers; *p; ++p, ++count);

    char** new_headers = calloc((count+1),sizeof(char*));
    char** h = new_headers;
    for (p = title; *p; ++p, ++h) *h = *p;
    for (p = ver_headers; *p; ++p, ++h)	*h = *p;
    *h = NULL;

    return new_headers;
}
#ifndef BOARD_HAS_NO_MISC_PARTITION
void set_sdcard_update_bootloader_message()
{
    struct bootloader_message boot;
    memset(&boot, 0, sizeof(boot));
    strlcpy(boot.command, "boot-recovery", sizeof(boot.command));
    strlcpy(boot.recovery, "recovery\n", sizeof(boot.recovery));
    set_bootloader_message(&boot);
}
#endif

int erase_root(const char *root)
{
    ui_set_background(BACKGROUND_ICON_ERROR);
    ui_show_indeterminate_progress();
    ui_print("Formatting %s...\n", root);
    return format_root_device(root);
}

int get_menu_selection(char** headers, char** items, int menu_only, int selected) {
    // throw away keys pressed previously, so user doesn't
    // accidentally trigger menu items.
    ui_clear_key_queue();

    ui_start_menu(headers, items, selected);
	//int sel =0;
    int chosen_item = -1;

    while (chosen_item < 0) {
        int key = ui_wait_key();
	
	/*	char* key_str = calloc(18,sizeof(char));
		sprintf(key_str, "Key %d pressed.\n", key);
		ui_print(key_str);*/

	if (key == KEY_BACKSPACE || key == KEY_END || key == KEY_BACK || key == KEY_POWER) {
	    return(ITEM_BACK);
	}

        int visible = ui_text_visible();

        int action = device_handle_key(key, visible);

        if (action < 0) {
            switch (action) {
		    case HIGHLIGHT_UP:
			--selected;
			selected = ui_menu_select(selected);
			break;
		    case HIGHLIGHT_DOWN:
			++selected;
			selected = ui_menu_select(selected);
			break;
		    case SELECT_ITEM:
			chosen_item = selected;
			break;
		    case NO_ACTION:
			break;
		    case ITEM_BACK:
			chosen_item = ITEM_BACK;
			break;
		    }
        } else if (!menu_only) {
            chosen_item = action;
        }
    }

    ui_end_menu();
    return chosen_item;
}

void wipe_data(int confirm) {
    if (confirm) {
        static char** title_headers = NULL;

        if (title_headers == NULL) {
            char* headers[] = { "Confirm wipe of all user data?",
                                "  THIS CAN NOT BE UNDONE.",
                                "",
                                NULL };
            title_headers = prepend_title(headers);
        }

        char* items[] = { " No",
                          " No",
                          " No",
                          " No",
                          " No",
                          " No",
                          " No",
                          " Yes -- delete all user data",   // [7]
                          " No",
                          " No",
                          " No",
                          NULL };

        int chosen_item = get_menu_selection(title_headers, items, 1, 0);
        if (chosen_item != 7) {
            return;
        }
    }

    ui_print("\n-- Wiping data...\n");
    device_wipe_data();
    erase_root("DATA:");
    erase_root("CACHE:");
    ui_print("Data wipe complete.\n");
}

void ui_printf_int(const char* format, int arg)
{
    char* out = calloc(strlen(format)+12,sizeof(char));
    sprintf(out,format,arg);
    ui_print(out);
    free(out);
}

void get_check_menu_opts(char** items, char** chk_items, int* flags)
{
    int i;
    int count;
    for (count=0;chk_items[count];count++); // count it

    for (i=0; i<count; i++) {
	if (items[i]!=NULL) free(items[i]);
	items[i]=calloc(strlen(chk_items[i])+5,sizeof(char)); // 4 for "(*) " and 1 for the NULL-terminator
	sprintf(items[i],"(%s) %s", (*flags&(1<<i))?"*":" " , chk_items[i]);
    }
}

void show_check_menu(char** headers, char** chk_items, int* flags)
{
    int chosen_item = -1;

    int i;
    for (i=0;chk_items[i];i++); // count it
    i+=2; // i is the index of NULL in chk_items[], and we want an array 1 larger than chk_items, so add 1 for that and 1 for the NULL at the end => i+2

    char** items = calloc(i,sizeof(char*));
    items[0]="Finished";
    items[i]=NULL;

    while(chosen_item!=0) {
	get_check_menu_opts(items+1,chk_items,flags); // items+1 so we don't overwrite the Finished option

	chosen_item = get_menu_selection(headers,items,0,chosen_item==-1?0:chosen_item);
	if (chosen_item==0) {continue;}
	chosen_item-=1; // make it 0-indexed for bit-flipping the int
	*flags^=(1<<chosen_item); // flip the bit corresponding to the chosen item
	chosen_item+=1; // don't ruin the loop!
    }
}

int runve(char* filename, char** argv, char** envp, int secs)
{
    int opipe[2];
    int ipipe[2];
    pipe(opipe);
    pipe(ipipe);
    pid_t pid = fork();
    int status = 0;
    if (pid == 0) {
	dup2(opipe[1],1);
	dup2(ipipe[0],0);
	close(opipe[0]);
	close(ipipe[1]);
        execve(filename,argv,envp);
	char* error_msg = calloc(strlen(filename)+20,sizeof(char));
	sprintf(error_msg,"Could not execute %s\n",filename);
	ui_print(error_msg);
	free(error_msg);
	return(9999);
    }
    close(opipe[1]);
    close(ipipe[0]);
    FILE* from = fdopen(opipe[0],"r");
    FILE* to = fdopen(ipipe[1],"w");
    char* cur_line = calloc(100,sizeof(char));
    char* tok;
    int total_lines;
    int cur_lines;
    int num_items;
    int num_headers;
    int num_chks;
    float cur_progress;
    char** items = NULL;
    char** headers = NULL;
    char** chks = NULL;
    
    int i = 0; // iterator for menu items
    int j = 0; // iterator for menu headers
    int k = 0; // iterator for check menu items
    int l = 0;  // iterator for outputting flags from check menu
    int flags = INT_MAX;
    int choice;
    
    while (fgets(cur_line,100,from)!=NULL) {
	printf("%s",cur_line);
	tok=strtok(cur_line," \n");
	if(tok==NULL) {continue;}
	if(strcmp(tok,"*")==0) {
	    tok=strtok(NULL," \n");
	    if(tok==NULL) {continue;}
	    if(strcmp(tok,"ptotal")==0) {
		ui_set_progress(0.0);
		ui_show_progress(1.0,0);
		total_lines=atoi(strtok(NULL," "));
	    } else if (strcmp(tok,"print")==0) {
		ui_print(strtok(NULL,""));
	    } else if (strcmp(tok,"items")==0) {
		num_items=atoi(strtok(NULL," \n"));
		if(items!=NULL) free(items);
		items=calloc((num_items+1),sizeof(char*));
		items[num_items]=NULL;
		i=0;
	    } else if (strcmp(tok,"item")==0) {
		if (i < num_items) {
		    tok=strtok(NULL,"\n");
		    items[i]=calloc((strlen(tok)+1),sizeof(char));
		    strcpy(items[i],tok);
		    i++;
		}
	    } else if (strcmp(tok,"headers")==0) {
		num_headers=atoi(strtok(NULL," \n"));
		if(headers!=NULL) free(headers);
		headers=calloc((num_headers+1),sizeof(char*));
		headers[num_headers]=NULL;
		j=0;
	    } else if (strcmp(tok,"header")==0) {
		if (j < num_headers) {
		    tok=strtok(NULL,"\n");
		    if (tok) {
			headers[j]=calloc((strlen(tok)+1),sizeof(char));
			strcpy(headers[j],tok);
		    } else {
			headers[j]="";
		    }
		    j++;
		}
	    } else if (strcmp(tok,"show_menu")==0) {
		choice=get_menu_selection(headers,items,0,0);
		fprintf(to, "%d\n", choice);
		fflush(to);
	    } else if (strcmp(tok,"pcur")==0) {
		cur_lines=atoi(strtok(NULL,"\n"));
		if (cur_lines%10==0 || total_lines-cur_lines<10) {
		    cur_progress=(float)cur_lines/((float)total_lines);
		    ui_set_progress(cur_progress);
		}
		if (cur_lines==total_lines) ui_reset_progress();
	    } else if (strcmp(tok,"check_items")==0) {
		num_chks=atoi(strtok(NULL," \n"));
		if(chks!=NULL) free(chks);
		chks=calloc((num_chks+1),sizeof(char*));
		chks[num_chks]=NULL;
		k = 0;
	    } else if (strcmp(tok,"check_item")==0) {
		if (k < num_chks) {
		    tok=strtok(NULL,"\n");
		    chks[k]=calloc(strlen(tok)+1,sizeof(char));
		    strcpy(chks[k],tok);
		    k++;
		}
	    } else if (strcmp(tok,"show_check_menu")==0) {
		show_check_menu(headers,chks,&flags);
		for(l=0;l<num_chks;l++) {
		    fprintf(to, "%d\n", !!(flags&(1<<l)));
		}
		fflush(to);
	    } else if (strcmp(tok,"show_indeterminate_progress")==0) {
		ui_show_indeterminate_progress();
	    } else {ui_print("unrecognized command "); ui_print(tok); ui_print("\n");}
	}
    }

    while (waitpid(pid, &status, WNOHANG) == 0) {
	sleep(1);
    }
    ui_print("\n");
    free(cur_line);
    return status;
}
