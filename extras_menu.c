#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#include "recovery.h"
#include "roots.h"
#include "recovery_ui.h"

void
disable_OTA ()
{
  ui_print ("\nDisabling OTA updates in ROM...");
  ensure_path_mounted ("/system");
  remove ("/system/etc/security/otacerts.zip");
  remove ("/cache/signed-*.*");
  ui_print ("\nOTA-updates disabled.\n");
  ensure_path_unmounted ("/system");
  return;
}

void
show_battstat ()
{
  FILE *fs = fopen ("/sys/class/power_supply/battery/status", "r");
  char *bstat = calloc (13, sizeof (char));

  fgets (bstat, 13, fs);

  FILE *fc = fopen ("/sys/class/power_supply/battery/capacity", "r");
  char *bcap = calloc (4, sizeof (char));

  fgets (bcap, 4, fc);

  FILE *ft = fopen ("/sys/class/power_supply/battery/temp", "r");
  char *btemp = calloc (3, sizeof (char));

  fgets (btemp, 3, ft);

  btemp = strcat (btemp, " C");
  int caplen = strlen (bcap);

  if (bcap[caplen - 1] == '\n')
	  {
	    bcap[caplen - 1] = 0;
	  }

  bcap = strcat (bcap, "%");
  ui_print ("\nBattery Status: ");
  ui_print ("%s", bstat);
  char *cmp;

  if (!(cmp = strstr (bstat, "Unknown")))
	  {
	    ui_print ("Charge Level: ");
	    ui_print ("%s", bcap);
	    ui_print ("\nTemperature: ");
	    ui_print ("%s", btemp);
	  }
  fclose (ft);
  fclose (fc);
  fclose (fs);
}

void
flashlight ()
{
  if (access ("/sys/class/leds/spotlight/brightness", F_OK))
	  {
	    ui_print ("\nFlashlight not found.\n");
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
root_menu ()
{

  static char **title_headers = NULL;

  if (title_headers == NULL)
	  {
	    char *headers[] = { "ROOT installed ROM?",
	      " ",
	      "Rooting without the superuser app installed",
	      "does nothing. Please install the superuser app",
	      "from the market! (by ChainsDD)",
	      " ",
	      NULL
	    };
	    title_headers = prepend_title (headers);
	  }

  char *items[] = { " No",
    " OK, give me root!",
    NULL
  };

  int chosen_item = get_menu_selection (title_headers, items, 0, 0);

  if (chosen_item != 1)
	  {
	    return;
	  }

  ui_print ("\nMounting system...");
  ensure_path_mounted ("/system");
  ui_print ("\ncopying su binary to /system/bin...");
  system ("busybox cp /rootfiles/su /system/bin");
  ui_print ("\nSetting permissions on su binary...");
  system ("busybox chown 0:0 /system/bin/su");
  system ("busybox chmod 6755 /system/bin/su");
  ui_print ("\nDone! Please install superuser APK.");
  ensure_path_unmounted ("/system");
  return;
}

void
show_extras_menu ()
{
  static char *headers[] = { "Extras",
    "",
    NULL
  };

  char *items[] = { "Custom Colors",
    "Disable OTA Update Downloads in ROM",
    "Show Battery Status",
    "Toggle Flashlight",
    "Activate Root Access in ROM",
    "Recovery Overclocking",
    NULL
  };

#define COLORS			0
#define OTA			1
#define BATT 			2
#define FLASHLIGHT     		3
#define ROOT_MENU	   	4
#define OVERCLOCK	   	5

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
		    case OTA:
		      disable_OTA ();
		      break;
		    case BATT:
		      show_battstat ();
		      break;
		    case FLASHLIGHT:
		      flashlight ();
		      break;
		    case ROOT_MENU:
		      root_menu ();
		      break;
		    case OVERCLOCK:
		      show_overclock_menu ();
		      break;
		    }
	  }
}
