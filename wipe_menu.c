#include <stdlib.h>
#include <stdio.h>

#include "mtdutils/mtdutils.h"
#include "recovery.h"
#include "roots.h"
#include "recovery_ui.h"

void wipe_partition(char* title, char* operation, char* partition) {
        static char** title_headers = NULL;

        if (title_headers == NULL) {
            char* headers[] = { title,
                                "THIS CANNOT BE UNDONE!",
                                "",
                                NULL };
            title_headers = prepend_title(headers);
        }

        char* items[] = { " No",
						  " No",
                          operation,
                          " No",
                          NULL };

        int chosen_item = get_menu_selection(title_headers, items, 1, 0);
        if (chosen_item != 2) {
            return;
        }
	
	if( strcmp( partition, "system" ) == 0 ) {
		ui_print("\n-- Wiping system...\n");
		ensure_path_unmounted("/system");
		erase_volume("/system");
		ui_print("System wipe complete.\n");
	}
	if( strcmp( partition, "data" ) == 0 ) {
		ui_print("\n-- Wiping data...\n");
		ensure_path_unmounted("/data");
		erase_volume("/data");
		ui_print("Data wipe complete.\n");
	}
	if( strcmp( partition, "boot" ) == 0 ) {
		ui_print("\n-- Wiping boot...\n");
		cmd_mtd_erase_raw_partition("boot");
		ui_print("Boot wipe complete.\n");
	}
	if( strcmp( partition, "cache" ) == 0 ) {
		ui_print("\n-- Wiping cache...\n");
		ui_print("-- May take a while on gingerbread...\n");
		write_files();
		ensure_path_unmounted("/cache");
		erase_volume("/cache");
		ensure_path_mounted("/cache");
		read_files();
		ui_print("Cache wipe complete.");
	}
	if( strcmp( partition, "batts" ) == 0 ) {
		ui_print("\n-- Wiping battery statistics...\n");
		remove("/data/system/batterystats.bin");
		ui_print("Battery stat wipe complete.\n");
	}	
	if( strcmp( partition, "dalvik-cache" ) == 0 ) {
		ui_print("\n-- Wiping dalvik-cache...\n");
		ensure_path_mounted("/data"); 
		system("rm -rf /data/dalvik-cache/*");	
		ensure_path_mounted("/cache");
		system("rm -rf /cache/dalvik-cache/*");		
		ui_print("\n dalvik-cache cleared.\n");
	}	
	if( strcmp( partition, "android-secure" ) == 0 ) {
		ui_print("\n-- Wiping .android-secure...\n");
		ensure_path_mounted("/sdcard"); 
		remove("/sdcard/.android-secure/*");
		ui_print("\n .android-secure cleared.\n");
	}
	if( strcmp( partition, "all" ) == 0 ) {
		ui_print("\n-- Wiping System...\n");
		ensure_path_unmounted("/system");
		erase_volume("/system");
		ui_print("System wipe complete.\n");
		ui_print("\n-- Wiping data...\n");
		ensure_path_unmounted("/data");
		erase_volume("/data");
		ui_print("Data wipe complete.\n");
		ui_print("\n-- Wiping cache...\n");
		ui_print("-- May take a while on gingerbread...\n");
		write_files();
		ensure_path_unmounted("/cache");
		erase_volume("/cache");
		ensure_path_mounted("/cache");
		read_files();
		ui_print("\n-- Wiping .android-secure...\n");
		ensure_path_mounted("/sdcard"); 
		remove("/sdcard/.android-secure/*");
		ui_print("\n .android-secure cleared.\n");
		ui_print("\n-- Wiping boot...\n");
		cmd_mtd_erase_raw_partition("boot");
		ui_print("Boot wipe complete.\n");
		ui_print("Device completely wiped.\n\n");
		ui_print("All that remains is RZR.\n");
	}
}

void show_wipe_menu()
{
    static char* headers[] = { "Choose a partition to wipe",
			       "USE CAUTION:",
			       "These operations *CANNOT BE UNDONE*",
			       "",
			       NULL };

    char* items[] = { "Wipe all",
			  "Wipe system",
		      "Wipe data",
			  "Wipe .android-secure",
		      "Wipe boot",
			  "Wipe cache",
			  "Wipe battery stats",
			  "Wipe dalvik-cache",
		      NULL };

#define WIPE_ALL			0			  
#define WIPE_SYSTEM         1
#define WIPE_DATA       	2
#define WIPE_AS				3
#define WIPE_BOOT      		4
#define WIPE_CACHE			5
#define WIPE_BATT			6
#define WIPE_DK				7


int chosen_item = -1;

    while(chosen_item != ITEM_BACK) {
	chosen_item = get_menu_selection(headers,items,1,chosen_item<0?0:chosen_item);
	
	
        switch (chosen_item) {
	case WIPE_ALL:
		wipe_partition("Are you sure?", "Yes - wipe EVERYTHING", "all");
		break;
		
	case WIPE_SYSTEM:
		wipe_partition("Are you sure?", "Yes - wipe SYSTEM", "system");
		break;
		
	case WIPE_DATA:
		wipe_partition("Are you sure?", "Yes - wipe DATA", "data");
		break;
		
	case WIPE_AS:
		wipe_partition("Are you sure?", "Yes - wipe ANDROID-SECURE", "android-secure");
	    break;
		
	case WIPE_BOOT:
		wipe_partition("Are you sure?", "Yes - wipe BOOT", "boot");
	    break;	

	case WIPE_CACHE:
		wipe_partition("Are you sure?", "Yes - wipe CACHE", "cache");
	    break;	
		
	case WIPE_BATT:
		wipe_partition("Are you sure?", "Yes - wipe BATTERY STATS", "batts");
	    break;
		
	case WIPE_DK:
		wipe_partition("Are you sure?", "Yes - wipe DALVIK-CACHE", "dalvik-cache");
	    break;
        }
    }
}
