#include <stdlib.h>
#include <stdio.h>

#include "recovery_lib.h"
#include "roots.h"
#include "recovery_ui.h"

void disable_OTA() {
	ui_print("\nDisabling OTA updates in ROM...");
	ensure_root_path_mounted("SYSTEM:");
	remove("/system/etc/security/otacerts.zip");
	remove("/cache/signed-*.*");
	ui_print("\nOTA-updates disabled.\n");
	ensure_root_path_unmounted("SYSTEM:");
	return;
}	

void show_battstat() {
	FILE* fs = fopen("/sys/class/power_supply/battery/status","r");
    char* bstat = calloc(13,sizeof(char));
    fgets(bstat, 13, fs);
	
	FILE* fc = fopen("/sys/class/power_supply/battery/capacity","r");
    char* bcap = calloc(4,sizeof(char));
    fgets(bcap, 4, fc);
	
	FILE* ft = fopen("/sys/class/power_supply/battery/temp","r");
    char* btemp = calloc(3,sizeof(char));
    fgets(btemp, 3, ft);
	
	btemp = strcat(btemp," C");
	bcap = strcat(bcap,"%%");
	ui_print("\n");
	ui_print("\nBattery Status: ");
	ui_print(bstat);
	ui_print("Charge Level: ");
	ui_print(bcap);
	ui_print("\nTemperature: ");
	ui_print(btemp);

	
	fclose(ft);
	fclose(fc);
	fclose(fs);
}

void flashlight() {
	char brightness[3];
	int bi;
	FILE* flr = fopen("/sys/class/leds/spotlight/brightness","r");
    fgets(brightness, 3, flr);
	bi = atoi(brightness);
	FILE* flw = fopen("/sys/class/leds/spotlight/brightness","w");
	if (bi == 0) {
		fputs("255",flw);
		fputs("\n",flw);
	} else { 
		fputs("0",flw);
		fputs("\n",flw);
	}
	fclose(flr);
	fclose(flw);
}

void fix_permissions() {
    char* argv[] = { "/sbin/fix_permissions",
		     NULL };

    char* envp[] = { NULL };
  
	int status = runve("/sbin/fix_permissions",argv,envp,1); 	
	return;
}

void root_menu(int confirm) {
    if (confirm) {
        static char** title_headers = NULL;

        if (title_headers == NULL) {
            char* headers[] = { "ROOT installed ROM?",
								" ",
                                "Rooting without the superuser app installed",
                                "does nothing. Please install the superuser app",
								"from the market! (by ChainsDD)",
								" ",
                                NULL };
            title_headers = prepend_title(headers);
        }

        char* items[] = { " No",
                          " OK, give me root!",
                          NULL };

        int chosen_item = get_menu_selection(title_headers, items, 1, 0);
        if (chosen_item != 1) {
            return;
        }
    }
	char* argv[] = { "/sbin/actroot",
	NULL };

    char* envp[] = { NULL };
	
	ensure_root_path_mounted("SYSTEM:");
	remove("/system/recovery-from-boot.p");
    int status = runve("/sbin/actroot",argv,envp,1);
	ui_print("\nFlash Recovery Service disabled.\n");
	ensure_root_path_unmounted("SYSTEM:");
	return;
}

void show_options_menu()
{
    static char* headers[] = { "Extras",
			       "or press DEL or POWER to return",
			       "",
			       NULL };

    char* items[] = { "Custom Colors",
				"Disable OTA Update Downloads in ROM",
				"Show Battery Status",
				"Toggle Flashlight",
				"Activate Root Access in ROM",
				"Recovery Overclocking",
		      NULL };
			  
#define COLORS         0
#define OTA			   1
#define BATT		   2
#define FLASHLIGHT     3
#define ROOT_MENU	   4
#define OVERCLOCK	   5

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
	case FLASHLIGHT:
		flashlight();
		break;
	case ROOT_MENU:
		root_menu(ui_text_visible());
		break;
	case OVERCLOCK:
		show_overclock_menu();
		break;
        }
    }
}
