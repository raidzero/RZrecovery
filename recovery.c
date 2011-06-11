/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "bootloader.h"
#include "common.h"
#include "cutils/properties.h"
#include "install.h"
#include "minui/minui.h"
#include "minzip/DirUtil.h"
#include "roots.h"
#include "recovery_ui.h"
#include "flashutils/flashutils.h"
#include "colors_menu.h"
#include "install_menu.h"
#include "recovery_lib.h"
#include "recovery_menu.h"

static const char *TEMPORARY_LOG_FILE = "/tmp/recovery.log";
static const char *SAVE_LOG_FILE = "SDCARD:recovery.log";
static const char *COMMAND_FILE = "CACHE:recovery/command";
static const char *LOG_FILE = "CACHE:recovery/log";

static const struct option OPTIONS[] = {
  { "send_intent", required_argument, NULL, 's' },
  { "update_package", required_argument, NULL, 'u' },
  { "wipe_data", no_argument, NULL, 'w' },
  { "wipe_cache", no_argument, NULL, 'c' },
  { "install_tgz", required_argument, NULL, 'r' },
  { NULL, 0, NULL, 0 },
};

/*
 * The recovery tool communicates with the main system through /cache files.
 *   /cache/recovery/command - INPUT - command line for tool, one arg per line
 *   /cache/recovery/log - OUTPUT - combined log file from recovery run(s)
 *   /cache/recovery/intent - OUTPUT - intent that was passed in
 *
 * The arguments which may be supplied in the recovery.command file:
 *   --send_intent=anystring - write the text out to recovery.intent
 *   --update_package=root:path - verify install an OTA package file
 *   --wipe_data - erase user data (and cache), then reboot
 *   --wipe_cache - wipe cache (but not user data), then reboot
 *   --set_encrypted_filesystem=on|off - enables / diasables encrypted fs
 *
 * After completing, we remove /cache/recovery/command and reboot.
 * Arguments may also be supplied in the bootloader control block (BCB).
 * These important scenarios must be safely restartable at any point:
 *
 * FACTORY RESET
 * 1. user selects "factory reset"
 * 2. main system writes "--wipe_data" to /cache/recovery/command
 * 3. main system reboots into recovery
 * 4. get_args() writes BCB with "boot-recovery" and "--wipe_data"
 *    -- after this, rebooting will restart the erase --
 * 5. erase_root() reformats /data
 * 6. erase_root() reformats /cache
 * 7. finish_recovery() erases BCB
 *    -- after this, rebooting will restart the main system --
 * 8. main() calls reboot() to boot main system
 *
 * OTA INSTALL
 * 1. main system downloads OTA package to /cache/some-filename.zip
 * 2. main system writes "--update_package=CACHE:some-filename.zip"
 * 3. main system reboots into recovery
 * 4. get_args() writes BCB with "boot-recovery" and "--update_package=..."
 *    -- after this, rebooting will attempt to reinstall the update --
 * 5. install_package() attempts to install the update
 *    NOTE: the package install must itself be restartable from any point
 * 6. finish_recovery() erases BCB
 *    -- after this, rebooting will (try to) restart the main system --
 * 7. ** if install failed **
 *    7a. prompt_and_wait() shows an error icon and waits for the user
 *    7b; the user reboots (pulling the battery, etc) into the main system
 * 8. main() calls maybe_install_firmware_update()
 *    ** if the update contained radio/hboot firmware **:
 *    8a. m_i_f_u() writes BCB with "boot-recovery" and "--wipe_cache"
 *        -- after this, rebooting will reformat cache & restart main system --
 *    8b. m_i_f_u() writes firmware image into raw cache partition
 *    8c. m_i_f_u() writes BCB with "update-radio/hboot" and "--wipe_cache"
 *        -- after this, rebooting will attempt to reinstall firmware --
 *    8d. bootloader tries to flash firmware
 *    8e. bootloader writes BCB with "boot-recovery" (keeping "--wipe_cache")
 *        -- after this, rebooting will reformat cache & restart main system --
 *    8f. erase_root() reformats /cache
 *    8g. finish_recovery() erases BCB
 *        -- after this, rebooting will (try to) restart the main system --
 * 9. main() calls reboot() to boot main system
 *
 * ENCRYPTED FILE SYSTEMS ENABLE/DISABLE
 * 1. user selects "enable encrypted file systems"
 * 2. main system writes "--set_encrypted_filesystem=on|off" to
 *    /cache/recovery/command
 * 3. main system reboots into recovery
 * 4. get_args() writes BCB with "boot-recovery" and
 *    "--set_encrypted_filesystems=on|off"
 *    -- after this, rebooting will restart the transition --
 * 5. read_encrypted_fs_info() retrieves encrypted file systems settings from /data
 *    Settings include: property to specify the Encrypted FS istatus and
 *    FS encryption key if enabled (not yet implemented)
 * 6. erase_root() reformats /data
 * 7. erase_root() reformats /cache
 * 8. restore_encrypted_fs_info() writes required encrypted file systems settings to /data
 *    Settings include: property to specify the Encrypted FS status and
 *    FS encryption key if enabled (not yet implemented)
 * 9. finish_recovery() erases BCB
 *    -- after this, rebooting will restart the main system --
 * 10. main() calls reboot() to boot main system
 */

static const int MAX_ARG_LENGTH = 4096;
static const int MAX_ARGS = 100;


//write recovery files from cache to sdcard
void write_files() {
	ensure_path_mounted("/sdcard");
	system("cp /cache/rgb /sdcard/RZR/rgb");
}

//read recovery files from sdcard to cache
void read_files() {
	ensure_path_mounted("/sdcard");
	if( access("/sdcard/RZR/rgb", F_OK ) != -1 ) {
		system("cp /sdcard/RZR/rgb /cache/rgb");		
	} else {
		mkdir("/sdcard/RZR", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		set_color(54,74,255);
	}
	ensure_path_unmounted("/sdcard");
}

// command line args come from, in decreasing precedence:
//   - the actual command line
//   - the bootloader control block (one per line, after "recovery")
//   - the contents of COMMAND_FILE (one per line)
static void
get_args(int *argc, char ***argv) {
    read_files();
    struct bootloader_message boot;
    memset(&boot, 0, sizeof(boot));
#ifndef BOARD_HAS_NO_MISC_PARTITION
    get_bootloader_message(&boot);  // this may fail, leaving a zeroed structure
#endif

    if (boot.command[0] != 0 && boot.command[0] != 255) {
        LOGI("Boot command: %.*s\n", sizeof(boot.command), boot.command);
    }

    if (boot.status[0] != 0 && boot.status[0] != 255) {
        LOGI("Boot status: %.*s\n", sizeof(boot.status), boot.status);
    }

    // --- if arguments weren't supplied, look in the bootloader control block
    if (*argc <= 1) {
        boot.recovery[sizeof(boot.recovery) - 1] = '\0';  // Ensure termination
        const char *arg = strtok(boot.recovery, "\n");
        if (arg != NULL && !strcmp(arg, "recovery")) {
            *argv = (char **) malloc(sizeof(char *) * MAX_ARGS);
            (*argv)[0] = strdup(arg);
            for (*argc = 1; *argc < MAX_ARGS; ++*argc) {
                if ((arg = strtok(NULL, "\n")) == NULL) break;
                (*argv)[*argc] = strdup(arg);
            }
            LOGI("Got arguments from boot message\n");
        } else if (boot.recovery[0] != 0 && boot.recovery[0] != 255) {
            LOGE("Bad boot message\n\"%.20s\"\n", boot.recovery);
        }
    }

    // --- if that doesn't work, try the command file
    if (*argc <= 1) {
        FILE *fp = fopen_root_path(COMMAND_FILE, "r");
        if (fp != NULL) {
            char *argv0 = (*argv)[0];
            *argv = (char **) malloc(sizeof(char *) * MAX_ARGS);
            (*argv)[0] = argv0;  // use the same program name

            char buf[MAX_ARG_LENGTH];
            for (*argc = 1; *argc < MAX_ARGS; ++*argc) {
                if (!fgets(buf, sizeof(buf), fp)) break;
                (*argv)[*argc] = strdup(strtok(buf, "\r\n"));  // Strip newline.
            }

            check_and_fclose(fp, COMMAND_FILE);
            LOGI("Got arguments from %s\n", COMMAND_FILE);
        }
    }

    // --> write the arguments we have back into the bootloader control block
    // always boot into recovery after this (until finish_recovery() is called)
    strlcpy(boot.command, "boot-recovery", sizeof(boot.command));
    strlcpy(boot.recovery, "recovery\n", sizeof(boot.recovery));
    int i;
    for (i = 1; i < *argc; ++i) {
        strlcat(boot.recovery, (*argv)[i], sizeof(boot.recovery));
        strlcat(boot.recovery, "\n", sizeof(boot.recovery));
    }
#ifndef BOARD_HAS_NO_MISC_PARTITION
    set_bootloader_message(&boot);  
#endif
}

static void
print_property(const char *key, const char *name, void *cookie) {
    fprintf(stderr, "%s=%s\n", key, name);
}

int main(int argc, char **argv) {
    // check if we are running an internal command
    if(strstr(argv[0], "recovery") == NULL) {
        // determine which command and run it
        if(strstr(argv[0], "flash_image") != NULL) {
            return flash_image_main(argc, argv);
        } else if(strstr(argv[0], "erase_image") != NULL) {
            return erase_image_main(argc, argv);
        } else if(strstr(argv[0], "dump_image") != NULL) {
            return dump_image_main(argc, argv);
        } else if(strstr(argv[0], "e2fsck") != NULL) {
            return e2fsck_main(argc, argv);
        } else if(strstr(argv[0], "mke2fs") != NULL) {
            return mke2fs_main(argc, argv);
        } else if(strstr(argv[0], "tune2fs") != NULL) {
            return tune2fs_main(argc, argv);
        } 
    }
    time_t start = time(NULL);

    // If these fail, there's not really anywhere to complain...
    freopen(TEMPORARY_LOG_FILE, "a", stdout); setbuf(stdout, NULL);
    freopen(TEMPORARY_LOG_FILE, "a", stderr); setbuf(stderr, NULL);
    fprintf(stderr, "Starting recovery on %s", ctime(&start));

    ui_init();
    get_args(&argc, &argv);

	ui_print("\nWelcome to RZRecovery.  Use the volume up and");
	ui_print("\ndown buttons to move between menu options, the");
	ui_print("\ncamera button to select them, and the power");
	ui_print("\nbutton to back out of a menu.\n\n");

    int previous_runs = 0;
    const char *send_intent = NULL;
    const char *update_package = NULL;
    const char *install_tgz = NULL;
    int wipe_data = 0, wipe_cache = 0/*, mount_sys = 0, mount_data = 0*/;

    int arg;
    while ((arg = getopt_long(argc, argv, "", OPTIONS, NULL)) != -1) {
        switch (arg) {
        case 'p': previous_runs = atoi(optarg); break;
        case 's': send_intent = optarg; break;
        case 'u': update_package = optarg; break;
        case 'w': wipe_data = wipe_cache = 1; break;
        case 'c': wipe_cache = 1; break;
		case 'r': install_tgz = optarg; break;
        case '?':
            LOGE("Invalid command argument\n");
            continue;
        }
    }

    device_recovery_start();

    fprintf(stderr, "Command:");
    for (arg = 0; arg < argc; arg++) {
        fprintf(stderr, " \"%s\"", argv[arg]);
    }
    fprintf(stderr, "\n\n");

    property_list(print_property, NULL);
    fprintf(stderr, "\n");

    int status = INSTALL_SUCCESS;

    if (update_package != NULL) {
        status = install_package(update_package);
        if (status != INSTALL_SUCCESS) ui_print("Installation aborted.\n");
    } else if (install_tgz != NULL) {
		status = install_rom_from_tar(install_tgz);
		if (status != INSTALL_SUCCESS) ui_print("Installation failed.\n");
		} else if (wipe_data) {
			if (device_wipe_data()) status = INSTALL_ERROR;
			if (erase_root("DATA:")) status = INSTALL_ERROR;
			if (wipe_cache && erase_root("CACHE:")) status = INSTALL_ERROR;
			if (status != INSTALL_SUCCESS) ui_print("Data wipe failed.\n");
		} else if (wipe_cache) {
			if (wipe_cache && erase_root("CACHE:")) status = INSTALL_ERROR;
			if (status != INSTALL_SUCCESS) ui_print("Cache wipe failed.\n");
		/*} else if (mount_sys) {
			ui_print("Mounting /system\n");
			ensure_root_path_mounted("SYSTEM:");
		} else if (mount_data) {
			ui_print("Mounting /data\n");
			ensure_root_path_mounted("DATA:");*/
		} else {
        status = INSTALL_ERROR;  // No command specified
    }

    if (status != INSTALL_SUCCESS) ui_set_background(BACKGROUND_ICON_ERROR);
    if (status != INSTALL_SUCCESS || ui_text_visible()) prompt_and_wait();

    // Otherwise, get ready to boot the main system...
    finish_recovery(send_intent);
    ui_print("Rebooting...\n");
    sync();
    reboot(RB_AUTOBOOT);
    return EXIT_SUCCESS;
}
