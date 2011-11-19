#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#include "recovery.h"
#include "roots.h"
#include "recovery_ui.h"

void show_battstat ()
{
  FILE *fs = fopen ("/sys/class/power_supply/battery/status", "r");
  char *bstat = calloc (14, sizeof (char));

  fgets (bstat, 14, fs);

  FILE *fc = fopen ("/sys/class/power_supply/battery/capacity", "r");
  char *bcap = calloc (14, sizeof (char));

  fgets (bcap, 4, fc);
  
  int noTemp;

  int caplen = strlen (bcap);

  if (bcap[caplen - 1] == '\n')
	  {
	    bcap[caplen - 1] = 0;
	  }

  bcap = strcat (bcap, "%");
  ui_print ("\nBattery Status: ");
  ui_print ("%s\n", bstat);
  char *cmp;

  if (!(cmp = strstr (bstat, "Unknown")))
	  {
	    ui_print ("Charge Level: ");
	    ui_print ("%s\n", bcap);
	    if (!access ("/sys/class/power_supply/battery/temp", F_OK)) {
		ui_print ("Temperature: ");
		FILE *ft = fopen ("/sys/class/power_supply/battery/temp", "r");
		char *btemp = calloc (13, sizeof (char));
		fgets (btemp, 3, ft);
		btemp = strcat (btemp, " C");
	    	ui_print ("%s\n", btemp);
		fclose(ft);
	    }
	   }
  fclose (fc);
  fclose (fs);
}

void
flashlight ()
{
  if (access ("/sys/class/leds/spotlight/brightness", F_OK))
	  {
	    ui_print_n ("\nFlashlight not found.\n");
	    return;
	  }
  char brightness[3];
  int bi;
  FILE *flr = fopen ("/sys/class/leds/spotlight/brightness", "r");

  fgets (brightness, 3, flr);
  bi = atoi (brightness);
  FILE *flw = fopen ("/sys/class/leds/spotlight/brightness", "w");

  if (bi == 0)
	  {
	    fputs ("255", flw);
	    fputs ("\n", flw);
	  }
  else
	  {
	    fputs ("0", flw);
	    fputs ("\n", flw);
	  }
  fclose (flr);
  fclose (flw);
}

void
show_extras_menu ()
{
  static char *headers[] = { "Extras",
    "",
    NULL
  };


  char* items[] = { "Custom Colors",
    		"Show Battery Status",
    		"Recovery Overclocking",
    		"ROM Tweaks",
		NULL
  	};

#define COLORS			0
#define BATT 			1	
#define OVERCLOCK	   	2
#define ROMTWEAKS		3

  int chosen_item = -1;

  while (chosen_item != ITEM_BACK)
	  {
	    chosen_item =
	      get_menu_selection (headers, items, 0,
				  chosen_item < 0 ? 0 : chosen_item);


	    switch (chosen_item)
		    {
		    case COLORS:
		      show_colors_menu ();
		      break;
		    case BATT:
		      show_battstat ();
		      break;
		    case OVERCLOCK:
		      show_overclock_menu ();
		      break;
		    case ROMTWEAKS:
		      show_romTweaks_menu();
		      break;
		    }
	  }
}
