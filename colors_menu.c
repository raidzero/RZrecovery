#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/reboot.h>
#include <sys/time.h>

#include "recovery.h"
#include "roots.h"
#include "recovery_ui.h"

void
set_color (char red, char green, char blue)
{
  char txt;

  if (green >= 150) txt = 0; else txt = 255;
  FILE *fp = fopen ("/cache/rgb", "wb");

  fwrite (&red, 1, 1, fp);
  fwrite (&green, 1, 1, fp);
  fwrite (&blue, 1, 1, fp);
  fwrite (&txt, 1, 1, fp);
  fclose (fp);
  if (access ("/cache/rnd", F_OK) != -1) remove("/cache/rnd");
  ensure_path_mounted ("/sdcard");
  if (access ("/sdcard/RZR/rnd", F_OK) != -1) remove("/sdcard/RZR/rnd");
  write_files ();
}

void
set_random (int rnd)
{
  struct timeval tv;
  struct timezone tz;
  struct tm *tm;

  gettimeofday (&tv, &tz);
  tm = localtime (&tv.tv_sec);
  srand (tv.tv_usec);
  char cR = rand () % 255;
  char cG = rand () % 255;
  char cB = rand () % 255;

  set_color (cR, cG, cB);
  if (rnd == 1)
  {
    remove("/cache/rgb");
    ensure_path_mounted("/sdcard");
    remove("/sdcard/RZR/rgb");
    system ("echo > /cache/rnd"); 
    ensure_path_unmounted("/sdcard");
  }
  write_files ();
}

void
show_colors_menu ()
{
  static char *headers[] = { "Choose a color",
    "",
    NULL
  };

  char *items[] = { "Random",
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
    "White",
    "Grey",
    "Rootzwiki",
    "Rave mode",
    NULL
  };

#define RANDOM  		0
#define BLUE			1
#define CYAN			2
#define GREEN			3
#define ORANGE			4
#define PINK			5
#define PURPLE			6
#define RED			7
#define SMOKED			8
#define YELLOW			9
#define GOLD			10
#define WHITE			11
#define GREY			12
#define ROOTZ			13
#define RAVE			14

  int chosen_item = -1;

  while (chosen_item != ITEM_BACK)
	  {
	    chosen_item =
	      get_menu_selection (headers, items, 0,
				  chosen_item < 0 ? 0 : chosen_item);

	    switch (chosen_item)
		    {
		    case RANDOM:
		      set_random (0);
		      break;
		    case BLUE:
		      set_color (54, 74, 255);
		      break;
		    case CYAN:
		      set_color (0, 255, 255);
		      break;
		    case GREEN:
		      set_color (30, 247, 115);
		      break;
		    case ORANGE:
		      set_color (255, 115, 0);
		      break;
		    case PINK:
		      set_color (255, 0, 255);
		      break;
		    case PURPLE:
		      set_color (175, 0, 255);
		      break;
		    case RED:
		      set_color (255, 0, 0);
		      break;
		    case SMOKED:
		      set_color (200, 200, 200);
		      break;
		    case YELLOW:
		      set_color (255, 255, 0);
		      break;
		    case GOLD:
		      set_color (255, 204, 102);
		      break;
		    case WHITE:
		      set_color (255, 255, 255);
		      break;
		    case GREY:
		      set_color (100, 100, 100);
		      break;
		    case ROOTZ:
		      set_color (164, 198, 57);
		      break;
		    case RAVE:
		      set_random (1);
		      break;
		    }
	  }
}
