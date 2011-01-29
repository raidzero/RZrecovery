#include <stdlib.h>
#include <stdio.h>

#include "recovery_lib.h"
#include "roots.h"
#include "recovery_ui.h"

void disable_OTA() {
	ui_print("\nDisabling OTA updates in ROM...");
	ensure_root_path_mounted("SYSTEM:");
	ensure_root_path_mounted("CACHE:");
	remove("/system/etc/security/otacerts.zip");
	remove("/cache/signed-*.*");
	ui_print("\nOTA-updates disabled.\n");
}	

void show_battstat() {
	FILE* fs = fopen("/sys/class/power_supply/battery/status","r");
    char* bstat = calloc(13,sizeof(char));
    fgets(bstat, 13, fs);
	
	FILE* fc = fopen("/sys/class/power_supply/battery/capacity","r");
    char* bcap = calloc(4,sizeof(char));
    fgets(bcap, 4, fc);
	
	ui_print("\nBattery Status: ");
	ui_print(bstat);
	ui_print("Charge Level: ");
	ui_print(bcap);
	ui_print(" %");
}
	
void show_options_menu()
{
    static char* headers[] = { "Options",
			       "or press DEL or POWER to return",
			       "",
			       NULL };

    char* items[] = { "Colors",
				"Disable OTA updating",
				"Show Battery Status",
		      NULL };
			  
#define COLORS         0
#define OTA			   1
#define BATT		   2


int chosen_item = -1;

    while(chosen_item!=ITEM_BACK) {
	chosen_item = get_menu_selection(headers,items,1,chosen_item<0?0:chosen_item);


        switch (chosen_item) {
	case COLORS:
		show_colors_menu();
	    return;
	case OTA:
		disable_OTA();
		break;
	case BATT:
		show_battstat();
		break;

        }
    }
}
