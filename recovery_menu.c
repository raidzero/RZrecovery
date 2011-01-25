#include <unistd.h>
#include <stdio.h>
#include <sys/reboot.h>

#include "common.h"
#include "install.h"
#include "recovery_lib.h"
#include "recovery_ui.h"
#include "minui/minui.h"
#include "roots.h"
#include "firmware.h"

#include "nandroid_menu.h"
#include "mount_menu.h"
#include "install_menu.h"
#include "wipe_menu.h"

extern volatile int do_reboot;

void reboot_recovery()
{
	ui_print("\n-- Rebooting into recovery...\n");
	__reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_RESTART2, "recovery");
}

void poweroff()
{
	ui_print("\n-- Shutting down...");
	__reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_POWER_OFF, NULL);
}


void prompt_and_wait() 
{

    char* menu_headers[] = { "Modded by raidzero",
			     "",
			     NULL };

    char** headers = prepend_title(menu_headers);

    char* items[] = { "Reboot into Android",
		      "Reboot into Recovery",
			  "Power Off",
		      "Wipe Options",
		      "Mount Options",
		      "Backup/Restore",
		      "Install",
		      "Help",
		      NULL };


    char* argv[]={"/sbin/test_menu.sh",NULL};
    char* envp[]={NULL};


#define ITEM_REBOOT          0
#define ITEM_RECOVERY	     1
#define ITEM_POWEROFF		 2
#define ITEM_WIPE_PARTS      3
#define ITEM_MOUNT_MENU      4
#define ITEM_NANDROID_MENU   5
#define ITEM_INSTALL         6
#define ITEM_HELP		     7   

    int chosen_item = -1;
    for (;;) {
        ui_reset_progress();
	if (chosen_item==9999) chosen_item=0;

        chosen_item = get_menu_selection(headers, items, 0, chosen_item<0?0:chosen_item);

        // device-specific code may take some action here.  It may
        // return one of the core actions handled in the switch
        // statement below.
        chosen_item = device_perform_action(chosen_item);

        switch (chosen_item) {
	case ITEM_REBOOT:
		ui_print("\n-- Rebooting into Android...\n");
	    return;
	case ITEM_RECOVERY:
		reboot_recovery();
	    return;
	case ITEM_POWEROFF:
		poweroff();
	    return;
	case ITEM_WIPE_PARTS:
	    show_wipe_menu();
	    break;
	case ITEM_MOUNT_MENU:
	    show_mount_menu();
	    break;
	case ITEM_NANDROID_MENU:
	    show_nandroid_menu();
	    break;
	case ITEM_INSTALL:
	    show_install_menu();
	    break;
	case ITEM_HELP:
		ui_print("\n\n\nHELP/FEATURES:\n");
		ui_print("1.1ghz kernel, wipe menu,\n");
		ui_print("Battery charging,\n");
		ui_print("arbitrary unsigned update.zip install,\n");
		ui_print("arbitrary kernel/recovery img install\n");
		ui_print("rom.tar/tgz scripted install support\n");
		ui_print("\nRecovery images must be in /sdcard/recovery &\n");
		ui_print("end in either -rec.img, _rec.img, .rec.img\n");
		ui_print("\nKernel images must be in /sdcard/kernels &\n");
		ui_print("end in -kernel-boot.img, boot.img, kernel.img\n");
		ui_print("\nWipe Menu: wipes any partition on device\n");
		ui_print("also includes battery statistics wipe\n");
	    break;
        }
    }
}

