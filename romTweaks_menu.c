#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#include "recovery.h"
#include "roots.h"
#include "recovery_ui.h"

void
disable_OTA ()
{
  ensure_path_mounted ("/system");
  remove ("/system/etc/security/otacerts.zip");
  remove ("/cache/signed-*.*");
  ui_print ("OTA-updates disabled.\n");
  ensure_path_unmounted ("/system");
  return;
}

void
root_menu ()
{

  static char *headers[] = { "ROOT installed ROM?",
	      " ",
	      "Rooting without the superuser app installed",
	      "does nothing. Please install the superuser app",
	      "from the market! (by ChainsDD)",
	      " ",
	      NULL
	    };

  char *items[] = { "No",
    "OK, give me root!",
    NULL
  };

  int chosen_item = get_menu_selection (headers, items, 0, 0);

  if (chosen_item != 1) return;

  ui_print ("\nMounting system...\n");
  ensure_path_mounted ("/system");
  ui_print ("Copying su binary to /system/bin...\n");
  __system("busybox cp /rootfiles/su /system/bin");
  ui_print ("Setting permissions on su binary...\n");
  __system("busybox chown 0:0 /system/bin/su");
  __system("busybox chmod 6755 /system/bin/su");
  ui_print ("Done! Please install superuser APK.\n");
  ensure_path_unmounted ("/system");
  return;
}

void dalvik_menu() {

static char *headers[] = { "Dalvik Bytecode Tweaks",
	  		" ",
			NULL
			};

	  char *items[] = { "Disable Bytecode Verificaion",
	  		"Enable Bytecode Verification",
			NULL
			};

	  int chosen_item = get_menu_selection(headers, items, 0, 0);

	  if (chosen_item == 0) {
		__system("/sbin/RZR-noverify.sh disable");
		ui_print("Disabled Dalvik Bytecode Verification.\n");
		return;
	  }
	  if (chosen_item == 1) {
		__system("/sbin/RZR-noverify.sh enable");
		ui_print("Enabled Dalvik Bytecode Verification.\n");
		return;
	  }
}

void
show_romTweaks_menu ()
{
  static char *headers[] = { "ROM tweaks",
    "",
    NULL
  };


  char* items[] = { "Disable OTA Update Downloads in ROM",
    		"Activate Root Access in ROM",
		"Dalvik Bytecode Tweaks",
    		NULL
  	};

#define OTA			0	
#define ROOT_MENU	    	1		
#define DALVIK			2

  int chosen_item = -1;

  while (chosen_item != ITEM_BACK)
	  {
	    chosen_item =
	      get_menu_selection (headers, items, 0,
				  chosen_item < 0 ? 0 : chosen_item);


	    switch (chosen_item)
		    {
		    case OTA:
		      disable_OTA ();
		      break;
		    case ROOT_MENU:
		      root_menu ();
		      break;
		    case DALVIK:
		      dalvik_menu ();
		      break;
		    }
	  }
}
