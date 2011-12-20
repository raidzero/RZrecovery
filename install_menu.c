#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/limits.h>
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>
 
#include "common.h"
#include "minui/minui.h"
#include "recovery_ui.h"
#include "roots.h"
#include "install.h"
#include <string.h>
#include <sys/reboot.h>
  
#include "nandroid_menu.h"

char** install_list;
int position = 0;
int backup = 0;
int dwipe = 0;
int cwipe = 0;
char sdpath[PATH_MAX];
char sdpath_[PATH_MAX];

void set_backup()
{
 backup ^= 1;
}

void set_dwipe()
{
 dwipe ^= 1;
}
 
void set_cwipe()
{
 cwipe ^= 1;
}

int fail_silently = 0;

void init_list()
{
  if (position == 0)
  {
	install_list = malloc (50 * sizeof(char*));
    int i;
    for (i=0; i<50; i++)
    {
      install_list[i] = malloc(512);
    }
  }	
}

void update_list(int position, char* filename)
{
  strcpy(install_list[position], filename);
  char *basename = strrchr (filename, '/') + 1;
  ui_print("Queued item %s in position %i.\n", basename, position+1);
}

void queue_item(char* filename)
{
  if (position < 50 )
  {
    update_list(position, filename);
    position++; 
  } 
  else
  {
    ui_print("Maximum queued items is 25.\n");
  }
}

void clear_queue(int silent)
{
  position = 0;
  free(install_list);
  if (!silent) ui_print("Install queue cleared!\n");
}

char **
sortlist (char **list, int total)
{
  int i = 0;

  if (list != NULL)
	  {
	    for (i = 0; i < total; i++)
		    {
		      int curMax = -1;
		      int j;

		      for (j = 0; j < total - i; j++)
			      {
				if (curMax == -1
				     || strcmp (list[curMax], list[j]) < 0)
				  curMax = j;
			      }
		      char *temp = list[curMax];

		      list[curMax] = list[total - i - 1];
		      list[total - i - 1] = temp;
			} 
	  }
  return list;
}

void choose_file_menu (char *sdpath)
{
  /*
  this is just a dummy - to give user somewhere to return to when aborting preinstall/queueing
  and hopefully not get them stuck in a loop of iterations equal to the number of times they
  entered the preinstall menu
  */
  strcpy(sdpath_, sdpath);
  strcat(sdpath_, "/");
  choose_file_menu_final(sdpath_);
  ensure_path_unmounted(sdpath);
}


 void
choose_file_menu_final (char *sdpath)
{
  init_list();
  static char *headers[] = { "Choose item", "", NULL
  };
   char path[PATH_MAX] = "";

  DIR * dir;
  struct dirent *de;
  int total = 0;
  int ftotal = 0;
  int dtotal = 0;
  int i;
  int j;
  char **flist;
  char **dlist;
  char **list;

  if (ensure_path_mounted (sdpath) != 0)
	  {
	    LOGE ("Can't mount %s\n", sdpath);
	    return;
	  }
   dir = opendir (sdpath);
  if (dir == NULL)
	  {
	    LOGE ("Couldn't open directory");
	    LOGE ("Please make sure it exists!");
	    return;
	  }
  
    //count the number of valid files:
    while ((de = readdir (dir)) != NULL)
	  {
	    if (de->d_type == DT_DIR && strcmp (de->d_name, ".") != 0
		 && strcmp (de->d_name, "nandroid") != 0)
		    {
		      dtotal++;
		    }
	    else
		    {
		      if (strcmp
			   (de->d_name + strlen (de->d_name) - 4,
			    ".zip") == 0)
			      {
				ftotal++;
			      }
		      if (strcmp
			   (de->d_name + strlen (de->d_name) - 4,
			    ".tar") == 0)
			      {
				ftotal++;
			      }
		      if (strcmp
			   (de->d_name + strlen (de->d_name) - 4,
			    ".apk") == 0)
			      {
				ftotal++;
			      }
		      if (strcmp
			   (de->d_name + strlen (de->d_name) - 4,
			    ".tgz") == 0)
			      {
				ftotal++;
			      }
		      if (strcmp
			   (de->d_name + strlen (de->d_name) - 7,
			    "rec.img") == 0)
			      {
				ftotal++;
			      }
		      if (strcmp
			   (de->d_name + strlen (de->d_name) - 8,
			    "boot.img") == 0)
			      {
				ftotal++;
			      }
		    }
	  }
  total = ftotal + dtotal;
  if (total == 0)
	  {
	    ui_print ("\nNo valid files found!\n");
	  }
  else
	  {
	    flist = (char **) malloc ((ftotal + 1) * sizeof (char *));
	    flist[ftotal] = NULL;
	     dlist = (char **) malloc ((dtotal + 1) * sizeof (char *));
	    dlist[dtotal] = NULL;
	     list = (char **) malloc ((total + 1) * sizeof (char *));
	    list[total] = NULL;
	     rewinddir (dir);
	     i = 0;		//dir iterator
	    j = 0;		//file iterator
	    while ((de = readdir (dir)) != NULL)
		    {
		      
			//create dirs list
			if (de->d_type == DT_DIR
			    && strcmp (de->d_name, ".") != 0
			    && strcmp (de->d_name, "nandroid") != 0)
			      {
				dlist[i] =
				  (char *) malloc (strlen (sdpath) +
						   strlen (de->d_name) + 1);
				strcpy (dlist[i], sdpath);
				strcat (dlist[i], de->d_name);
				dlist[i] = (char *) malloc (strlen (de->d_name) + 2);	//need one extra byte for the termnitaing char
				strcat (de->d_name, "/\0");	//add "/" followed by terminating character since these are dirs
				strcpy (dlist[i], de->d_name);
				i++;
			      }
		      
			//create files list
			if ((de->d_type == DT_REG
			     &&
			     (strcmp
			      (de->d_name + strlen (de->d_name) - 4,
			       ".zip") == 0
			      || strcmp (de->d_name + strlen (de->d_name) - 4,
					 ".apk") == 0
			      || strcmp (de->d_name + strlen (de->d_name) - 4,
					 ".tar") == 0
			      || strcmp (de->d_name + strlen (de->d_name) - 4,
					 ".tgz") == 0
			      || strcmp (de->d_name + strlen (de->d_name) - 7,
					 "rec.img") == 0
			      || strcmp (de->d_name + strlen (de->d_name) - 8,
					 "boot.img") == 0)))
			      {
				flist[j] =
				  (char *) malloc (strlen (sdpath) +
						   strlen (de->d_name) + 1);
				strcpy (flist[j], sdpath);
				strcat (flist[j], de->d_name);
				flist[j] =
				  (char *) malloc (strlen (de->d_name) + 1);
				strcpy (flist[j], de->d_name);
				j++;
			      }
	    }    if (closedir (dir) < 0)
		    {
		      LOGE ("Failure closing directory\n");
		      return;
		    }
	    
	      //sort lists
	      sortlist (flist, ftotal);
	    sortlist (dlist, dtotal);
	     
	      //join the file and dir list, with dirs on top - thanks cvpcs
	      i = 0;
	    j = 0;
	    int k;

	    for (k = 0; k < total; k++)
		    {
		      if (i < dtotal)
			      {
				list[k] = strdup (dlist[i++]);
			      }
		      else
			      {
				list[k] = strdup (flist[j++]);
			      }
		    }
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
				if (opendir (install_string) == NULL)
					{	//handle selection
					  return preinstall_menu (install_string);
					}
				else
					{
					  return choose_file_menu (install_string);
					}
			      }
		    }
	  }
}

  int
install_img (char *filename, char *partition)
{
  ui_print ("\n-- Flash image from...\n");
  char *basename = strrchr(filename, '/') + 1;
  ui_print ("%s", basename);
  ui_print ("\n");
  
  char *argv[] = { "/sbin/flash_img", partition, filename, NULL };
  char *envp[] = { NULL };

  int status = runve("/sbin/flash_img", argv, envp, 1);
  if (!WIFEXITED (status) || WEXITSTATUS (status) != 0)
  {
    switch(WEXITSTATUS(status))
    {
      case 101: 
        ui_print("flash_image boot success!\n");
        return 0;
      case 102: 
        ui_print("dd flash boot fail!\n");
        return -1;
      case 103:
        ui_print("dd flash boot success!\n");
        return 0;
      case 201:
        ui_print("flash_image recovery success!\n");
        return 0;
      case 202:
        ui_print("dd flash recovery fail!\n");
        return -1;
      case 203:
        ui_print("dd flash recovery success!\n");
        return 0;
    }
  }      
  return 0;
}

 int
install_apk (char *filename, char *location)
{
   ui_print ("\n-- Installing APK from\n");
  ui_print ("%s", filename);
  ui_print ("\nTo\n");
  ensure_path_mounted (location);
   char locString[PATH_MAX];
  char *basename = strrchr (filename, '/') + 1;

   strcpy (locString, location);
  strcat (locString, basename);
   ui_print ("%s", locString);
   char *permString;

   if (strcmp (location, "/data/app/") == 0)
    permString = "1000:1000";
  if (strcmp (location, "/system/app/") == 0)
    permString = "0:0";
   char *argv[] = { "/sbin/busybox ", "cp", filename, location, NULL
  };
   char *envp[] = { NULL };
   runve ("/sbin/busybox", argv, envp, 1);
   char *argy[] =
    { "/sbin/busybox ", "chown", permString, locString, NULL
  };
   char *envy[] = { NULL };
   runve ("/sbin/busybox", argy, envy, 1);
   ui_print ("\nAPK install from sdcard complete.");
  return 0;
}

int install_update_package (char *filename)
{
  char *extension = (filename + strlen (filename) - 3);
  char *boot_extension = (filename + strlen (filename) - 8);
  char *rec_extension = (filename + strlen (filename) - 7);
  char *apk_extension = (filename + strlen (filename) - 3);
  printf("Position: %i\n", position);
  
  if (backup) nandroid("backup", "preinstall", DEFAULT, 1, 0); backup = 0;; //reset it after the first run
  if (dwipe) wipe_partition("data", 1); dwipe = 0;
  if (cwipe) wipe_partition("cache", 1); cwipe = 0;
  if (strcmp (extension, "zip") == 0)
	  {
	    ui_print ("\nZIP detected.\n");
	    remove ("/block_update");
	    if (!install_update_zip (filename)) return -1;
	  }
  if (strcmp (extension, "tar") == 0 || strcmp (extension, "tgz") == 0)
	  {
	    ui_print ("\nTAR detected.\n");
	    if (!install_rom_from_tar (filename)) return -1;
	  }
  if (strcmp (apk_extension, "apk") == 0)
	  {
	    ui_print ("\nAPK detected.\n");
	    preinstall_apk (filename, "recovery");
	  }
  if (strcmp (rec_extension, "rec.img") == 0)
	  {
	    ui_print ("\nRECOVERY IMAGE detected.");
	    if (!install_img (filename, "recovery")) return -1;
	  }
  if (strcmp (boot_extension, "boot.img") == 0)
	  {
	    ui_print ("\nBOOT IMAGE detected.");
	    if (!install_img (filename, "boot")) return -1;
	  }
  return 0;
}

int install_queued_items() 
{
   int i;
   char* filename;
   for (i=0; i<position; i++) 
   {
	 filename = install_list[i];
	 ui_print("Installing item %i of %i from queue. \n", i+1, position);
	 if (!install_update_package(filename))
	 {
	   ui_print("Install failed! Aborting queue...\n");
	   clear_queue(0);
	   return 101;
	 }
   }
   clear_queue(1);
   return 0;
}

 int
install_update_zip (char *filename)
{
  char *path = NULL;

  puts (filename);
  ui_print ("\n-- Install update.zip from sdcard...\n");
  //set_sdcard_update_bootloader_message ();
  // ah ha! so THAT was the problem!
  ui_print ("Attempting update from...\n");
  ui_print ("%s", filename);
  ui_print ("\n");
  int status = install_package (filename);

  if (status != INSTALL_SUCCESS)
	  {
	    ui_print ("Installation aborted.\n");
	    return -1;
	  }
  else
	  {
	    ui_print ("\nInstall from sdcard complete.\n");
	  }
  return 0;
}

 int
install_rom_from_tar (char *filename) 
{
  ui_print ("Attempting to install ROM from ");
  ui_print ("%s", filename);
  ui_print ("...\n");
   char *argv[] =
    { "/sbin/nandroid-mobile.sh", "--install-rom", filename, "--progress",
NULL
  };
   char *envp[] = { NULL };
   int status = runve ("/sbin/nandroid-mobile.sh", argv, envp, 1);

  if (!WIFEXITED (status) || WEXITSTATUS (status) != 0)
	  {
	    ui_printf_int ("ERROR: install exited with status %d\n",
			    WEXITSTATUS (status));
	    __system("cd /tmp; for files in `find . | grep -v rzrpref_`; do rm -rf $files; done");
	    return WEXITSTATUS (status);
	  }
  else
	  {
	    ui_print ("(done)\n");
	  }
  __system("cd /tmp; for files in `find . | grep -v rzrpref_`; do rm -rf $files; done");	  
  ui_reset_progress ();
  return 0;
}

void
preinstall_apk (char *filename)
{
  char *basename = strrchr (filename, '/') + 1;
  char install_string[PATH_MAX];
  char *location;

  strcpy (install_string, "Install ");
  strcat (install_string, basename);
   char *headers[] =
    { "APK Install Menu", "Where will this APK go?", " ", NULL
  };
   char *items[] =
    { "Abort APK Install", "/data/app", "/system/app", NULL
  };
  
#define ITEM_NO 		0
#define ITEM_DATA 		1
#define ITEM_SYSTEM 	2
  int chosen_item = -1;

  while (chosen_item != ITEM_BACK)
	  {
	    chosen_item =
	      get_menu_selection (headers, items, 1,
				  chosen_item < 0 ? 0 : chosen_item);
	    switch (chosen_item)
		    {
		    case ITEM_NO:
		      chosen_item = ITEM_BACK;
		      return;
		    case ITEM_DATA:
		      ui_print ("Installing to DATA...\n");
		      location = "/data/app/";
		      install_apk (filename, location);
		      return;
		    case ITEM_SYSTEM:
		      ui_print ("Installing to SYSTEM...\n");
		      location = "/system/app/";
		      install_apk (filename, location);
		      return;
		    }
	  }
}

void
get_preinstall_menu_opts (char **preinstall_opts, int reboot_into_android)
{

  char **tmp = malloc (5 * sizeof (char *));
  int i;
  for (i=0; i<4; i++)
  {
    tmp[i] = malloc (strlen ("(*)  Backup before install") * sizeof (char));
  }
  
  sprintf (tmp[0], "(%c) Backup before install", backup ? '*':' ');
  sprintf (tmp[1], "(%c) Wipe cache", cwipe ? '*':' ');
  sprintf (tmp[2], "(%c) Wipe data", dwipe ? '*':' ');
  sprintf (tmp[3], "(%c) Reboot afterwards", reboot_into_android ? '*':' ');
  tmp[4] = NULL;

  char **h = preinstall_opts;
  char **j = tmp;

  for (; *j; j++, h++)
    *h = *j;
}

 void
preinstall_menu (char *filename)
{
  int reboot_into_android = 0;
  char *basename = strrchr (filename, '/') + 1;
  char *current_basename = strrchr (filename, '/') + 1;
  char install_string[PATH_MAX];
  
  if (position > 0 )
  {
    ui_print("\nCurrent queue: \n");
    basename = "item(s) now";
	int z;
	char* install_list_base;
	for (z=0; z<position; z++)
	{
	  install_list_base = strrchr (install_list[z], '/') + 1;
	  ui_print("%i: %s\n", z+1, install_list_base);
	} 
	ui_print("%i: %s\n", position+1, current_basename);
  }
  strcpy (install_string, "Install ");
  strcat (install_string, basename);

  char *headers[] =
  { "Preinstall Menu", "Please make your selections.", " ", NULL
  };
  char *preinstall_opts[] =
  { 
	NULL,
	NULL,
	NULL,
	NULL,
	"Add to install queue", 
	"Clear install queue", 
	"Abort install",
	install_string, 
	NULL
  };
  
#define ITEM_BACKUP		0
#define ITEM_CWIPE 		1
#define ITEM_DWIPE 		2
#define ITEM_REBOOT		3
#define ITEM_QUEUE		4
#define ITEM_CLEAR      5
#define ITEM_NO			6
#define ITEM_INSTALL 	7

  int chosen_item = -1;

  while (chosen_item != ITEM_BACK)
	  {
	    get_preinstall_menu_opts (preinstall_opts, reboot_into_android);
	    chosen_item = get_menu_selection (headers, preinstall_opts, 0, chosen_item < 0 ? 0 : chosen_item);
		
	    switch (chosen_item)
		    {
		    case ITEM_NO:
		      choose_file_menu(sdpath_);
			  return;
		    case ITEM_BACKUP:
		      set_backup();
		      break;
		    case ITEM_DWIPE:
		      set_dwipe();
		      break;
		    case ITEM_CWIPE:
		      set_cwipe();
		      break;
			case ITEM_QUEUE:
			  queue_item(filename);
			  choose_file_menu(sdpath_);
			  return;
			case ITEM_CLEAR:
			  clear_queue(0);
			  choose_file_menu(sdpath_);
			  return;
		    case ITEM_INSTALL:
			  if (position > 0) 
			  {
			    if (install_queued_items() != 101) install_update_package (filename);
			  }
			  else
			  {
			    install_update_package(filename);
			  }
			  if (reboot_into_android)
			  {  
			    reboot_android();
			  }
			  return;
		    case ITEM_REBOOT:
			   reboot_into_android ^= 1;
			   break;
		    }
	  }
}


