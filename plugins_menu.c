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

 void
choose_plugin_menu (char *sdpath)
{
  static char *headers[] = { "Choose plugin", "", NULL
  };
   char path[PATH_MAX] = "";

  DIR * dir;
  struct dirent *de;
  int total = 0;
  int j;
  char **list;

  ensure_path_mounted ("/sdcard");
  dir = opendir (sdpath);

  
    //count the number of valid files:
    while ((de = readdir (dir)) != NULL)
	{
		if (strcmp(de->d_name + strlen (de->d_name) - 4, ".tar") == 0 || strcmp(de->d_name + strlen (de->d_name) - 4, ".tgz") == 0)
		{
			total++;
		}
	}
  if (total == 0)
	  {
	    ui_print ("\nNo plugins found!\n");
	  }
  else
	  {
	    list = (char **) malloc ((total + 1) * sizeof (char *));
	    list[total] = NULL;
	    rewinddir (dir);
	    j = 0;		//file iterator
	    while ((de = readdir (dir)) != NULL)
		    {		      
			//create files list
			if ((de->d_type == DT_REG && (strcmp (de->d_name + strlen (de->d_name) - 4, ".tar") == 0
			|| strcmp (de->d_name + strlen (de->d_name) - 4, ".tgz") == 0)))
			{
				list[j] = (char *) malloc (strlen (sdpath) + strlen (de->d_name) + 1);
				strcpy (list[j], sdpath);
				strcat (list[j], de->d_name);
				list[j] = (char *) malloc (strlen (de->d_name) + 1);
				strcpy (list[j], de->d_name);
				j++;
			}
	    }    
		if (closedir (dir) < 0)
		    {
		      LOGE ("Failure closing directory\n");
		      return;
		    }
	    
	     //sort lists
	    sortlist (list, total);
	     
	    int chosen_item = -1;

	    while (chosen_item < 0)
		    {
		      chosen_item =
			get_menu_selection (headers, list, 0,
					    chosen_item <
					    0 ? 0 : chosen_item);
		      if (chosen_item >= 0 && chosen_item != ITEM_BACK)
			  {
				 char install_string[PATH_MAX];	//create real path from selection

				strcpy (install_string, sdpath);
				strcat (install_string, list[chosen_item]);
				printf("About to execute plugin %s\n", install_string);
				run_plugin(install_string);
			   }
		    }
	  }
}

 int
run_plugin (char *filename) 
{
   char *argv[] =
    { "/sbin/nandroid-mobile.sh", "--install-rom", filename, "--progress", "--plugin",
NULL
  };
   char *envp[] = { NULL };
   int status = runve ("/sbin/nandroid-mobile.sh", argv, envp, 1);

  if (!WIFEXITED (status) || WEXITSTATUS (status) != 0)
	  {
	    ui_printf_int ("ERROR: plugin exited with status %d\n",
			    WEXITSTATUS (status));
		ui_reset_progress ();
	    __system("cd /tmp; for files in `find . | grep -v \"recovery\.log\"`; do rm -rf $files; done");
	    return WEXITSTATUS (status);
	  }
  __system("cd /tmp; for files in `find . | grep -v \"recovery\.log\"`; do rm -rf $files; done");	  
  ui_reset_progress ();
  return 0;
}