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

#define MAX_COLS 64
#define MAX_ROWS 32

#define CHAR_WIDTH 10
#define CHAR_HEIGHT 18

#define PROGRESSBAR_INDETERMINATE_STATES 6
#define PROGRESSBAR_INDETERMINATE_FPS 24


#define TEXTCOLOR gr_color(255,255,255,255);



enum { LEFT_SIDE, CENTER_TILE, RIGHT_SIDE, NUM_SIDES };

static pthread_mutex_t gUpdateMutex = PTHREAD_MUTEX_INITIALIZER;
static gr_surface gBackgroundIcon[NUM_BACKGROUND_ICONS];
static gr_surface gProgressBarIndeterminate[PROGRESSBAR_INDETERMINATE_STATES];
static gr_surface gProgressBarEmpty[NUM_SIDES];
static gr_surface gProgressBarFill[NUM_SIDES];

static const struct { gr_surface* surface; const char *name; } BITMAPS[] = {
    { &gBackgroundIcon[BACKGROUND_ICON_INSTALLING], "icon_installing" },
    { &gBackgroundIcon[BACKGROUND_ICON_ERROR],      "icon_error" },
    { &gBackgroundIcon[BACKGROUND_ICON_FIRMWARE_INSTALLING],
      "icon_firmware_install" },
    { &gBackgroundIcon[BACKGROUND_ICON_FIRMWARE_ERROR],
      "icon_firmware_error" },
    { &gProgressBarIndeterminate[0],    "indeterminate1" },
    { &gProgressBarIndeterminate[1],    "indeterminate2" },
    { &gProgressBarIndeterminate[2],    "indeterminate3" },
    { &gProgressBarIndeterminate[3],    "indeterminate4" },
    { &gProgressBarIndeterminate[4],    "indeterminate5" },
    { &gProgressBarIndeterminate[5],    "indeterminate6" },
    { &gProgressBarEmpty[LEFT_SIDE],    "progress_bar_empty_left_round" },
    { &gProgressBarEmpty[CENTER_TILE],  "progress_bar_empty" },
    { &gProgressBarEmpty[RIGHT_SIDE],   "progress_bar_empty_right_round" },
    { &gProgressBarFill[LEFT_SIDE],     "progress_bar_left_round" },
    { &gProgressBarFill[CENTER_TILE],   "progress_bar_fill" },
    { &gProgressBarFill[RIGHT_SIDE],    "progress_bar_right_round" },
    { NULL,                             NULL },
};

static gr_surface gCurrentIcon = NULL;

static enum ProgressBarType {
    PROGRESSBAR_TYPE_NONE,
    PROGRESSBAR_TYPE_INDETERMINATE,
    PROGRESSBAR_TYPE_NORMAL,
} gProgressBarType = PROGRESSBAR_TYPE_NONE;

// Progress bar scope of current operation
static float gProgressScopeStart = 0, gProgressScopeSize = 0, gProgress = 0;
static time_t gProgressScopeTime, gProgressScopeDuration;

// Set to 1 when both graphics pages are the same (except for the progress bar)
static int gPagesIdentical = 0;

// Log text overlay, displayed when a magic key is pressed
static char text[MAX_ROWS][MAX_COLS];
static int text_cols = 0, text_rows = 0;
static int text_col = 0, text_row = 0, text_top = 0;
static int show_text = 0;

static char menu[MAX_ROWS][MAX_COLS];
static int show_menu = 0;
static int menu_top = 0, menu_items = 0, menu_sel = 0;

// Key event input queue
static pthread_mutex_t key_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t key_queue_cond = PTHREAD_COND_INITIALIZER;
static int key_queue[256], key_queue_len = 0;
static volatile char key_pressed[KEY_MAX + 1];

// Clear the screen and draw the currently selected background icon (if any).
// Should only be called with gUpdateMutex locked.
static void draw_background_locked(gr_surface icon)
{
    gPagesIdentical = 0;
    gr_color(0, 0, 0, 255);
    gr_fill(0, 0, gr_fb_width(), gr_fb_height());

    if (icon) {
        int iconWidth = gr_get_width(icon);
        int iconHeight = gr_get_height(icon);
        int iconX = (gr_fb_width() - iconWidth) / 2;
        int iconY = (gr_fb_height() - iconHeight) / 2;
        gr_blit(icon, 0, 0, iconWidth, iconHeight, iconX, iconY);
    }
}

// Draw the progress bar (if any) on the screen.  Does not flip pages.
// Should only be called with gUpdateMutex locked.
static void draw_progress_locked()
{
    if (gProgressBarType == PROGRESSBAR_TYPE_NONE) return;

    int iconHeight = gr_get_height(gBackgroundIcon[BACKGROUND_ICON_INSTALLING]);
    int width = gr_get_width(gProgressBarIndeterminate[0]);
    int height = gr_get_height(gProgressBarIndeterminate[0]);

    int dx = (gr_fb_width() - width)/2;
    int dy = (3*gr_fb_height() + iconHeight - 2*height)/4;

    // Erase behind the progress bar (in case this was a progress-only update)
    gr_color(0, 0, 0, 255);
    gr_fill(dx, dy, width, height);

    if (gProgressBarType == PROGRESSBAR_TYPE_NORMAL) {
        float progress = gProgressScopeStart + gProgress * gProgressScopeSize;
        int pos = (int) (progress * width);

        gr_surface s = (pos ? gProgressBarFill : gProgressBarEmpty)[LEFT_SIDE];
        gr_blit(s, 0, 0, gr_get_width(s), gr_get_height(s), dx, dy);

        int x = gr_get_width(s);
        while (x + (int) gr_get_width(gProgressBarEmpty[RIGHT_SIDE]) < width) {
            s = (pos > x ? gProgressBarFill : gProgressBarEmpty)[CENTER_TILE];
            gr_blit(s, 0, 0, gr_get_width(s), gr_get_height(s), dx + x, dy);
            x += gr_get_width(s);
        }

        s = (pos > x ? gProgressBarFill : gProgressBarEmpty)[RIGHT_SIDE];
        gr_blit(s, 0, 0, gr_get_width(s), gr_get_height(s), dx + x, dy);
    }

    if (gProgressBarType == PROGRESSBAR_TYPE_INDETERMINATE) {
        static int frame = 0;
        gr_blit(gProgressBarIndeterminate[frame], 0, 0, width, height, dx, dy);
        frame = (frame + 1) % PROGRESSBAR_INDETERMINATE_STATES;
    }
}

static void draw_text_line(int row, const char* t) {
    if (t[0] != '\0') {
	gr_text(0, (row+1)*CHAR_HEIGHT-1, t);
    }
}

// Redraw everything on the screen.  Does not flip pages.
// Should only be called with gUpdateMutex locked.
static void draw_screen_locked(void)
{
	int cRv;
	int cGv;
	int cBv;
	
	int tRv;
	int tGv;
	int tBv;
	
	ensure_root_path_mounted("DATA:");
	
	if( access("/data/mrgb", F_OK ) != -1 ) {
		FILE *fp = fopen ("/data/mrgb", "rb");
		fread(&cRv, 1, 1, fp);
		fread(&cGv, 1, 1, fp);
		fread(&cBv, 1, 1, fp);
		fclose(fp);
	} else {
		cRv = 54;
		cGv = 74;
		cBv = 255;
	}
	
	if( access("/data/trgb", F_OK ) != -1 ) {
		FILE *fp = fopen ("/data/trgb", "rb");
		fread(&tRv, 1, 1, fp);
		fread(&tGv, 1, 1, fp);
		fread(&tBv, 1, 1, fp);
		fclose(fp);
	} else {
		tRv = 255;
		tGv = 255;
		tBv = 255;
	}
 
    draw_background_locked(gCurrentIcon);
    draw_progress_locked();

    if (show_text) {
        gr_color(0, 0, 0, 160);
        gr_fill(0, 0, gr_fb_width(), gr_fb_height());

        int i = 0;
        if (show_menu) {
 	    gr_color(cRv,cGv,cBv,255);
		gr_fill(0, (menu_top+menu_sel) * CHAR_HEIGHT,
			gr_fb_width(), (menu_top+menu_sel+1)*CHAR_HEIGHT+1);

            for (; i < menu_top + menu_items; ++i) {
                if (i == menu_top + menu_sel) {
                    gr_color(tRv,tGv,tBv,255);
                    draw_text_line(i, menu[i]);
		    gr_color(cRv,cGv,cBv,255);
			} else {
                    draw_text_line(i, menu[i]);
                }
            }
            gr_fill(0, i*CHAR_HEIGHT+CHAR_HEIGHT/2-1,
                    gr_fb_width(), i*CHAR_HEIGHT+CHAR_HEIGHT/2+1);
            ++i;
        }

        TEXTCOLOR

	    for (; i < text_rows; ++i) {
		draw_text_line(i, text[(i+text_top) % text_rows]);
	    }
    }
}

// Redraw everything on the screen and flip the screen (make it visible).
// Should only be called with gUpdateMutex locked.
static void update_screen_locked(void)
{
    draw_screen_locked();
    gr_flip();
}

// Updates only the progress bar, if possible, otherwise redraws the screen.
// Should only be called with gUpdateMutex locked.
static void update_progress_locked(void)
{
    if (show_text || !gPagesIdentical) {
        draw_screen_locked();    // Must redraw the whole screen
        gPagesIdentical = 1;
    } else {
        draw_progress_locked();  // Draw only the progress bar
    }
    gr_flip();
}

// Keeps the progress bar updated, even when the process is otherwise busy.
static void *progress_thread(void *cookie) {
    for (;;) {
        usleep(1000000 / PROGRESSBAR_INDETERMINATE_FPS);
        pthread_mutex_lock(&gUpdateMutex);

        // update the progress bar animation, if active
        // skip this if we have a text overlay (too expensive to update)
        if (gProgressBarType == PROGRESSBAR_TYPE_INDETERMINATE) {
            update_progress_locked();
        }

        // move the progress bar forward on timed intervals, if configured
        int duration = gProgressScopeDuration;
        if (gProgressBarType == PROGRESSBAR_TYPE_NORMAL && duration > 0) {
            int elapsed = time(NULL) - gProgressScopeTime;
            float progress = 1.0 * elapsed / duration;
            if (progress > 1.0) progress = 1.0;
            if (progress > gProgress) {
                gProgress = progress;
                update_progress_locked();
            }
        }

        pthread_mutex_unlock(&gUpdateMutex);
    }
    return NULL;
}

// Reads input events, handles special hot keys, and adds to the key queue.
static void *input_thread(void *cookie) {
    int rel_sum = 0;
    int fake_key = 0;
    for (;;) {
        // wait for the next key event
        struct input_event ev;
        do {
            ev_get(&ev, 0);

            if (ev.type == EV_SYN) {
                continue;
            } else if (ev.type == EV_REL) {
                if (ev.code == REL_Y) {
                    // accumulate the up or down motion reported by
                    // the trackball.  When it exceeds a threshold
                    // (positive or negative), fake an up/down
                    // key event.
                    rel_sum += ev.value;
                    if (rel_sum > 3) {
                        fake_key = 1;
                        ev.type = EV_KEY;
                        ev.code = KEY_DOWN;
                        ev.value = 1;
                        rel_sum = 0;
                    } else if (rel_sum < -3) {
                        fake_key = 1;
                        ev.type = EV_KEY;
                        ev.code = KEY_UP;
                        ev.value = 1;
                        rel_sum = 0;
                    }
                }
            } else {
                rel_sum = 0;
            }
        } while (ev.type != EV_KEY || ev.code > KEY_MAX);

        pthread_mutex_lock(&key_queue_mutex);
        if (!fake_key) {
            // our "fake" keys only report a key-down event (no
            // key-up), so don't record them in the key_pressed
            // table.
            key_pressed[ev.code] = ev.value;
        }
        fake_key = 0;
        const int queue_max = sizeof(key_queue) / sizeof(key_queue[0]);
        if (ev.value > 0 && key_queue_len < queue_max) {
            key_queue[key_queue_len++] = ev.code;
            pthread_cond_signal(&key_queue_cond);
        }
        pthread_mutex_unlock(&key_queue_mutex);

        if (ev.value > 0 && device_toggle_display(key_pressed, ev.code)) {
            pthread_mutex_lock(&gUpdateMutex);
            show_text = !show_text;
            update_screen_locked();
            pthread_mutex_unlock(&gUpdateMutex);
        }

        if (ev.value > 0 && device_reboot_now(key_pressed, ev.code)) {
            reboot(RB_AUTOBOOT);
        }
    }
    return NULL;
}

void ui_init(void) {
    gr_init();
    ev_init();

    text_col = text_row = 0;
    text_rows = gr_fb_height() / CHAR_HEIGHT;
    if (text_rows > MAX_ROWS) text_rows = MAX_ROWS;
    text_top = 1;

    text_cols = gr_fb_width() / CHAR_WIDTH;
    if (text_cols > MAX_COLS - 1) text_cols = MAX_COLS - 1;

    int i;
    for (i = 0; BITMAPS[i].name != NULL; ++i) {
        int result = res_create_surface(BITMAPS[i].name, BITMAPS[i].surface);
        if (result < 0) {
            if (result == -2) {
                LOGI("Bitmap %s missing header\n", BITMAPS[i].name);
            } else {
                LOGE("Missing bitmap %s\n(Code %d)\n", BITMAPS[i].name, result);
            }
            *BITMAPS[i].surface = NULL;
        }
    }

    pthread_t t;
    pthread_create(&t, NULL, progress_thread, NULL);
    pthread_create(&t, NULL, input_thread, NULL);

    pthread_mutex_lock(&gUpdateMutex);
    show_text = !show_text;
    update_screen_locked();
    pthread_mutex_unlock(&gUpdateMutex);

}

char *ui_copy_image(int icon, int *width, int *height, int *bpp) {
    pthread_mutex_lock(&gUpdateMutex);
    draw_background_locked(gBackgroundIcon[icon]);
    *width = gr_fb_width();
    *height = gr_fb_height();
    *bpp = sizeof(gr_pixel) * 8;
    int size = *width * *height * sizeof(gr_pixel);
    char *ret = malloc(size);
    if (ret == NULL) {
        LOGE("Can't allocate %d bytes for image\n", size);
    } else {
        memcpy(ret, gr_fb_data(), size);
    }
    pthread_mutex_unlock(&gUpdateMutex);
    return ret;
}

void ui_set_background(int icon)
{
    pthread_mutex_lock(&gUpdateMutex);
    gCurrentIcon = gBackgroundIcon[icon];
    update_screen_locked();
    pthread_mutex_unlock(&gUpdateMutex);
}

void ui_show_indeterminate_progress()
{
    pthread_mutex_lock(&gUpdateMutex);
    if (gProgressBarType != PROGRESSBAR_TYPE_INDETERMINATE) {
        gProgressBarType = PROGRESSBAR_TYPE_INDETERMINATE;
        update_progress_locked();
    }
    pthread_mutex_unlock(&gUpdateMutex);
}

void ui_show_progress(float portion, int seconds)
{
    pthread_mutex_lock(&gUpdateMutex);
    gProgressBarType = PROGRESSBAR_TYPE_NORMAL;
    gProgressScopeStart += gProgressScopeSize;
    gProgressScopeSize = portion;
    gProgressScopeTime = time(NULL);
    gProgressScopeDuration = seconds;
    gProgress = 0;
    update_progress_locked();
    pthread_mutex_unlock(&gUpdateMutex);
}

void ui_set_progress(float fraction)
{
    pthread_mutex_lock(&gUpdateMutex);
    if (fraction < 0.0) fraction = 0.0;
    if (fraction > 1.0) fraction = 1.0;
    if (gProgressBarType == PROGRESSBAR_TYPE_NORMAL && fraction > gProgress) {
        // Skip updates that aren't visibly different.
        int width = gr_get_width(gProgressBarIndeterminate[0]);
        float scale = width * gProgressScopeSize;
        if ((int) (gProgress * scale) != (int) (fraction * scale)) {
            gProgress = fraction;
            update_progress_locked();
        }
    }
    pthread_mutex_unlock(&gUpdateMutex);
}

void ui_reset_progress()
{
    pthread_mutex_lock(&gUpdateMutex);
    gProgressBarType = PROGRESSBAR_TYPE_NONE;
    gProgressScopeStart = gProgressScopeSize = 0;
    gProgressScopeTime = gProgressScopeDuration = 0;
    gProgress = 0;
    update_screen_locked();
    pthread_mutex_unlock(&gUpdateMutex);
}

void ui_print(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, 256, fmt, ap);
    va_end(ap);

    fputs(buf, stderr);

    // This can get called before ui_init(), so be careful.
    pthread_mutex_lock(&gUpdateMutex);
    if (text_rows > 0 && text_cols > 0) {
        char *ptr;
        for (ptr = buf; *ptr != '\0'; ++ptr) {
            if (*ptr == '\n' || text_col >= text_cols) {
                text[text_row][text_col] = '\0';
                text_col = 0;
                text_row = (text_row + 1) % text_rows;
                if (text_row == text_top) text_top = (text_top + 1) % text_rows;
            }
            if (*ptr != '\n') text[text_row][text_col++] = *ptr;
        }
        text[text_row][text_col] = '\0';
        update_screen_locked();
    }
    pthread_mutex_unlock(&gUpdateMutex);
}

void ui_start_menu(char** headers, char** items, int sel) {
    int i;
    pthread_mutex_lock(&gUpdateMutex);
    if (text_rows > 0 && text_cols > 0) {
        for (i = 0; i < text_rows; ++i) {
            if (headers[i] == NULL) break;
            strncpy(menu[i], headers[i], text_cols-1);
            menu[i][text_cols-1] = '\0';
        }
        menu_top = i;
        for (; i < text_rows; ++i) {
            if (items[i-menu_top] == NULL) break;
            strncpy(menu[i], items[i-menu_top], text_cols-1);
            menu[i][text_cols-1] = '\0';
        }
        menu_items = i - menu_top;
        show_menu = 1;
        menu_sel = sel;
        update_screen_locked();
    }
    pthread_mutex_unlock(&gUpdateMutex);
}

int ui_menu_select(int sel) {
    int old_sel;
    pthread_mutex_lock(&gUpdateMutex);
    if (show_menu > 0) {
        old_sel = menu_sel;
        menu_sel = sel;
        if (menu_sel < 0) menu_sel = menu_items-1;
        if (menu_sel >= menu_items) menu_sel = 0;
        sel = menu_sel;
        if (menu_sel != old_sel) update_screen_locked();
    }
    pthread_mutex_unlock(&gUpdateMutex);
    return sel;
}

void ui_end_menu() {
    int i;
    pthread_mutex_lock(&gUpdateMutex);
    if (show_menu > 0 && text_rows > 0 && text_cols > 0) {
        show_menu = 0;
        update_screen_locked();
    }
    pthread_mutex_unlock(&gUpdateMutex);
}

int ui_text_visible()
{
    pthread_mutex_lock(&gUpdateMutex);
    int visible = show_text;
    pthread_mutex_unlock(&gUpdateMutex);
    return visible;
}

int ui_wait_key()
{
    pthread_mutex_lock(&key_queue_mutex);
    while (key_queue_len == 0) {
        pthread_cond_wait(&key_queue_cond, &key_queue_mutex);
    }

    int key = key_queue[0];
    memcpy(&key_queue[0], &key_queue[1], sizeof(int) * --key_queue_len);
    pthread_mutex_unlock(&key_queue_mutex);
    return key;
}

int ui_key_pressed(int key)
{
    // This is a volatile static array, don't bother locking
    return key_pressed[key];
}

void ui_clear_key_queue() {
    pthread_mutex_lock(&key_queue_mutex);
    key_queue_len = 0;
    pthread_mutex_unlock(&key_queue_mutex);
}

void ui_char_shifted(char* c) {
    switch(*c) {
    case 'a':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c='\t';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='A';
	}
	break;
    case 'b':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c='+';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='B';
	}
	break;
    case 'c':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c='_';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='C';
	}
	break;
    case 'd':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c='#';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='D';
	}
	break;
    case 'e':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c='3';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='E';
	}
	break;
    case 'f':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c='$';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='F';
	}
	break;
    case 'g':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c='%';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='G';
	}
	break;
    case 'h':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c='=';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='H';
	}
	break;
    case 'i':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c='8';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='I';
	}
	break;
    case 'j':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c='&';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='J';
	}
	break;
    case 'k':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c='*';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='K';
	}
	break;
    case 'l':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c='(';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='L';
	}
	break;
    case 'm':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c='\'';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='M';
	}
	break;
    case 'n':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c='"';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='N';
	}
	break;
    case 'o':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c='9';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='O';
	}
	break;
    case 'p':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c='0';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='P';
	}
	break;
    case 'q':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c='1';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='Q';
	}
	break;
    case 'r':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c='4';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='R';
	}
	break;
    case 's':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c='!';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='S';
	}
	break;
    case 't':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c='5';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='T';
	}
	break;
    case 'u':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c='7';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='U';
	}
	break;
    case 'v':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c='-';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='V';
	}
	break;
    case 'w':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c='2';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='W';
	}
	break;
    case 'x':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c='>';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='X';
	}
	break;
    case 'y':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c='6';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='Y';
	}
	break;
    case 'z':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c='<';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='Z';
	}
	break;
    case '.':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c=':';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='.';
	}
	break;
    case ',':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c=';';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c=',';
	}
	break;
    case '@':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c='~';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='@';
	}
	break;
    case '/':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c='^';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='/';
	}
	break;
    case '?':
	if (ui_key_pressed(KEY_LEFTALT) || ui_key_pressed(KEY_RIGHTALT)) {
	    *c=')';
	}
	else if (ui_key_pressed(KEY_LEFTSHIFT)||ui_key_pressed(KEY_RIGHTSHIFT)) {
	    *c='?';
	}
	break;
    }
}

char ui_get_char() {
    char ret = NULL;
    int key;
    while (ret==NULL) {
	key = ui_wait_key();
	switch(key) {
	case KEY_A:
	    ret='a';
	    break;
	case KEY_B:
	    ret='b';
	    break;
	case KEY_C:
	    ret='c';
	    break;
	case KEY_D:
	    ret='d';
	    break;
	case KEY_E:
	    ret='e';
	    break;
	case KEY_F:
	    ret='f';
	    break;
	case KEY_G:
	    ret='g';
	    break;
	case KEY_H:
	    ret='h';
	    break;
	case KEY_I:
	    ret='i';
	    break;
	case KEY_J:
	    ret='j';
	    break;
	case KEY_K:
	    ret='k';
	    break;
	case KEY_L:
	    ret='l';
	    break;
	case KEY_M:
	    ret='m';
	    break;
	case KEY_N:
	    ret='n';
	    break;
	case KEY_O:
	    ret='o';
	    break;
	case KEY_P:
	    ret='p';
	    break;
	case KEY_Q:
	    ret='q';
	    break;
	case KEY_R:
	    ret='r';
	    break;
	case KEY_S:
	    ret='s';
	    break;
	case KEY_T:
	    ret='t';
	    break;
	case KEY_U:
	    ret='u';
	    break;
	case KEY_V:
	    ret='v';
	    break;
	case KEY_W:
	    ret='w';
	    break;
	case KEY_X:
	    ret='x';
	    break;
	case KEY_Y:
	    ret='y';
	    break;
	case KEY_Z:
	    ret='z';
	    break;
	case KEY_COMMA:
	    ret=',';
	    break;
	case KEY_DOT:
	    ret='.';
	    break;
	case KEY_SLASH:
	    ret='/';
	    break;
	case KEY_QUESTION:
	    ret='?';
	    break;
	case KEY_EMAIL:
	    ret='@';
	    break;
	case KEY_BACKSPACE:
	    ret='\b';
	    break;
	case KEY_ENTER:
	    ret='\n';
	    break;
	}
    }
    ui_char_shifted(&ret);
    return ret;
}

void ui_read_line_n(char* buf, int n)
{
    int i;
    char current = NULL;
    char* tmp = malloc(2*sizeof(char));
    tmp[1]=NULL;
    for (i=0; i<n; i++) {
	current=ui_get_char();
	tmp[0]=current;
	ui_print(tmp);
	if(current=='\b' && i>0) {
	    pthread_mutex_lock(&gUpdateMutex);
	    i-=2;
	    text_col-=2;
	    text[text_row][text_col]='\0';
	    update_screen_locked();
	    pthread_mutex_unlock(&gUpdateMutex);
	}
	else if (current=='\n') {
	    buf[i]=NULL;
	    break;
	}
	else {
	    buf[i]=current;
	}
    }
    buf[n]=NULL;
    return;
}
