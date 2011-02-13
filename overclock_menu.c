#include <stdlib.h>
#include <stdio.h>

#include "recovery_lib.h"
#include "roots.h"
#include "recovery_ui.h"

void set_oc(char* speed) {
	FILE* f = fopen("/cache/oc","w");
    fputs(speed, f);
	fputs("\n",f);
	fclose(f);
	set_cpufreq(speed);
}
	
void show_overclock_menu() {
	
    static char* headers[] = { "Overclock Menu",
			       "Make a selection of press POWER/DEL to return",
			       "",
			       NULL };

    char* items[] = { "600 MHz",
				"800 Mhz",
				"900 MHz",
				"1000 MHz",
				"1100 MHz",
		      NULL };
			  
#define oc600         	0
#define oc800			1
#define oc900		    2
#define oc1000     		3
#define oc1100	    	4

int chosen_item = -1;

    while(chosen_item!=ITEM_BACK) {
	chosen_item = get_menu_selection(headers,items,1,chosen_item<0?0:chosen_item);


        switch (chosen_item) {
	case oc600:
		set_oc("600000");
		ui_print("Maxspeed set to 600 MHz\n");
	    return;
	case oc800:
		set_oc("800000");
		ui_print("Max clockspeed set to 800 MHz\n");
		break;
	case oc900:
		set_oc("900000");
		ui_print("Max clockspeed set to 900 MHz\n");
		break;
	case oc1000:
		set_oc("1000000");
		ui_print("Max clockspeed set to 1000 MHz\n");
		break;
	case oc1100:
		set_oc("1100000");
		ui_print("Max clockspeed set to 1100 MHz\n");
		break;
        }
    }
}
