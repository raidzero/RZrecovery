#include <stdlib.h>
#include <stdio.h>

#include "mtdutils/mtdutils.h"
#include "mmcutils/mmcutils.h"
#include "recovery.h"
#include "roots.h"
#include "recovery_ui.h"

char* STORAGE_ROOT;

int wipe_partition(char* partition, int autoaccept) 
{
	char wipe_header[255] = "";
	char path_string[255] = "";
	char operation[255] = "";

	if (strcmp(partition,"battery statistics") != 0 && strcmp(partition,"dalvik-cache") !=0 &&
	strcmp(partition,"android_secure") != 0)
	{
	  sprintf(path_string,"/%s", partition);
	}
	else
	{
	  sprintf(path_string,"%s", partition);
	}
	
	sprintf(wipe_header,"Wipe %s?", path_string);
	sprintf(operation,"Yes - wipe %s", path_string);
    
	if (strcmp (partition, "all") == 0)
	{
	    if (confirm_selection("Wipe EVERYTHING?", "Yes - wipe the entire device", autoaccept)) {
			ui_print("Preparing to wipe everything...\n");
			ui_show_indeterminate_progress();
			write_files();
			ui_print ("\n-- Wiping system... ");
			ensure_path_unmounted ("/system");
			erase_volume ("/system");
			if (volume_present("/datadata")) 
			{
			   ui_print("\n-- Wiping datadata");
			   ensure_path_unmounted("/datadata");
			   erase_volume("/datadata");
                        }
			ui_print ("\n-- Wiping data... ");
			ensure_path_unmounted ("/data");
			erase_volume ("/data");
			ui_print ("\n-- Wiping cache... ");
			ensure_path_unmounted ("/cache");
			erase_volume ("/cache");
			ensure_path_mounted ("/cache");		
			ui_print ("\n-- Wiping .android_secure... ");
			ensure_path_mounted (STORAGE_ROOT);
			__system ("rm -rf %s/.android_secure/*", STORAGE_ROOT);
			ui_print ("\n-- Wiping boot...");
			erase_volume("/boot");
			ui_print ("\nDone.\n");
			ui_print ("Device completely wiped.\n\n");
			ui_print ("All that remains is RZR.\n");
			read_files ();
			ui_reset_progress();
			return 0;
		} else {
		    return -1;
		}
	}
	  
	if (confirm_selection(wipe_header, operation, autoaccept))
	{	
		
		ui_print("-- Wiping %s... ",partition);
		ui_show_indeterminate_progress();
		if (strcmp(path_string, "/cache") == 0) write_files();
		
		if (strcmp (partition, "battery statistics") == 0)
		{
			ensure_path_mounted("/data");
			__system ("rm /data/system/batterystats.bin");
			ensure_path_unmounted("/data");
			ui_print("Done.\n");
			ui_reset_progress();
			return 0;
		 }

		if (strcmp (partition, "android_secure") == 0)
		{
			ensure_path_mounted (STORAGE_ROOT);
			__system ("rm -rf %s/.android_secure/*", STORAGE_ROOT);
			ui_print("Done.\n");
			ui_reset_progress();
			return 0;
		}
		
		if (strcmp (partition, "dalvik-cache") == 0)
		{
			if (!is_path_mounted("/data")) ensure_path_mounted ("/data");
			if (!is_path_mounted("/cache")) ensure_path_mounted("/cache");
			__system ("rm -rf /cache/dalvik-cache/*");
			__system ("rm -rf /data/dalvik-cache/*");
			ui_print("Done.\n");
			read_files();
			ui_reset_progress();
			return 0;
		}
		
		if (strcmp(path_string,"/data") == 0)
		{
		  if (volume_present("/datadata")) 
		  {
		    ui_print("\n-- Wiping datadata");
		    ensure_path_unmounted("/datadata");
		    erase_volume("/datadata");
		    ui_reset_progress();
		  }
		}

		ensure_path_unmounted (path_string);
		if (erase_volume (path_string) == 0)	
		{
			ui_print ("Done.\n", path_string);
			read_files();
			ui_reset_progress();
			return 0;
		} else {
			ui_print("Failed.\n", path_string);
			read_files();
			ui_reset_progress();
			return -1;
		}	
	} else {
		ui_reset_progress();
		return -1;
	}
}


void
show_wipe_menu ()
{
  static char *headers[] = { "Choose a partition to wipe",
    "USE CAUTION:",
    "These operations *CANNOT BE UNDONE*",
    "",
    NULL
  };

  char *items[] = { "Wipe all",
    "Wipe system",
    "Wipe data",
    "Wipe .android_secure",
    "Wipe boot",
    "Wipe cache",
    "Wipe battery stats",
    "Wipe dalvik-cache",
    NULL
  };

#define WIPE_ALL			0
#define WIPE_SYSTEM         1
#define WIPE_DATA       	2
#define WIPE_AS				3
#define WIPE_BOOT      		4
#define WIPE_CACHE			5
#define WIPE_BATT			6
#define WIPE_DK				7


  STORAGE_ROOT = get_storage_root();
  int chosen_item = -1;

  while (chosen_item != ITEM_BACK)
	  {
	    chosen_item =
	      get_menu_selection (headers, items, 0,
				  chosen_item < 0 ? 0 : chosen_item);


	    switch (chosen_item)
		    {
		    case WIPE_ALL:
		      wipe_partition("all", 0);
		      break;

		    case WIPE_SYSTEM:
		      wipe_partition("system", 0);
		      break;

		    case WIPE_DATA:
		      wipe_partition("data", 0);
		      break;

		    case WIPE_AS:
		      wipe_partition("android_secure", 0);
		      break;

		    case WIPE_BOOT:
		      wipe_partition("boot", 0);
		      break;

		    case WIPE_CACHE:
		      wipe_partition("cache", 0);
		      break;

		    case WIPE_BATT:
		      wipe_partition("battery statistics", 0);
		      break;

		    case WIPE_DK:
		      wipe_partition("dalvik-cache", 0);
		      break;
		    }
	  }
}
