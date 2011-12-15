#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>

#include "recovery.h"
#include "roots.h"
#include "recovery_ui.h"
#include "plugins_menu.h"

char* backuppath;

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

  ensure_path_mounted (sdpath);
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
    "Current directory:",
    "",
    "",
    NULL
  };

  char *items[] = { "/sdcard/nandroid",
    "/sdcard/external_sd/nandroid",
    "/emmc/nandroid",
    "/data/media/nandroid",
    NULL
  };

#define sdcard_nandroid 0
#define sdcard_external 1
#define emmc			2
#define data_media		3

  headers[2] = get_nandroid_path();

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
		  backuppath = "/sdcard/external_sd/nandroid";
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
	    ui_print("Volume does not exist: %s!\n", backuppath);
          }
	}
}

int fail_silently;

int get_fail_silently()
{
  return fail_silently;
}

int set_backuppath(const char* sdpath) 
{
  char path[PATH_MAX] = "";
  DIR *dir;
  struct dirent *de;
  int total = 0;
  
  fail_silently = 1;
  if (!volume_present(sdpath)) 
  {
    return -1;
  }

  ensure_path_mounted (sdpath);

  dir = opendir (sdpath);
  if (dir == NULL) 
  {
    char CMD[PATH_MAX];
    sprintf(CMD, "mkdir -p %s", sdpath);
    __system(CMD);
    printf("Created new directory %s\n", sdpath);
  }  
  
  __system("rm /tmp/nandloc");
  FILE *fp;
  fp = fopen("/tmp/nandloc", "w");
  fprintf(fp, "%s\0", backuppath);
  fclose(fp);
  
  return 0;
}

void set_repeat_scroll_delay(char *delay)
{
  __system("rm /tmp/scroll");
  FILE *fp = fopen("/tmp/scroll", "w");
  fprintf(fp, "%s\0", delay);
  fclose(fp);
}

char* get_current_delay()

{
  char* delay = calloc(5, sizeof(char));
  if (access("/tmp/scroll",F_OK) != -1)
  {
    FILE *fp = fopen("/tmp/scroll" ,"r");
    fgets(delay, 5, fp);
  }
  else
  {
    delay = "185";
  }
  return delay;
}

void show_repeat_scroll_menu()
{

  char* keyhold_delay = get_current_delay();
  printf("keyhold_delay: %s\n", keyhold_delay);
  char delay_string[80];

  strcpy(delay_string, "Current delay: ");
  strcat(delay_string, keyhold_delay);
  strcat(delay_string, " ms");
  
  char* headers[] = { "Repeat-scroll delay (lower is faster)", 
    delay_string,
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
    "95",
    "85",
    "75",
    "65",
    "55",
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
#define SPD_95	17
#define SPD_85  18
#define SPD_75	19
#define SPD_65	20
#define SPD_55	21

  char *delay;
  int chosen_item = -1;

  chosen_item = get_menu_selection(headers, items, 0, chosen_item < 0 ? 0: chosen_item);
  if (chosen_item != ITEM_BACK)
  {
    delay = items[chosen_item];

    set_repeat_scroll_delay(delay);
    ui_print("Scroll delay is %s milliseconds.\n", delay);
  }
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
  char* PLUGINS_DIR = get_plugins_dir();
  
  ensure_path_mounted(PLUGINS_DIR);

  printf("Plugins Dir: %s\n", PLUGINS_DIR);
  static char *headers[] = { "Extras",
    "",
    NULL
  };
    
  char* items[6];
  items[0] = "Show Battery Status";
  items[1] = "View Log";
  if (plugins_present(PLUGINS_DIR)) 
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
		       choose_plugin_menu(PLUGINS_DIR);
		       break;
		    }
	  }
  ensure_path_unmounted(PLUGINS_DIR);
}
