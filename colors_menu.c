#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/reboot.h>
#include <sys/time.h>

#include "recovery.h"
#include "roots.h"
#include "recovery_ui.h"

void
set_color (char red, char green, char blue, char bg)
{
  char txt;

  if (green >= 150) txt = 0; else txt = 255;
  FILE *fp = fopen ("/cache/rgb", "wb");

  fwrite (&red, 1, 1, fp);
  fwrite (&green, 1, 1, fp);
  fwrite (&blue, 1, 1, fp);
  fwrite (&txt, 1, 1, fp);
  fwrite (&bg, 1, 1, fp);
  fclose (fp);
  if (access ("/cache/rnd", F_OK) != -1) remove("/cache/rnd");
  ensure_path_mounted ("/sdcard");
  if (access ("/sdcard/RZR/rnd", F_OK) != -1) remove("/sdcard/RZR/rnd");
  write_files ();
}

void set_icon (char* icon) {
  ensure_path_mounted("/sdcard");
  
  __system("rm /sdcard/RZR/icon*");
  __system("rm /cache/icon*");
  if (strcmp(icon,"rz")==0) {
    ui_set_background(BACKGROUND_ICON_RZ);
    __system("echo > /sdcard/RZR/icon_rz && echo > /cache/icon_rz");
  }
  if (strcmp(icon,"rw")==0) { 
    ui_set_background(BACKGROUND_ICON_RW);
    __system("echo > /sdcard/RZR/icon_rw && echo > /cache/icon_rw");
  }
  ensure_path_unmounted("/sdcard");
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

  set_color (cR, cG, cB, 0);
  if (rnd == 1)
  {
    set_color(cR, cG, cB, 0);
    remove("/cache/rgb");
    ensure_path_mounted("/sdcard");
    remove("/sdcard/RZR/rgb");
    __system("echo rnd > /cache/rnd"); 
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
    "RootzWiki (grey)",
    "RootzWiki (black)",
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
#define ROOTZG			13
#define ROOTZB			14
#define RAVE			15

  int chosen_item = -1;

  while (chosen_item != ITEM_BACK)
	  {
	    chosen_item =
	      get_menu_selection (headers, items, 0,
				  chosen_item < 0 ? 0 : chosen_item);

	    switch (chosen_item)
		    {
		    case RANDOM:
		      set_icon("rz");
		      set_random (0);
		      break;
		    case BLUE:
		      set_icon("rz");
		      set_color (54, 74, 255, 0);
		      break;
		    case CYAN:
		      set_icon("rz");
		      set_color (0, 255, 255, 0);
		      break;
		    case GREEN:
		      set_icon("rz");
		      set_color (30, 247, 115, 0);
		      break;
		    case ORANGE:
		      set_icon("rz");
		      set_color (255, 115, 0, 0);
		      break;
		    case PINK:
		      set_icon("rz");
		      set_color (255, 0, 255, 0);
		      break;
		    case PURPLE:
		      set_icon("rz");
		      set_color (175, 0, 255, 0);
		      break;
		    case RED:
		      set_icon("rz");
		      set_color (255, 0, 0, 0);
		      break;
		    case SMOKED:
		      set_icon("rz");
		      set_color (200, 200, 200, 0);
		      break;
		    case YELLOW:
		      set_icon("rz");
		      set_color (255, 255, 0, 0);
		      break;
		    case GOLD:
		      set_icon("rz");
		      set_color (255, 204, 102, 0);
		      break;
		    case WHITE:
		      set_icon("rz");
		      set_color (255, 255, 255, 0);
		      break;
		    case GREY:
		      set_icon("rz");
		      set_color (100, 100, 100, 0);
		      break;
		    case ROOTZG:
		      set_icon("rw");
		      set_color(139, 181, 79, 20);
		      break;
		    case ROOTZB:
		      set_icon("rw");
		      set_color (139, 181, 79, 0);
		      break;
		    case RAVE:
		      set_icon("rz");
		      set_random (1);
		      break;
		    }
	  }
}
