#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/reboot.h>
#include <sys/time.h>

#include "recovery.h"
#include "roots.h"
#include "recovery_ui.h"

char* RZR_DIR;

void
set_color (char red, char green, char blue)
{
  char txt;

  if (green >= 150) txt = 0; else txt = 255;
  FILE *fp = fopen ("/tmp/.rzrpref_rgb", "wb");

  fwrite (&red, 1, 1, fp);
  fwrite (&green, 1, 1, fp);
  fwrite (&blue, 1, 1, fp);
  fwrite (&txt, 1, 1, fp);
  fclose (fp);
  if (access ("/tmp/.rzrpref_rnd", F_OK) != -1) remove("/tmp/.rzrpref_rnd");
}

void set_icon (char* icon) {
  __system("rm /tmp/.rzrpref_icon*");
  if (strcmp(icon,"rz")==0) {
    ui_set_background(BACKGROUND_ICON_RZ);
    char RZ_CMD[PATH_MAX];
    sprintf(RZ_CMD, "echo > /tmp/.rzrpref_icon_rz", RZR_DIR);
    __system(RZ_CMD);
  }
  if (strcmp(icon,"rw")==0) { 
    ui_set_background(BACKGROUND_ICON_RW);
    char RW_CMD[PATH_MAX];
    sprintf(RW_CMD, "echo > /tmp/.rzrpref_icon_rw");
    __system(RW_CMD);
  }
  if (strcmp(icon,"gm")==0) { 
    ui_set_background(BACKGROUND_ICON_GM);
    char RW_CMD[PATH_MAX];
    sprintf(RW_CMD, "echo > /tmp/.rzrpref_icon_gm");
    __system(RW_CMD);
  }
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
    set_color(cR, cG, cB);
    remove("/tmp/.rzrpref_rgb");
    __system("echo rnd > /tmp/.rzrpref_rnd"); 
  }
}

void
show_colors_menu ()
{
  RZR_DIR = get_rzr_dir();
  printf("RZR_DIR: %s\n", RZR_DIR);
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
    "RootzWiki",
    "Rave mode",
    "Galmin",
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
#define GALM			15

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
		      set_color (54, 74, 255);
		      break;
		    case CYAN:
		      set_icon("rz");
		      set_color (0, 255, 255);
		      break;
		    case GREEN:
		      set_icon("rz");
		      set_color (30, 247, 115);
		      break;
		    case ORANGE:
		      set_icon("rz");
		      set_color (255, 115, 0);
		      break;
		    case PINK:
		      set_icon("rz");
		      set_color (255, 0, 255);
		      break;
		    case PURPLE:
		      set_icon("rz");
		      set_color (175, 0, 255);
		      break;
		    case RED:
		      set_icon("rz");
		      set_color (255, 0, 0);
		      break;
		    case SMOKED:
		      set_icon("rz");
		      set_color (200, 200, 200);
		      break;
		    case YELLOW:
		      set_icon("rz");
		      set_color (255, 255, 0);
		      break;
		    case GOLD:
		      set_icon("rz");
		      set_color (255, 204, 102);
		      break;
		    case WHITE:
		      set_icon("rz");
		      set_color (255, 255, 255);
		      break;
		    case GREY:
		      set_icon("rz");
		      set_color (100, 100, 100);
		      break;
		    case ROOTZ:
		      set_icon("rw");
		      set_color (139, 181, 79);
		      break;
		    case RAVE:
		      set_icon("rz");
		      set_random (1);
		      break;
		    case GALM:
		      set_icon("gm");
		      set_color (154, 166, 154);
		      break;
		    }
	  }
}
