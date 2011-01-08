#include <stdlib.h>
#include <stdio.h>

#include "recovery_lib.h"
#include "roots.h"
#include "recovery_ui.h"

void show_options_menu()
{
    static char* headers[] = { "Options",
			       "or press DEL or POWER to return",
			       "",
			       NULL };

    char* items[] = { "Colors",
			 
		      NULL };
			  
#define COLORS         0


int chosen_item = -1;

    while(chosen_item!=ITEM_BACK) {
	chosen_item = get_menu_selection(headers,items,1,chosen_item<0?0:chosen_item);


        switch (chosen_item) {
	case COLORS:
		show_colors_menu();
	    return;

        }
    }
}
