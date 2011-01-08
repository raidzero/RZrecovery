#include <linux/input.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>


#include "common.h"
#include "minui/minui.h"
#include "recovery_ui.h"

void getColors() {
 FILE *rgbfile;
 int maxline = 3;
 int i=0, j=0;
 char rgbstring[3][3]; 
rgbfile = fopen("/cache/rgbfile", "r");

if (rgbfile==NULL) {
	ui_print("Can't open RGB file, using defaults\n");

	int cR = 54;
	int cG = 74;
	int cB = 255; 

	return;
}
 while (fgets(rgbstring[i], maxline, rgbfile)) {      

    rgbstring[i][strlen(rgbstring[i])-1] = '\0';      
    i++;
   }
	int cR = rgbstring[1];
	int cG = rgbstring[2];
	int cB = rgbstring[3]; 
  
 fclose(rgbfile);
 
 return;
}