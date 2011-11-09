#include <stdlib.h>
#include <stdio.h>

#include "mtdutils/mtdutils.h"
#include "recovery.h"
#include "roots.h"
#include "recovery_ui.h"


int confirm_ext_wipe(char* partition) 
{
  Volume* v = volume_for_path(partition);
  int valid = 0;
  //check if it is already ext
  if (strcmp(v->fs_type,"ext2") == 0 || strcmp(v->fs_type,"ext3") == 0 || strcmp(v->fs_type,"ext4") == 0) 
  {
    valid = 1;  
  }  
  if (!valid) 
  {
    return -1;
  }
  else
  {
  
    char question[PATH_MAX];
    char* headers[] = { question,
      "THIS CANNOT BE UNDONE!",
      "",
      NULL
    };

    char* items[] = { "No",
      "No",
      "Yes, wipe & format ext2",
      "Yes, wipe & format ext3",
      "Yes, wipe & format ext4",
      "No"
      NULL
    };

    int chosen_item = get_menu_selection (headers, items, 0, 0);

    if (chosen_item == 2) 
    {
      //ext2
    }
    if (chosen_item == 3)
    {
      //ext3
    }
    if (chosen_item == 4)
    {
      //ext4
    }
  }  
}
  


int wipe_partition(char* partition) 
{
	write_files();
	char wipe_header[255] = "";
	char path_string[255] = "";
	char operation[255] = "";
	if (!strcmp(partition,"batts")) partition = "Battery statistics"; 
	sprintf(path_string,"/%s", partition);
	sprintf(wipe_header,"Wipe %s?", path_string);
	sprintf(operation,"Yes - wipe %s", path_string);
    
	if (strcmp (partition, "all") == 0)
	{
	    if (confirm_selection("Wipe EVERYTHING?", "Yes - wipe the entire device")) {
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
			ui_print ("\n-- Wiping .android-secure... ");
			ensure_path_mounted ("/sdcard");
			__system ("rm -rf /sdcard/.android-secure/*");
			ui_print ("\n-- Wiping boot...");
			cmd_mtd_erase_raw_partition ("boot");
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
	  
	if (confirm_selection(wipe_header, operation))
	{	
		ui_print("-- Wiping %s... ",partition);
		
		if (strcmp (partition, "boot") == 0)
		{
			cmd_mtd_erase_raw_partition ("boot");
			ui_print("Done.\n");
			ui_reset_progress();
			return 0;
		}
		
		if (strcmp (partition, "Battery statistics") == 0)
		{
			ensure_path_mounted("/data");
			char batt_string[255];
			__system ("rm /data/system/batterystats.bin");
			ensure_path_unmounted("/data");
			ui_print("Done.\n");
			ui_reset_progress();
			return 0;
		 }

		if (strcmp (partition, "android-secure") == 0)
		{
			ensure_path_mounted ("/sdcard");
			__system ("rm -rf /sdcard/.android-secure/*");
			ui_print("Done.\n");
			ui_reset_progress();
			return 0;
		}
		
		if (strcmp (partition, "dalvik-cache") == 0)
		{
			ensure_path_mounted ("/data");
			ensure_path_mounted("/cache");
			__system ("rm -rf /cache/dalvik-cache*");
			__system ("rm -rf /data/dalvik-cache");
			ui_print("Done.\n");
			read_files();
			ui_reset_progress();
			return 0;
		}
		
		if (strcmp(path_string,"/data"))
		{
		  if (volume_present("/datadata")) 
		  {
		    ui_print("\n-- Wiping datadata");
		    ensure_path_unmounted("/datadata");
		    erase_volume("/datadata");
		  }
		}	
		ensure_path_unmounted (path_string);
		if (!erase_volume (path_string))	
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
		read_files();
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
    "Wipe .android-secure",
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


  int chosen_item = -1;

  while (chosen_item != ITEM_BACK)
	  {
	    chosen_item =
	      get_menu_selection (headers, items, 0,
				  chosen_item < 0 ? 0 : chosen_item);


	    switch (chosen_item)
		    {
		    case WIPE_ALL:
		      wipe_partition("all");
		      break;

		    case WIPE_SYSTEM:
		      wipe_partition("system");
		      break;

		    case WIPE_DATA:
		      wipe_partition("data");
		      break;

		    case WIPE_AS:
		      wipe_partition("android-secure");
		      break;

		    case WIPE_BOOT:
		      wipe_partition("boot");
		      break;

		    case WIPE_CACHE:
		      wipe_partition("cache");
		      break;

		    case WIPE_BATT:
		      wipe_partition("batts");
		      break;

		    case WIPE_DK:
		      wipe_partition("dalvik-cache");
		      break;
		    }
	  }
}
