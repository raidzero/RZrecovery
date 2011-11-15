#include <stdio.h>
#include <linux/input.h>
#include <sys/wait.h>
#include <sys/limits.h>
#include <dirent.h>

#include "common.h"
#include "recovery.h"
#include "recovery_ui.h"
#include "minui/minui.h"
#include "roots.h"
#include "strings.h"
#include "nandroid_menu.h"
#include "install_menu.h"

void
nandroid (const char* operation, char *subname, char partitions, int reboot_after, int show_progress, int compress)
{
  ui_print ("Attempting Nandroid %s.\n", operation);

  int boot = partitions & BOOT;
  int data = partitions & DATA;
  int cache = partitions & CACHE;
  int system = partitions & SYSTEM;
  int asecure = partitions & ASECURE;
  int args;

  if (!strcmp(operation,"backup") == 0 ) args = 6;
  else args = 7;

  if (!boot) 
  {
    printf("\nBoot ignored.");
    args++;
  }
  if (!asecure)
  {
    printf("\nAndroid_secure ignored.");
    args++;
  }  
  if (!system)
  {
    printf("\nSystem ignored.");
    args++;
  }
  if (!data)
  {
    printf("\nData ignored.");
    args++;
  }  
  if (!cache)
  {
    printf("\nCache ignored.");
    args++;
  }  
  if (show_progress)
  {
    printf("\nProgress shown.");
    args++;
  }  
  if (compress)
  {
    printf("\nCompression activated.");
    args++;
  } 
  if (reboot_after)
  {
    printf("\nReboot triggered.");
  }
  if (!strcmp (subname, ""))
  {
    printf("\nSubname given: %s", subname);
    args += 2;			// if a subname is specified, we need 2 more arguments
  }  

  char **argv = malloc (args * sizeof (char *));

  argv[0] = "/sbin/nandroid-mobile.sh";
  
  if (strcmp(operation,"backup") == 0) argv[1] = "--backup";
  if (strcmp(operation,"restore") == 0) argv[1] = "--restore";
  argv[2] = "--defaultinput";
  argv[args] = NULL;

  int i = 3;

  if (!boot)
	  {
	    argv[i++] = "--noboot";
	  }
  if (!asecure)
	  {
	    argv[i++] = "--nosecure";
	  }
  if (!system)
	  {
	    argv[i++] = "--nosystem";
	  }
  if (!data)
	  {
	    argv[i++] = "--nodata";
	  }
  if (!cache) 
  	  {
	    argv[i++] = "--nocache";
	  }
  if (show_progress)
	  {
	    argv[i++] = "--progress";
	  }
  if (compress)
	  {
	    argv[i++] = "--compress";
	  }	  
  if (strcmp (subname, ""))
	  {
	    argv[i++] = "--subname";
	    argv[i++] = subname;
	  }
  argv[i++] = NULL;

  char *envp[] = { NULL };

  write_files();
  int status = runve ("/sbin/nandroid-mobile.sh", argv, envp, 80);
  read_files();

  if (!WIFEXITED (status) || WEXITSTATUS (status) != 0)
    {
      if (!WIFEXITED (status) || WEXITSTATUS (status) == 34) 
      {
        if (ask_question("Out of space. Delete a backup?"))
        {  
          ui_reset_progress();
	  show_delete_menu();
        } else {
          ui_printf_int ("ERROR: Nandroid exited with status %d\n",
			   WEXITSTATUS (status));
        }
      }
    }  
    else
    {
      ui_print ("(done)\n");
       if (reboot_after) {
	 reboot_android();
       }  
    }
  ui_reset_progress ();
}

void
nandroid_adv_r_choose_file (char *filename, char *nandroid_folder)
{
  static char *headers[] = { "Choose a backup",
    "",
    NULL
  };
  char path[PATH_MAX] = "";
  DIR *dir;
  struct dirent *de;
  int total = 0;
  int i;
  char **files;
  char **list;

  if (ensure_path_mounted (nandroid_folder) != 0)
	  {
	    LOGE ("Can't mount %s\n", nandroid_folder);
	    return;
	  }

  dir = opendir (nandroid_folder);
  if (dir == NULL)
	  {
	    LOGE ("Couldn't open directory %s\n", nandroid_folder);
	    return;
	  }

  while ((de = readdir (dir)) != NULL)
	  {
	    if (de->d_name[0] != '.')
		    {
		      total++;
		    }
	  }

  if (total == 0)
	  {
	    LOGE ("No nandroid backups found\n");
	    if (closedir (dir) < 0)
		    {
		      LOGE ("Failed to close directory %s\n", path);
		      goto out;	// eww, maybe this can be done better
		    }
	  }
  else
	  {
	    files = (char **) malloc ((total + 1) * sizeof (char *));
	    files[total] = NULL;

	    list = (char **) malloc ((total + 1) * sizeof (char *));
	    list[total] = NULL;

	    rewinddir (dir);

	    i = 0;
	    while ((de = readdir (dir)) != NULL)
		    {
		      if (de->d_name[0] != '.')
			      {
				files[i] =
				  (char *) malloc (strlen (nandroid_folder) +
						   strlen (de->d_name) + 1);
				strcpy (files[i], nandroid_folder);
				strcat (files[i], de->d_name);

				list[i] =
				  (char *) malloc (strlen (de->d_name) + 1);
				strcpy (list[i], de->d_name);

				i++;
			      }
		    }

	    if (closedir (dir) < 0)
		    {
		      LOGE ("Failure closing directory %s\n", path);
		      goto out;	// again, eww
		    }

	    sortlist(list, total); //sort it alphabetically
	    int chosen_item = -1;

	    while (chosen_item < 0)
		    {
		      chosen_item =
			get_menu_selection (headers, list, 0,
					    chosen_item <
					    0 ? 0 : chosen_item);
		      if (chosen_item >= 0 && chosen_item != ITEM_BACK)
			      {
				strcpy (filename, list[chosen_item]);
			      }
		    }
	  }
out:
  for (i = 0; i < total; i++)
	  {
	    free (files[i]);
	    free (list[i]);
	  }
  free (files);
  free (list);
}

void
reverse_array (char **inver_a)
{
  int j = sizeof (inver_a) / sizeof (char **);
  int i, temp;

  j--;
  for (i = 0; i < j; i++)
	  {
	    temp = inver_a[i];
	    inver_a[i] = inver_a[j];
	    inver_a[j] = temp;
	    j--;
	  }
  return inver_a;
}

void
get_nandroid_adv_r_menu_opts (char **list, char p, char *br, int reboot_after, int show_progress)
{

  char **tmp = malloc (8 * sizeof (char *));
  int i;

  for (i = 0; i < 7; i++)
	  {
	    tmp[i] =
	      malloc ((strlen ("(*)  Reboot afterwards") + strlen (br) +
		       1) * sizeof (char));
	  }

  sprintf (tmp[0], "(%c) %s BOOT", p & BOOT ? '*' : ' ', br);
  sprintf (tmp[1], "(%c) %s DATA", p & DATA ? '*' : ' ', br);
  sprintf (tmp[2], "(%c) %s ANDROID-SECURE", p & ASECURE ? '*' : ' ', br);
  sprintf (tmp[3], "(%c) %s SYSTEM", p & SYSTEM ? '*' : ' ', br);
  sprintf (tmp[4], "(%c) %s CACHE", p & CACHE ? '*' : ' ', br);
  sprintf (tmp[5], "(%c) show progress", show_progress ? '*':' ');
  sprintf (tmp[6], "(%c) reboot afterwards", reboot_after ? '*':' ');

  tmp[7] = NULL;

  char **h = list;
  char **j = tmp;

  for (; *j; j++, h++)
    *h = *j;

}

void
get_nandroid_adv_b_menu_opts (char **list, char p, char *br, int reboot_after, int show_progress, int compress)
{

  char **tmp = malloc (9 * sizeof (char *));
  int i;

  for (i = 0; i < 8; i++)
	  {
	    tmp[i] =
	      malloc ((strlen ("(*)  Reboot afterwards") + strlen (br) +
		       1) * sizeof (char));
	  }

  sprintf (tmp[0], "(%c) %s BOOT", p & BOOT ? '*' : ' ', br);
  sprintf (tmp[1], "(%c) %s DATA", p & DATA ? '*' : ' ', br);
  sprintf (tmp[2], "(%c) %s ANDROID-SECURE", p & ASECURE ? '*' : ' ', br);
  sprintf (tmp[3], "(%c) %s SYSTEM", p & SYSTEM ? '*' : ' ', br);
  sprintf (tmp[4], "(%c) %s CACHE", p & CACHE ? '*' : ' ', br);
  sprintf (tmp[5], "(%c) show progress", show_progress ? '*':' ');
  sprintf (tmp[6], "(%c) gzip compress", compress ? '*':' ');
  sprintf (tmp[7], "(%c) reboot afterwards", reboot_after ? '*':' ');

  tmp[8] = NULL;

  char **h = list;
  char **j = tmp;

  for (; *j; j++, h++)
    *h = *j;

}

void
show_nandroid_adv_r_menu ()
{
  char *headers[] = { "Choose partitions to restore",
    "prefix:",
    "",
    "",
    NULL
  };

  char *items[] = { "Choose backup",
    "Perform restore",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  };

#define ITEM_CHSE 0
#define R_ITEM_PERF 1
#define R_ITEM_B    2
#define R_ITEM_D    3
#define R_ITEM_A    4
#define R_ITEM_S    5
#define R_ITEM_C    6
#define R_ITEM_P    7
#define R_ITEM_R    8

  char filename[PATH_MAX];

  filename[0] = NULL;
  char partitions = (char) DEFAULT;
  int chosen_item = -1;
  int show_progress = 1;
  int reboot_after = 0;
  int compress = 0;

  while (chosen_item != ITEM_BACK)
	  {
	    get_nandroid_adv_r_menu_opts (items + 2, partitions, "restore", reboot_after, show_progress);	// put the menu options in items[] starting at index 2
	    chosen_item =
	      get_menu_selection (headers, items, 0,
				  chosen_item < 0 ? 0 : chosen_item);

	    switch (chosen_item)
		    {
		    case ITEM_CHSE:
		      nandroid_adv_r_choose_file (filename,
						  "/sdcard/nandroid");
		      headers[2] = filename;
		      break;
		    case R_ITEM_PERF:
		      ui_print ("Restoring...\n");
		      nandroid("restore", filename, partitions, reboot_after, show_progress, compress);
		      break;
		    case R_ITEM_B:
		      partitions ^= BOOT;
		      break;
		    case R_ITEM_D:
		      partitions ^= DATA;
		      break;
		    case R_ITEM_A:
		      partitions ^= ASECURE;
		      break;
		    case R_ITEM_S:
		      partitions ^= SYSTEM;
		      break;
		    case R_ITEM_C:
		      partitions ^= CACHE;
		      break;
		    case R_ITEM_P:
		      show_progress ^= 1;
		      break;
		    case R_ITEM_R:
		      reboot_after ^= 1;
		      break;
		    }
	  }
}

void
show_delete_menu() 
{
  char *headers[] = { "Choose a backup to delete",
    "Backup:",
    "",
    "",
    NULL
  };

  char *items[] = { "Choose backup",
    "Delete!",
    NULL
  };

#define D_ITEM_C  0
#define D_ITEM_D  1
  
  char filename[PATH_MAX];
  filename[0] = NULL;

  char operation[PATH_MAX];
  char del_cmd[PATH_MAX];
  int chosen_item = -1;

  while (chosen_item != ITEM_BACK) 
  {
    chosen_item = get_menu_selection(headers, items, 0, 0);
  
    switch (chosen_item) 
    {
      case D_ITEM_C:
        nandroid_adv_r_choose_file (filename, "/sdcard/nandroid");
        headers[2] = filename;
	sprintf(operation, "Delete %s", filename);
	sprintf(del_cmd,"rm -rf /sdcard/nandroid/%s", filename);
        break;
      case D_ITEM_D:  
        if (confirm_selection("Are you sure?", operation))
        { 
	  if (strcmp(filename,"") != 0) 
	  {  
            __system(del_cmd);
	    ui_print("Backup %s deleted!\n", filename);
	  } else {
	    ui_print("You must select a backup first!\n");
	  }  
        } else {
	  return;
	}
        return;
    }
  }  
}


void
show_compress_menu() 
{
  char *headers[] = { "Choose a backup to compress",
    "Backup:",
    "",
    "",
    NULL
  };

  char *items[] = { "Choose backup",
    "Compress!",
    NULL
  };

#define C_ITEM_C  0
#define C_ITEM_D  1
  
  char filename[PATH_MAX];
  filename[0] = NULL;

  char operation[PATH_MAX];
  char cmp_cmd[PATH_MAX];
  int chosen_item = -1;

  while (chosen_item != ITEM_BACK) 
  {
    chosen_item = get_menu_selection(headers, items, 0, 0);
  
    switch (chosen_item) 
    {
      case C_ITEM_C:
        nandroid_adv_r_choose_file (filename, "/sdcard/nandroid");
        headers[2] = filename;
	
	sprintf(operation, "Compress %s", filename);
	char pathname[256];
	sprintf(pathname, "/sdcard/nandroid/%s", filename);
	char **argv = malloc (2 * sizeof (char *));

	argv[0] = "/sbin/compress_nandroid.sh";
	argv[1] = pathname;
	argv[2] = NULL;
	
	char *envp[] = { NULL };
        break;
      case C_ITEM_D:  
	if (strcmp(filename,"") != 0) 
	{  
          runve("/sbin/compress_nandroid.sh", argv, envp, 200);
	  ui_reset_progress();
	} else {
	  ui_print("You must select a backup first!\n");
	}  
        return;
    }
  }  
}

void
show_nandroid_adv_b_menu ()
{
  char *headers[] = { "Choose partitions to backup",
    "",
    NULL
  };
  char *items[] = { "Perform backup",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  };

#define B_ITEM_PERF 0
#define B_ITEM_B    1
#define B_ITEM_D    2
#define B_ITEM_A    3
#define B_ITEM_S    4
#define B_ITEM_C    5
#define B_ITEM_P    6
#define B_ITEM_Z    7
#define B_ITEM_R    8

  char filename[PATH_MAX];

  filename[0] = NULL;
  int chosen_item = -1;
  int show_progress = 1;
  int reboot_after = 0;
  int compress = 0;

  char partitions = (char) DEFAULT;

  while (chosen_item != ITEM_BACK)
	  {
	    get_nandroid_adv_b_menu_opts (items + 1, partitions, "backup", reboot_after, show_progress, compress);	// put the menu options in items[] starting at index 1
	    chosen_item =
	      get_menu_selection (headers, items, 0,
				  chosen_item < 0 ? 0 : chosen_item);

	    switch (chosen_item)
		    {
		    case B_ITEM_PERF:
		      nandroid("backup", filename, partitions, reboot_after, show_progress, compress);
		      break;
		    case B_ITEM_B:
		      partitions ^= BOOT;
		      break;
		    case B_ITEM_D:
		      partitions ^= DATA;
		      break;
		    case B_ITEM_A:
		      partitions ^= ASECURE;
		      break;
		    case B_ITEM_S:
		      partitions ^= SYSTEM;
		      break;
		    case B_ITEM_C:
		      partitions ^= CACHE;
		      break;
		    case B_ITEM_P:
		      show_progress ^= 1;
		      break;
		    case B_ITEM_Z:
		      compress ^= 1;
		      break;
		    case B_ITEM_R:
		      reboot_after ^= 1;
		      break;
		    }
	  }
}


void
show_nandroid_menu ()
{
  static char *headers[] = { "Nandroid Menu",
    "",
    NULL
  };
  static char *items[] = { "Nandroid Backup",
    "Nandroid Restore",
    "Compress existing backup",
    "Delete backup",
    NULL
  };

#define ITEM_ADV_BACKUP  0
#define ITEM_ADV_RESTORE 1
#define ITEM_COMPRESS    2
#define ITEM_DELETE	 3

  int chosen_item = -1;

  while (chosen_item != ITEM_BACK)
	  {
	    chosen_item =
	      get_menu_selection (headers, items, 0,
				  chosen_item < 0 ? 0 : chosen_item);

	    switch (chosen_item)
		    {
		    case ITEM_ADV_BACKUP:
		      show_nandroid_adv_b_menu ();
		      break;
		    case ITEM_ADV_RESTORE:
		      show_nandroid_adv_r_menu ();
		      break;
		    case ITEM_COMPRESS:
		      show_compress_menu();
		      break;
		    case ITEM_DELETE:
		      show_delete_menu();
		      break;
		    }
	  }
}
