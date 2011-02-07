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

//save colors in case recovery is not exited from one of my functions
write_rgb();

void reboot_android() {
	ui_print("\n-- Rebooting into Android...\n");
	write_rgb();
	sync();
	__reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_RESTART2, NULL);
}
void reboot_recovery() {
	ui_print("\n-- Rebooting into recovery...\n");
	write_rgb();
	sync();
	__reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_RESTART2, "recovery");
}

void poweroff() {
	ui_print("\n-- Shutting down...");
	write_rgb();
	sync();
	__reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_POWER_OFF, NULL);
}


void prompt_and_wait() {
	
    char* menu_headers[] = { "Modded by raidzero",
							 " ",
							 NULL };

    char** headers = prepend_title(menu_headers);

    char* items[] = { "Reboot into Android",
		      "Reboot into Recovery",
			  "Power Off",
		      "Wipe Options",
		      "Mount Options",
		      "Backup/Restore",
		      "Install",
			  "Extras",
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
#define ITEM_OPTIONS		 7
#define ITEM_HELP		     8   

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
		reboot_android();
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
	    choose_file_menu("/sdcard/", ".zip", ".tgz", ".tar", "rec.img", "boot.img");
	    break;
	case ITEM_OPTIONS:
		show_options_menu();
		break;
	case ITEM_HELP:
		ui_print("\n*HELP/FEATURES*\n");
		ui_print("1.1ghz low voltage kernel w/ battery charging\n");
		ui_print("arbitrary unsigned update.zip install\n");
		ui_print("arbitrary kernel/recovery img install\n");
		ui_print("One level of subfolders under /sdcard/updates\n");
		ui_print("rom.tar/tgz scripted install support\n");
		ui_print("Recovery images must be in /sdcard/recovery &\n");
		ui_print("end in rec.img\n");
		ui_print("Kernel images must be in /sdcard/kernels &\n");
		ui_print("end in boot.img or kernel.img\n");
		ui_print("Wipe menu: wipes any partition on device\n");
		ui_print("also includes battery statistics &\n");
		ui_print("dalvik-cache wipe.\n");
		ui_print("Options: customize colors, OTA updates\n");
		ui_print("display battery statistics\n");
	    break;
        }
    }
}

