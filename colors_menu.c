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

void set_manual_k() {
	int cR = 0;
	int cG = 0;
	int cB = 0;
	ui_print("\nPress R to increment Red   by 10"); //key 19
	ui_print("\nPress G to increment Green by 10");//key 34 
	ui_print("\nPress B to increment Blue  by 10");//key 48
    ui_print("\nPress Enter to save.\n");//key 28
	int key;
    int action;
          do
                {
                    key = ui_wait_key();
                    action = device_handle_key(key, 1);
                    if (key == 19) {
						cR = cR + 10;
						gr_color(cR,cG,cB,255);
						ui_print("\nRed+");
						set_color(cR,cG,cB);
					}
					
					if (key == 34) {
						cG = cG + 10;
						gr_color(cR,cG,cB,255);
						ui_print("\nGreen+");
						set_color(cR,cG,cB);
						gr_fill(0, 0, 10, 10);
					}
					
					if (key == 48) {
						cB = cB + 10;
						gr_color(cR,cG,cB,255);
						ui_print("\nBlue+");
						set_color(cR,cG,cB);
						gr_fill(0, 0, 10, 10);
					}
					if (key != 28) {
					set_color(cR,cG,cB);
					}
                }
                while (key != 28);
				return;
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
				"Manual - keys",
				"Manual - values",
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
#define MANUALK			10
#define MANUALD			11

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
		set_color(54,74,255); //use white text
		set_ht_color(255,255,255);
		break;
	case CYAN:
		ui_print("\nYou selected cyan.\n");
		set_color(0,255,255); //use black text
		set_ht_color(0,0,0);
		break;
	case GREEN:
		ui_print("\nYou selected green.\n");
		set_color(0,255,74); // use black text
		set_ht_color(0,0,0);
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
	case MANUALK:
		set_manual_k();
		break;
	case MANUALD:
		set_manual_d();
		break;
        }
    }
}
