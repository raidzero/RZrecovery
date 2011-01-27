#include <stdlib.h>
#include <stdio.h>

#include "recovery_lib.h"
#include "roots.h"
#include "recovery_ui.h"

void wipe_all(int confirm) {
    if (confirm) {
        static char** title_headers = NULL;

        if (title_headers == NULL) {
            char* headers[] = { "Confirm complete wipe?",
                                "THIS CAN NOT BE UNDONE.",
                                "",
                                NULL };
            title_headers = prepend_title(headers);
        }

        char* items[] = { " No",
                          " Yes -- Wipe EVERYTHING",   // [7]
                          " No",
                          NULL };

        int chosen_item = get_menu_selection(title_headers, items, 1, 0);
        if (chosen_item != 1) {
            return;
        }
    }
	ui_print("\n-- Wiping System...\n");
    erase_root("SYSTEM:");
    ui_print("System wipe complete.\n");
    ui_print("\n-- Wiping data...\n");
    erase_root("DATA:");
    ui_print("Data wipe complete.\n");
    ui_print("\n-- Wiping boot...\n");
    erase_root("BOOT:");
    ui_print("Boot wipe complete.\n");
	ui_print("\n-- Wiping cache...\n");
    erase_root("CACHE:");
    ui_print("Misc wipe complete.\n");
    ui_print("\n-- Wiping misc...\n");
    erase_root("MISC:");
    ui_print("Misc wipe complete.\n");
	ui_print("Device completely wiped.\n\n");
	ui_print("All that remains is RZR.\n");
}

void wipe_systemp(int confirm) {
    if (confirm) {
        static char** title_headers = NULL;

        if (title_headers == NULL) {
            char* headers[] = { "Confirm wipe of system?",
                                "THIS CAN NOT BE UNDONE.",
                                "",
                                NULL };
            title_headers = prepend_title(headers);
        }

        char* items[] = { " No",
                          " Yes -- Wipe system",   // [7]
                          " No",
                          NULL };

        int chosen_item = get_menu_selection(title_headers, items, 1, 0);
        if (chosen_item != 1) {
            return;
        }
    }
    ui_print("\n-- Wiping system...\n");
    erase_root("SYSTEM:");
    ui_print("System wipe complete.\n");
}

void wipe_datap(int confirm) {
    if (confirm) {
        static char** title_headers = NULL;

        if (title_headers == NULL) {
            char* headers[] = { "Confirm wipe of data?",
                                "THIS CAN NOT BE UNDONE.",
                                "",
                                NULL };
            title_headers = prepend_title(headers);
        }

        char* items[] = { " No",
                          " Yes -- wipe data",   // [7]
                          " No",
                          NULL };

        int chosen_item = get_menu_selection(title_headers, items, 1, 0);
        if (chosen_item != 1) {
            return;
        }
    }
    ui_print("\n-- Wiping data...\n");
    erase_root("DATA:");
    ui_print("Data wipe complete.\n");
}

void wipe_bootp(int confirm) {
    if (confirm) {
        static char** title_headers = NULL;

        if (title_headers == NULL) {
            char* headers[] = { "Confirm wipe of boot?",
                                "THIS CAN NOT BE UNDONE.",
                                "",
                                NULL };
            title_headers = prepend_title(headers);
        }

        char* items[] = { " No",
                          " Yes -- Wipe boot",   // [7]
                          " No",
                          NULL };

        int chosen_item = get_menu_selection(title_headers, items, 1, 0);
        if (chosen_item != 1) {
            return;
        }
    }
    ui_print("\n-- Wiping boot...\n");
    erase_root("BOOT:");
    ui_print("Boot wipe complete.\n");
}

void wipe_cachep(int confirm) {
    if (confirm) {
        static char** title_headers = NULL;

        if (title_headers == NULL) {
            char* headers[] = { "Confirm wipe of cache?",
                                "THIS CAN NOT BE UNDONE.",
                                "",
                                NULL };
            title_headers = prepend_title(headers);
        }

        char* items[] = { " No",
                          " Yes -- Wipe cache",   // [7]
                          " No",
                          NULL };

        int chosen_item = get_menu_selection(title_headers, items, 1, 0);
        if (chosen_item != 1) {
            return;
        }
    }
    ui_print("\n-- Wiping cache...\n");
    erase_root("CACHE:");
    ui_print("Cache wipe complete.\n");
}

void wipe_miscp(int confirm) {
    if (confirm) {
        static char** title_headers = NULL;

        if (title_headers == NULL) {
            char* headers[] = { "Confirm wipe of misc?",
                                "THIS CAN NOT BE UNDONE.",
                                "",
                                NULL };
            title_headers = prepend_title(headers);
        }

        char* items[] = { " No",
                          " Yes -- Wipe misc",   // [7]
                          " No",
                          NULL };

        int chosen_item = get_menu_selection(title_headers, items, 1, 0);
        if (chosen_item != 1) {
            return;
        }
    }
    ui_print("\n-- Wiping misc...\n");
    erase_root("MISC:");
    ui_print("Misc wipe complete.\n");
}

void wipe_batts(int confirm) {
    if (confirm) {
        static char** title_headers = NULL;

        if (title_headers == NULL) {
            char* headers[] = { "Confirm wipe of battery stats?",
                                "THIS CAN NOT BE UNDONE.",
                                "",
                                NULL };
            title_headers = prepend_title(headers);
        }

        char* items[] = { " No",
                          " Yes -- Wipe battery stats",   // [7]
                          " No",
                          NULL };

        int chosen_item = get_menu_selection(title_headers, items, 1, 0);
        if (chosen_item != 1) {
            return;
        }
    }
		ui_print("\n-- Wiping battery stats...\n");
		ensure_root_path_mounted("DATA:"); 
		remove("/data/system/batterystats.bin");
		ui_print("\n Battery Statistics cleared.\n");
}

void wipe_dc(int confirm) {
    if (confirm) {
        static char** title_headers = NULL;

        if (title_headers == NULL) {
            char* headers[] = { "Confirm wipe of dalvik-cache?",
                                "THIS CAN NOT BE UNDONE.",
                                "",
                                NULL };
            title_headers = prepend_title(headers);
        }

        char* items[] = { " No",
                          " Yes -- Wipe dalvik-cache",   // [7]
                          " No",
                          NULL };

        int chosen_item = get_menu_selection(title_headers, items, 1, 0);
        if (chosen_item != 1) {
            return;
        }
    }
		ui_print("\n-- Wiping dalvik-cache...\n");
		ensure_root_path_mounted("DATA:"); 
		remove("/data/dalvik-cache/*");
		ui_print("\n dalvik-cache cleared.\n");
}

void show_wipe_menu()
{
    static char* headers[] = { "Choose an item to wipe",
			       "or press DEL or POWER to return",
			       "USE CAUTION:",
			       "These operations *CANNOT BE UNDONE*",
			       "",
			       NULL };

    char* items[] = { "Wipe All",
			  "Wipe system",
		      "Wipe data",
		      "Wipe boot",
			  "Wipe cache",
		      "Wipe misc",
			  "Wipe battery stats",
			  "Wipe dalvik-cache",
		      NULL };

#define WIPE_ALL			0			  
#define WIPE_SYSTEM         1
#define WIPE_DATA       	2
#define WIPE_BOOT      		3
#define WIPE_CACHE			4
#define WIPE_MISC   		5
#define WIPE_BATT			6
#define WIPE_DK				7

int chosen_item = -1;

    while(chosen_item!=ITEM_BACK) {
	chosen_item = get_menu_selection(headers,items,1,chosen_item<0?0:chosen_item);

        switch (chosen_item) {
		
	case WIPE_ALL:
		wipe_all(ui_text_visible());
		break;
		
	case WIPE_SYSTEM:
		wipe_systemp(ui_text_visible());
		break;
		
	case WIPE_DATA:
		wipe_datap(ui_text_visible());
		break;
		
	case WIPE_BOOT:
		wipe_bootp(ui_text_visible());
	    break;	

	case WIPE_CACHE:    
		wipe_cachep(ui_text_visible());
	    break;
		
	case WIPE_MISC:
		wipe_miscp(ui_text_visible());
	    break;

	case WIPE_BATT:
		wipe_batts(ui_text_visible());
	    break;
	case WIPE_DK:
		wipe_dc(ui_text_visible());
	    break;
        }
    }
}
