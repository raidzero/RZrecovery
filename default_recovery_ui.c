/*
 * Copyright (C) 2009 The Android Open Source Project
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

#include <linux/input.h>

#include "recovery_ui.h"
#include "common.h"

char* MENU_HEADERS[] = { "Welcome to RZRecovery",
                         "by raidzero",
			 "",
                         NULL };

char* MENU_ITEMS[] = { "reboot android",
			"reboot recovery",
			"power off",
			"extras",
                       "install from sdcard",
                       "wipe data/factory reset",
                       "wipe cache partition",
                       NULL };
		       
		       
int device_recovery_start() {
    return 0;
}

int device_reboot_now(volatile char* key_pressed, int key_code) {
    return 0;
}

int device_handle_key(int key_code) {
    switch (key_code) {
	case KEY_DOWN:
	case KEY_VOLUMEDOWN:
	    return HIGHLIGHT_DOWN;

	case KEY_UP:
	case KEY_VOLUMEUP:
	    return HIGHLIGHT_UP;

	case KEY_ENTER:
	case KEY_CAMERA:
	case KEY_CENTER:
	case KEY_HOME:
	    return SELECT_ITEM;
	
	case KEY_BACK:
	case KEY_END:
	case KEY_POWER:
		return ITEM_BACK;
        }
    return NO_ACTION;
}

int device_perform_action(int which) {
    return which;
}

int device_wipe_data() {
    return 0;
}
