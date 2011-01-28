#include <stdlib.h>
#include <stdio.h>
#include <sys/reboot.h>

#include "recovery_lib.h"
#include "roots.h"
#include "recovery_ui.h"


void set_color(char red, char green, char blue) {
	FILE *fp = fopen ("/data/mrgb", "wb");
	fwrite(&red, 1, 1, fp);
	fwrite(&green, 1, 1, fp);
	fwrite(&blue, 1, 1, fp);
	fclose(fp);
}

void set_ht_color(char red, char green, char blue) {
	FILE *fp = fopen ("/data/trgb", "wb");
	fwrite(&red, 1, 1, fp);
	fwrite(&green, 1, 1, fp);
	fwrite(&blue, 1, 1, fp);
	fclose(fp);
}

void set_random() {

	

	char cR = rand() % 255;
	char cG = rand() % 255;
	char cB = rand() % 255;
	
	if ( cG >= 150 ) {
		set_ht_color(0,0,0);
	} else {
		set_ht_color(255,255,255);
	}
	
	
	FILE *fp = fopen ("/data/mrgb", "wb");
	fwrite(&cR, 1, 1, fp);
	fwrite(&cG, 1, 1, fp);
	fwrite(&cB, 1, 1, fp);
	fclose(fp);		
}
			  

void show_colors_menu() {
    static char* headers[] = { "Choose a color",
			       "or press DEL or POWER to return",
			       "",
			       NULL };

    char* items[] = { "Random",
				"Blue",
				"Cyan",
				"Green",
				"Orange",
				"Pink",
				"Purple",
				"Red",
				"Smoked",
				"Yellow",
		      NULL };
			  
#define RANDOM  		0
#define BLUE			1
#define CYAN			2
#define GREEN			3
#define ORANGE			4
#define PINK			5
#define PURPLE			6
#define RED				7
#define SMOKED			8
#define YELLOW			9

int chosen_item = -1;

    while(chosen_item!=ITEM_BACK) {
	chosen_item = get_menu_selection(headers,items,1,chosen_item<0?0:chosen_item);

        switch (chosen_item) {
	case RANDOM:
		ui_print("\nYou have selected random color.\n");
		set_random();
		break;
	case BLUE:	
		ui_print("\nYou selected blue.\n");
		set_color(54,74,255); // total 383, use white text
		set_ht_color(255,255,255);
		break;
	case CYAN:
		ui_print("\nYou selected cyan.\n");
		set_color(0,255,255); //total 510, use black text
		set_ht_color(0,0,0);
		break;
	case GREEN:
		ui_print("\nYou selected green.\n");
		set_color(0,255,74); // use black text
		set_ht_color(100,100,100);
		break;
	case ORANGE:
		ui_print("\nYou selected orange.\n");
		set_color(255,115,0); //use white text
		set_ht_color(255,255,255);
		break;
	case PINK:
		ui_print("\nYou selected pink.\n");
		set_color(255,0,255); //use white text
		set_ht_color(0,0,0);
		break;
	case PURPLE:
		ui_print("\nYou selected purple.\n");
		set_color(175,0,255); //use white text
		set_ht_color(255,255,255);
		break;
	case RED:
		ui_print("\nYou selected red.\n");
		set_color(255,0,0); //use white text
		set_ht_color(255,255,255);
		break;
	case SMOKED:
		ui_print("\nYou selected smoked.\n");
		set_color(200,200,200); //use black text
		set_ht_color(0,0,0);
		break;
	case YELLOW:
		ui_print("\nYou selected yellow.\n");
		set_color(255,255,0); //use black text
		set_ht_color(0,0,0);
		break;
        }
    }
}
