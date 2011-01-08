#include <stdlib.h>
#include <stdio.h>
#include <sys/reboot.h>

#include "recovery_lib.h"
#include "roots.h"
#include "recovery_ui.h"

void set_random() {
			int cR = rand() % 256;
			int cG = rand() % 256;
			int cB = rand() % 256;
}

void show_colors_menu()
{
    static char* headers[] = { "Colors",
			       "or press DEL or POWER to return",
			       "",
			       NULL };

    char* items[] = { "Random",
			  NULL };
			  
#define RANDOM  		1

int chosen_item = -1;

    while(chosen_item!=ITEM_BACK) {
	chosen_item = get_menu_selection(headers,items,1,chosen_item<0?0:chosen_item);
	
	char* argv[] = { NULL };
	char* envp[] = { NULL };

        switch (chosen_item) {
	case RANDOM:		
			set_random();
			break;
        }
    }
}
