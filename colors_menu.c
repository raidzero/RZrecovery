#include <stdlib.h>
#include <stdio.h>
#include <sys/reboot.h>

#include "recovery_lib.h"
#include "roots.h"
#include "recovery_ui.h"



void set_color(char red, char green, char blue) {
	char txt;
	if ( green >= 150) {
		txt = 0;
	} else { 
		txt = 255;
	}
	FILE *fp = fopen ("/sdcard/RZR/rgb", "wb");
	fwrite(&red, 1, 1, fp);
	fwrite(&green, 1, 1, fp);
	fwrite(&blue, 1, 1, fp);
	fwrite(&txt, 1, 1, fp);
	fclose(fp);
}

void set_random() {
	char cR = rand() % 255;
	char cG = rand() % 255;
	char cB = rand() % 255;
	char txt;
	if ( cG >= 150) {
		txt = 0;
	} else { 
		txt = 255;
	}
	
	FILE *fp = fopen ("/sdcard/RZR/rgb", "wb");
	fwrite(&cR, 1, 1, fp);
	fwrite(&cG, 1, 1, fp);
	fwrite(&cB, 1, 1, fp);
	fwrite(&txt, 1, 1, fp);
	fclose(fp);		
}

void set_manual_d() {
	char R[3];
	char G[3];
	char B[3];
	int cR;
	int cG;
	int cB;
		ui_print("\nPlease enter decimal values.");
		ui_print("\nHold down Alt to enter numbers.");
		ui_print("\nEnter the value for Red (0-255): ");
	    ui_read_line_n(R,3);
		ui_print("\nEnter the value for Green (0-255): ");
	    ui_read_line_n(G,3);
		ui_print("\nEnter the value for Blue (0-255): ");
	    ui_read_line_n(B,3);
		
		cR = atoi(R);
		cG = atoi(G);
		cB = atoi(B);
		
		//check if value is over 255
		if ( cR > 255 ) {
			cR = 255;
		}
		if ( cG > 255 ) {
			cG = 255;
		}
		if ( cB > 255 ) {
			cB = 255;
		}
		set_color(cR,cG,cB);
		return;
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
				"Gold",
				"Manual",
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
#define GOLD			10
#define MANUALD			11

int chosen_item = -1;

    while(chosen_item!=ITEM_BACK) {
	chosen_item = get_menu_selection(headers,items,1,chosen_item<0?0:chosen_item);

        switch (chosen_item) {
	case RANDOM:
		set_random();
		break;
	case BLUE:	
		set_color(54,74,255);
		break;
	case CYAN:
		set_color(0,255,255);
		break;
	case GREEN:
		set_color(0,255,74); 
		break;
	case ORANGE:
		set_color(255,115,0);
		break;
	case PINK:
		set_color(255,0,255);
		break;
	case PURPLE:
		set_color(175,0,255);
		break;
	case RED:
		set_color(255,0,0);
		break;
	case SMOKED:
		set_color(200,200,200);
		break;
	case YELLOW:
		set_color(255,255,0);
		break;
	case GOLD:
		set_color(255,204,102);
		break;
	case MANUALD:
		set_manual_d();
		break;
        }
    }
}
