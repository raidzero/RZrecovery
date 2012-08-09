#include <string.h>
#include <sys/reboot.h>
#include <unistd.h>

#include "recovery.h"
#include "recovery_ui.h"
#include "rz_funcs.h"

void
reboot_fn(char *action)
{
  if (strcmp(action, "android") == 0
      || strcmp(action, "recovery") == 0
      || strcmp(action, "bootloader") == 0 || strcmp(action, "poweroff") == 0)
  {
    if (strcmp(action, "poweroff") != 0)
    {
      ui_print("\n-- Rebooting into %s --\n", action);
      //write_files();
      sync();
      if (strcmp(action, "android") == 0)
	action = NULL;
      //if (access("/cache/recovery/command",F_OK) != -1) __system("rm /cache/recovery/command");
      //if (access("/cache/update.zip",F_OK) != -1) __system("rm /cache/update.zip");
      if (__reboot
	  (LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
	   LINUX_REBOOT_CMD_RESTART2, action))
      {
	reboot(RB_AUTOBOOT);
      }
    }
    else
    {
      ui_print("\n-- Shutting down --\n");
      //write_files();
      sync();
      if (__reboot
	  (LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
	   LINUX_REBOOT_CMD_POWER_OFF, NULL))
      {
	reboot(RB_AUTOBOOT);
      }
    }
  }
}

void
show_power_menu()
{
  static char *headers[] = { 
    "Power Options",
    "",
    NULL
  };

  static char *items[] = {
    "Reboot android",
    "Reboot recovery",
    "Bootloader",
    "Power off",
    NULL
  };

#define RB_ANDROID 		0
#define RB_RECOVERY		1
#define RB_BOOTLOADER		2
#define RB_POWEROFF             3

  int chosen_item = -1;

  while (chosen_item != ITEM_BACK)
  {
    chosen_item =
      get_menu_selection(headers, items, 0,
			 chosen_item < 0 ? 0 : chosen_item);

    switch (chosen_item)
    {
      case RB_ANDROID:
        reboot_fn("android");
        break;
      case RB_RECOVERY:
        reboot_fn("recovery");
        break;
      case RB_BOOTLOADER:
        reboot_fn("bootloader");
        break;
      case RB_POWEROFF:
        reboot_fn("poweroff");
        break;
    }
  }
}
