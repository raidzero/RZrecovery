#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>

#include "recovery.h"
#include "roots.h"
#include "recovery_ui.h"
#include "plugins_menu.h"

char* backuppath = "/sdcard/nandroid";

char* return_nandroid_path()
{
  return backuppath;
}

void show_battstat ()
{
  FILE *fs = fopen ("/sys/class/power_supply/battery/status", "r");
  char *bstat = calloc (14, sizeof (char));

  fgets (bstat, 14, fs);

  FILE *fc = fopen ("/sys/class/power_supply/battery/capacity", "r");
  char *bcap = calloc (14, sizeof (char));

  fgets (bcap, 4, fc);
  
  int noTemp;

  int caplen = strlen (bcap);

  if (bcap[caplen - 1] == '\n')
	  {
	    bcap[caplen - 1] = 0;
	  }

  bcap = strcat (bcap, "%");
  ui_print ("\nBattery Status: ");
  ui_print ("%s\n", bstat);
  char *cmp;

  if (!(cmp = strstr (bstat, "Unknown")))
	  {
	    ui_print ("Charge Level: ");
	    ui_print ("%s\n", bcap);
	    if (!access ("/sys/class/power_supply/battery/temp", F_OK)) {
		ui_print ("Temperature: ");
		FILE *ft = fopen ("/sys/class/power_supply/battery/temp", "r");
		char *btemp = calloc (13, sizeof (char));
		fgets (btemp, 3, ft);
		btemp = strcat (btemp, " C");
	    	ui_print ("%s\n", btemp);
		fclose(ft);
	    }
	   }
  fclose (fc);
  fclose (fs);
}

int plugins_present(const char* sdpath)
{
  DIR * dir;
  struct dirent *de;
  int present;
  int total = 0;

  ensure_path_mounted ("/sdcard");
  if (access(sdpath, F_OK) != -1)
  {
    dir = opendir (sdpath);
    printf("Dir opened.\n");
    while ((de = readdir (dir)) != NULL)
	{
		if (strcmp(de->d_name + strlen (de->d_name) - 4, ".tar") == 0 || strcmp(de->d_name + strlen (de->d_name) - 4, ".tgz") == 0)
		{
			total++;
		}
	}
	if (closedir(dir) > 0)
	{
	  printf("Failed to close plugin directory.\n");
	}
	if (total > 0) 
	{
	  printf("Plugins found.\n");
	  return 1;
	}
	else
	{
	  printf("No plugins found.\n");
	  return 0;
	}
  }
  else
  {
	printf("Plugins directory not present.\n");
	return 0;
  }
}

void show_nandroid_dir_menu()
{
  char *headers[] = { "Choose a nandroid directory",
    "",
    NULL
  };

  char *items[] = { "/sdcard/nandroid",
    "/sdcard/external_sdcard/nandroid",
    "/emmc/nandroid",
    "/data/media/nandroid",
    NULL
  };

#define sdcard_nandroid 0
#define sdcard_external 1
#define emmc			2
#define data_media		3

  int chosen_item = -1;
   while (chosen_item != ITEM_BACK)
	{
	  chosen_item = get_menu_selection (headers, items, 0, chosen_item < 0 ? 0 : chosen_item);
	  if (chosen_item == ITEM_BACK)
	  {
	    return;
	  }
	  switch (chosen_item)
	  {
		case sdcard_nandroid:
		  backuppath = "/sdcard/nandroid";
		  break;
		case sdcard_external:
		  backuppath = "/sdcard/external_sdcard";
		  break;
		case emmc:
		  backuppath = "/emmc/nandroid";
		  break;
		case data_media:
		  backuppath = "/data/media/nandroid";
		  break;
	  }
	  
	  int status = set_backuppath(backuppath);
	  
	  if (status != -1)
	  {
	    ui_print("Nandroid directory: %s\n", backuppath);
		return;
	  }
	  else
	  {
	    ui_print("Invalid selection: %s!\n", backuppath);
      }
	}
}

int set_backuppath(const char* sdpath) 
{
  char path[PATH_MAX] = "";
  DIR *dir;
  struct dirent *de;
  int total = 0;

  if (ensure_path_mounted (sdpath) != 0) return -1;

  dir = opendir (sdpath);
  if (dir == NULL) return -1;
  
  ensure_path_mounted("/sdcard");
  __system("rm /cache/nandloc");
  FILE *fp;
  fp = fopen("/cache/nandloc", "w");
  fprintf(fp, "%s\0", backuppath);
  fclose(fp);
  
  return 0;
}

void set_repeat_scroll_delay(char *delay)
{
  __system("rm /cache/scroll");
  FILE *fp = fopen("/cache/scroll", "w");
  fprintf(fp, "%s\0", delay);
  fclose(fp);
}
 
void show_repeat_scroll_menu()
{
  static char *headers[] = { "Repeat Scroll Delay",
    "In millseconds. Lower is faster",
    "",
    NULL
  };

  static char *items[] = { "255",
    "245",
    "235",
    "225",
    "215",
    "205",
    "195",
    "185",
    "175",
    "165",
    "155",
    "145",
    "135",
    "125",
    "115",
    "105",
    NULL
  };

#define SPD_255	0
#define SPD_245 1
#define SPD_235 3
#define SPD_225 4
#define SPD_215 5
#define SPD_205 6
#define SPD_195 7
#define SPD_185 8
#define SPD_175 9
#define SPD_165 10
#define SPD_155 11
#define SPD_145 12
#define SPD_135 13
#define SPD_125 14
#define SPD_115 15
#define SPD_105 16

  char *delay;
  int chosen_item = -1;
  while (chosen_item != ITEM_BACK)
  {
    chosen_item = get_menu_selection(headers, items, 1, chosen_item < 0 ? 0: chosen_item);
    delay = items[chosen_item];

    set_repeat_scroll_delay(delay);
    ui_print("Scroll delay is %s milliseconds.\n", delay);
    return;
  }
  return;
}        

void show_options_menu()
{
  static char *headers[] = { "Options",
    "",
    NULL
  };
  
  static char *items[] = {
    "Custom Colors",
	"Recovery Overclocking",
	"Nandroid Location",
	"Repeat-scroll Delay",
	NULL
  };
  
#define OPT_COLORS	0
#define OPT_OVRCLCK	1
#define OPT_NANDLOC	2
#define OPT_RPTSCRL	3

  int chosen_item = -1;
  while (chosen_item != ITEM_BACK)
	  {
	    chosen_item =
	      get_menu_selection (headers, items, 0,
				  chosen_item < 0 ? 0 : chosen_item);


	    switch (chosen_item)
		    {
		    case OPT_COLORS:
		      show_colors_menu ();
		      break;
		    case OPT_OVRCLCK:
		      show_overclock_menu ();
		      break;
		    case OPT_NANDLOC:
		      show_nandroid_dir_menu();
		      break;
		    case OPT_RPTSCRL:
		      show_repeat_scroll_menu();
		      break;
		    }
	  }
}  
  
  

  
void
show_extras_menu ()
{
  static char *headers[] = { "Extras",
    "",
    NULL
  };
  
  char* items[6];
  items[0] = "Show Battery Status";
  items[1] = "View Log";
  if (plugins_present("/sdcard/RZR/plugins")) 
  {
    items[2] = "Plugins";
	items[3] = NULL;
  }
  else items[2] = NULL;	

#define BATT 			0	
#define VIEW_LOG		1
#define PLUGINS			2

  int chosen_item = -1;

  while (chosen_item != ITEM_BACK)
	  {
	    chosen_item =
	      get_menu_selection (headers, items, 0,
				  chosen_item < 0 ? 0 : chosen_item);


	    switch (chosen_item)
		    {
		    case BATT:
		      show_battstat ();
		      break;
		    case VIEW_LOG:
		      view_log();
		      break;
		    case PLUGINS:
		       choose_plugin_menu("/sdcard/RZR/plugins/");
		       break;
		    }
	  }
}
