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
  
#include "common.h"
#include "minui/minui.h"
#include "recovery_ui.h"
  
#define MAX_COLS 96
#define MAX_ROWS 43
  
#define MENU_MAX_COLS 64
#define MENU_MAX_ROWS 750
  
#define CHAR_WIDTH 10
#define CHAR_HEIGHT 18
  
#define PROGRESSBAR_INDETERMINATE_STATES 6
#define PROGRESSBAR_INDETERMINATE_FPS 15
  
#define TEXTCOLOR gr_color(255,255,255,255);

#ifdef BOARD_HAS_NO_SELECT_BUTTON
static int virtualBack = 1;
#else
static int virtualBack = 1;
#endif
        
#ifdef BOARD_HAS_INVERTED_VOLUME
static int backwardsVolume = 1;
#else
static int backwardsVolume = 0;
#endif

char* BG_ICON;

int virtualBack_toggled()
{
  return virtualBack;
}

int backwardsVolume_toggled()
{
  return backwardsVolume;
}

static pthread_mutex_t gUpdateMutex = PTHREAD_MUTEX_INITIALIZER;
static gr_surface gBackgroundIcon[NUM_BACKGROUND_ICONS];
static gr_surface
  gProgressBarIndeterminate[PROGRESSBAR_INDETERMINATE_STATES];
static gr_surface gProgressBarEmpty;
static gr_surface gProgressBarFill;
 static const struct
{
  gr_surface *surface;
  const char *name;
} BITMAPS[] =
{
  
  { 
  &gBackgroundIcon[BACKGROUND_ICON_RZ], "icon_rz"}, 
  {
  &gBackgroundIcon[BACKGROUND_ICON_RW], "icon_rw"},
  {
  &gBackgroundIcon[BACKGROUND_ICON_GM], "galmin"},
  {
  &gBackgroundIcon[LOADING], "icon_loading"},
  {
  &gProgressBarIndeterminate[0], "indeterminate1"}, 
  {
  &gProgressBarIndeterminate[1], "indeterminate2"}, 
  {
  &gProgressBarIndeterminate[2], "indeterminate3"}, 
  {
  &gProgressBarIndeterminate[3], "indeterminate4"}, 
  {
  &gProgressBarIndeterminate[4], "indeterminate5"}, 
  {
  &gProgressBarIndeterminate[5], "indeterminate6"}, 
  {
  &gProgressBarEmpty, "progress_empty"}, 
  {
  &gProgressBarFill, "progress_fill"}, 
  {
NULL, NULL}, };

 static gr_surface gCurrentIcon = NULL;
 static int ui_log_stdout = 1;

 static enum ProgressBarType
{ PROGRESSBAR_TYPE_NONE, PROGRESSBAR_TYPE_INDETERMINATE,
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
static int menu_show_start = 0;	//menu start position
static char menu[MENU_MAX_ROWS][MENU_MAX_COLS];
static int show_menu = 0;
static int menu_top = 0, menu_items = 0, menu_sel = 0;

 
// Key event input queue
static pthread_mutex_t key_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t key_queue_cond = PTHREAD_COND_INITIALIZER;
static int key_queue[256], key_queue_len = 0;
static volatile char key_pressed[KEY_MAX + 1];

 
// Clear the screen and draw the currently selected background icon (if any).
// Should only be called with gUpdateMutex locked.
  static void
draw_background_locked (gr_surface icon) 
{
  gPagesIdentical = 0;
  gr_color (0, 0, 0, 255);
  gr_fill (0, 0, gr_fb_width (), gr_fb_height ());
   if (icon)
	  {
	    int iconWidth = gr_get_width (icon);
	    int iconHeight = gr_get_height (icon);
	    int iconX = (gr_fb_width () - iconWidth) / 2;
	    int iconY = (gr_fb_height () - iconHeight) / 2;

	    gr_blit (icon, 0, 0, iconWidth, iconHeight, iconX, iconY);
	  }
}  

// Draw the progress bar (if any) on the screen.  Does not flip pages.
// Should only be called with gUpdateMutex locked.
  static void
draw_progress_locked () 
{
  if (gProgressBarType == PROGRESSBAR_TYPE_NONE)
    return;
  int iconHeight;
  if (access("/tmp/.rzrpref_icon_rz", F_OK) != -1)
  {
    iconHeight = gr_get_height (gBackgroundIcon[BACKGROUND_ICON_RZ]);
  }
  else if (access("/tmp/.rzrpref_icon_rw", F_OK) != -1)
  {
    iconHeight = gr_get_height (gBackgroundIcon[BACKGROUND_ICON_RW]);
  }  
  else
  {
    iconHeight = gr_get_height (gBackgroundIcon[BACKGROUND_ICON_GM]);
  }
  int width = gr_get_width (gProgressBarEmpty);
  int height = gr_get_height (gProgressBarEmpty);
  int dx = (gr_fb_width () - width) / 2;
  //int dy = (3 * gr_fb_height () + iconHeight - 2 * height) / 4;
  int dy = (gr_fb_height() - height - 3); /*I dont want the progress bar to be relative to the 
  background icon - just put 22px above the bottom of the screen*/
  // Erase behind the progress bar (in case this was a progress-only update)
    gr_color (0, 0, 0, 255);
  gr_fill (dx, dy, width, height);
   if (gProgressBarType == PROGRESSBAR_TYPE_NORMAL)
	  {
	    float progress =
	      gProgressScopeStart + gProgress * gProgressScopeSize;
	    int pos = (int) (progress * width);

	     if (pos > 0)
		    {
		      gr_blit (gProgressBarFill, 0, 0, pos, height, dx, dy);
		    }
	    if (pos < width - 1)
		    {
		      gr_blit (gProgressBarEmpty, pos, 0, width - pos,
				height, dx + pos, dy);
		    }
	  }
   if (gProgressBarType == PROGRESSBAR_TYPE_INDETERMINATE)
	  {
	    static int frame = 0;

	    gr_blit (gProgressBarIndeterminate[frame], 0, 0, width, height,
		      dx, dy);
	    frame = (frame + 1) % PROGRESSBAR_INDETERMINATE_STATES;
	  }
}  static void

draw_text_line (int row, const char *t)
{
  if (t[0] != '\0')
	  {
	    gr_text (0, (row + 1) * CHAR_HEIGHT - 1, t);
	  }
}

 
// Redraw everything on the screen.  Does not flip pages.
// Should only be called with gUpdateMutex locked.
  static void
draw_screen_locked (void) 
{
  
    //define menu color integers
  char cRv , cGv, cBv, txt, bg;

  if (access ("/tmp/.rzrpref_rnd", F_OK) != -1)
  {
    struct timeval tv;
    struct timezone tz;
    struct tm *tm;

    gettimeofday (&tv, &tz);
    tm = localtime (&tv.tv_sec);
    srand (tv.tv_usec);
    bg = 0;
    cRv = rand () % 255;
    cGv = rand () % 255;
    cBv = rand () % 255;
    if (cGv >= 150) txt = 0; else txt = 255;
  } else {
  
  	if (access ("/tmp/.rzrpref_rgb", F_OK) != -1)
  	{
      		FILE * fp = fopen ("/tmp/.rzrpref_rgb", "rb");
      		fread (&cRv, 1, 1, fp);
      		fread (&cGv, 1, 1, fp);
     		fread (&cBv, 1, 1, fp);
      		fread (&txt, 1, 1, fp);
      		fclose (fp);
  	} else {
      		cRv = 54;
      		cGv = 74;
      		cBv = 255;
      		txt = 255;
  	}
  }
 
  

  draw_background_locked (gCurrentIcon);
  draw_progress_locked ();
  gr_color (0, 0, 0, 175);	// background color
  gr_fill (0, 0, gr_fb_width (), gr_fb_height ());	//fill the background with this color
  int i = 0;
  int j = 0;
  int row = 0;

   if (show_menu)
	  {
	    gr_color (cRv, cGv, cBv, 255);
	    gr_fill (0,
		      (menu_top + menu_sel - menu_show_start) * CHAR_HEIGHT,
		      gr_fb_width (),
		      (menu_top + menu_sel - menu_show_start +
		       1) * CHAR_HEIGHT + 1);
	     TEXTCOLOR  for (i = 0; i < menu_top; ++i)
		    {
		      draw_text_line (i, menu[i]);
		      row++;
		    }
	     
	    //draw line
	    gr_color (cRv, cGv, cBv, 255);
	    row--;		//go up one to draw our top line
	    gr_fill (0, row * CHAR_HEIGHT + CHAR_HEIGHT / 2 - 1,
		     gr_fb_width (),
		     row * CHAR_HEIGHT + CHAR_HEIGHT / 2 + 1);
	    row++;
	     if (menu_items - menu_show_start + menu_top >= MAX_ROWS)
	      j = MAX_ROWS - menu_top;
	    
	    else
	      j = menu_items - menu_show_start;
	     for (i = menu_show_start + menu_top;
		    i < (menu_show_start + menu_top + j); ++i)
		    {
		      if (i == menu_top + menu_sel)
			      {
				gr_color (txt, txt, txt, 255);
				draw_text_line (i - menu_show_start,
						 menu[i]);
				gr_color (cRv, cGv, cBv, 255);
			      }
		      else
			      {
				gr_color (cRv, cGv, cBv, 255);
				draw_text_line (i - menu_show_start,
						 menu[i]);
			      }
		      row++;
		    }
	    
	      //bottom line
	      gr_fill (0, row * CHAR_HEIGHT + CHAR_HEIGHT / 2 - 1,
		       gr_fb_width (),
		       row * CHAR_HEIGHT + CHAR_HEIGHT / 2 + 1);
	    row++;
	  }
  TEXTCOLOR			// bottom text
    for (; row < text_rows; ++row)
	  {
	    draw_text_line (row, text[(row + text_top) % text_rows]);
	   }
}

 
// Redraw everything on the screen and flip the screen (make it visible).
// Should only be called with gUpdateMutex locked.
  static void
update_screen_locked (void) 
{
  draw_screen_locked ();
  gr_flip ();
}  

// Updates only the progress bar, if possible, otherwise redraws the screen.
// Should only be called with gUpdateMutex locked.
  static void
update_progress_locked (void) 
{
  if (!gPagesIdentical)
	  {
	    draw_screen_locked ();	// Must redraw the whole screen
	    gPagesIdentical = 1;
	  }
  else
	  {
	    draw_progress_locked ();	// Draw only the progress bar
	  }
  gr_flip ();
}

 
// Keeps the progress bar updated, even when the process is otherwise busy.
static void *
progress_thread (void *cookie) 
{
  for (;;)
	  {
	    usleep (1000000 / PROGRESSBAR_INDETERMINATE_FPS);
	    pthread_mutex_lock (&gUpdateMutex);
	     
	      // update the progress bar animation, if active
	      // skip this if we have a text overlay (too expensive to update)
	      if (gProgressBarType == PROGRESSBAR_TYPE_INDETERMINATE)
		    {
		      update_progress_locked ();
		    }
	     
	      // move the progress bar forward on timed intervals, if configured
	    int duration = gProgressScopeDuration;

	    if (gProgressBarType == PROGRESSBAR_TYPE_NORMAL && duration > 0)
		    {
		      int elapsed = time (NULL) - gProgressScopeTime;
		      float progress = 1.0 * elapsed / duration;

		      if (progress > 1.0)
			progress = 1.0;
		      if (progress > gProgress)
			      {
				gProgress = progress;
				update_progress_locked ();
			      }
		    }
	     pthread_mutex_unlock (&gUpdateMutex);
	  }
  return NULL;
}

 
// Reads input events, handles special hot keys, and adds to the key queue.
static void *
input_thread (void *cookie) 
{
  int rel_sum = 0;
  int fake_key = 0;
  int last_code = 0;
  unsigned keyheld;

  for (;;)
	  {
	    
	      // wait for the next key event
	    struct input_event ev;

	    
	    do
		    {
		    if (ev_get(&ev, 0, keyheld) != 1) {
			// Check for an up/down key press
			if (ev.type == EV_KEY && (ev.code == 
				KEY_VOLUMEUP
				|| ev.code == KEY_VOLUMEDOWN ) && ev.value == 1) {
					keyheld = 1;
					last_code = ev.code;
				} else { 
					keyheld =0;
				}
			} else {
				// A return value of 1 means the last key should be repeated
				ev.type = EV_KEY;
				ev.code = last_code;
				ev.value = 1;
			}


		       if (ev.type == EV_SYN)
			      {
				continue;
			      }
		      else if (ev.type == EV_REL)
			      {
				if (ev.code == REL_Y)
					{
					  
					    // accumulate the up or down motion reported by
					    // the trackball.  When it exceeds a threshold
					    // (positive or negative), fake an up/down
					    // key event.
					    rel_sum += ev.value;
					  if (rel_sum > 3)
						  {
						    fake_key = 1;
						    ev.type = EV_KEY;
						    ev.code = KEY_VOLUMEDOWN;
						    ev.value = 1;
						    rel_sum = 0;
						  }
					  else if (rel_sum < -3)
						  {
						    fake_key = 1;
						    ev.type = EV_KEY;
						    ev.code = KEY_VOLUMEUP;
						    ev.value = 1;
						    rel_sum = 0;
						  }
					}
			      } else if (ev.type == EV_ABS && (ev.code == KEY_VOLUMEUP || ev.code == KEY_VOLUMEDOWN)) {
			        fake_key = 1;
				ev.type = EV_KEY;
			      } else {
				rel_sum = 0;
			      }
		    }
	    while (ev.type != EV_KEY || ev.code > KEY_MAX);
	     pthread_mutex_lock (&key_queue_mutex);
	    if (!fake_key)
		    {
		      
			// our "fake" keys only report a key-down event (no
			// key-up), so don't record them in the key_pressed
			// table.
			key_pressed[ev.code] = ev.value;
		    }
	    fake_key = 0;
	    const int queue_max = sizeof (key_queue) / sizeof (key_queue[0]);

	    if (ev.value > 0 && key_queue_len < queue_max)
		    {
		      key_queue[key_queue_len++] = ev.code;
		      pthread_cond_signal (&key_queue_cond);
		    }
	    pthread_mutex_unlock (&key_queue_mutex);
	  }
  return NULL;
}

 void
ui_init (void) 
{
  gr_init ();
  ev_init ();
   text_col = text_row = 0;
  text_rows = gr_fb_height () / CHAR_HEIGHT;
  if (text_rows > MAX_ROWS)
    text_rows = MAX_ROWS;
  text_top = 1;
   text_cols = gr_fb_width () / CHAR_WIDTH;
  if (text_cols > MAX_COLS - 1)
    text_cols = MAX_COLS - 1;
   int i;

  for (i = 0; BITMAPS[i].name != NULL; ++i)
	  {
	    int result =
	      res_create_surface (BITMAPS[i].name, BITMAPS[i].surface);
	    if (result < 0)
		    {
		      if (result == -2)
			      {
				LOGI ("Bitmap %s missing header\n",
				       BITMAPS[i].name);
			      }
		      else
			      {
				LOGE ("Missing bitmap %s\n(Code %d)\n",
				       BITMAPS[i].name, result);
			      }
		      *BITMAPS[i].surface = NULL;
		    }
	  }
   pthread_t t;
  pthread_create (&t, NULL, progress_thread, NULL);
  pthread_create (&t, NULL, input_thread, NULL);
}

 void
ui_set_background (int icon) 
{
  pthread_mutex_lock (&gUpdateMutex);
  gCurrentIcon = gBackgroundIcon[icon];
  update_screen_locked ();
  pthread_mutex_unlock (&gUpdateMutex);
}  void

ui_show_indeterminate_progress () 
{
  pthread_mutex_lock (&gUpdateMutex);
  if (gProgressBarType != PROGRESSBAR_TYPE_INDETERMINATE)
	  {
	    gProgressBarType = PROGRESSBAR_TYPE_INDETERMINATE;
	    update_progress_locked ();
	  }
  pthread_mutex_unlock (&gUpdateMutex);
}

 void
ui_show_progress (float portion, int seconds) 
{
  pthread_mutex_lock (&gUpdateMutex);
  gProgressBarType = PROGRESSBAR_TYPE_NORMAL;
  gProgressScopeStart += gProgressScopeSize;
  gProgressScopeSize = portion;
  gProgressScopeTime = time (NULL);
  gProgressScopeDuration = seconds;
  gProgress = 0;
  update_progress_locked ();
  pthread_mutex_unlock (&gUpdateMutex);
}  void

ui_set_progress (float fraction) 
{
  pthread_mutex_lock (&gUpdateMutex);
  if (fraction < 0.0)
    fraction = 0.0;
  if (fraction > 1.0)
    fraction = 1.0;
  if (gProgressBarType == PROGRESSBAR_TYPE_NORMAL && fraction > gProgress)
	  {
	    
	      // Skip updates that aren't visibly different.
	    int width = gr_get_width (gProgressBarIndeterminate[0]);
	    float scale = width * gProgressScopeSize;

	    if ((int) (gProgress * scale) != (int) (fraction * scale))
		    {
		      gProgress = fraction;
		      update_progress_locked ();
		    }
	  }
  pthread_mutex_unlock (&gUpdateMutex);
}  void

ui_reset_progress () 
{
  pthread_mutex_lock (&gUpdateMutex);
  gProgressBarType = PROGRESSBAR_TYPE_NONE;
  gProgressScopeStart = gProgressScopeSize = 0;
  gProgressScopeTime = gProgressScopeDuration = 0;
  gProgress = 0;
  update_screen_locked ();
  pthread_mutex_unlock (&gUpdateMutex);
} 

void
ui_print (const char *fmt, ...) 
{
  char buf[256];

  va_list ap;
  va_start (ap, fmt);
  vsnprintf (buf, 256, fmt, ap);
  va_end (ap);
  if (ui_log_stdout) fputs (buf, stdout);
   
    // This can get called before ui_init(), so be careful.
    pthread_mutex_lock (&gUpdateMutex);
  if (text_rows > 0 && text_cols > 0)
	  {
	    char *ptr;

	    for (ptr = buf; *ptr != '\0'; ++ptr)
		    {
		      if (*ptr == '\n' || text_col >= text_cols)
			      {
				text[text_row][text_col] = '\0';
				text_col = 0;
				text_row = (text_row + 1) % text_rows;
				if (text_row == text_top)
				  text_top = (text_top + 1) % text_rows;
			      }
		      if (*ptr != '\n')
			text[text_row][text_col++] = *ptr;
		    }
	    text[text_row][text_col] = '\0';
	    update_screen_locked ();
	  }
  pthread_mutex_unlock (&gUpdateMutex);
}

void ui_reset_text_col()
{
  pthread_mutex_lock(&gUpdateMutex);
  text_col = 0;
  pthread_mutex_unlock(&gUpdateMutex);
}

void view_log()
{
  int lines =0;
  char line[512];
  FILE* f;

  f = fopen("/tmp/recovery.log", "r");
  if (f == NULL)
  {
    ui_print("File does not exist!\n");
    return;
  }
  
  //avoid doubling the log with prints of itself...
  ui_log_stdout = 0;
  ui_print("\n\n\n\n\n\n"); // print a few newlines to separate old log from new log
  while (fgets(line, 512, f) != NULL && fgets(line, 512, f) != EOF) 
  { 
    
    start:
    ui_print("%s", line);
    lines++;
    if (lines == 35 ) // if 35 lines have been displayed, pause to let the user read
    {
      lines = 0;
      int key = ui_wait_key();
      int action = device_handle_key(key);
      if (action == ITEM_BACK) return;
      if (action == HIGHLIGHT_DOWN ) goto start;
    }
  }

  lines = 0;
  ui_log_stdout = 1;
}

void ui_key_test()
{
  device_recovery_start();
  ui_init();
  ui_print("-- RZrecovery Key Test Utility --\n\n");
  ui_print("To exit, flash a full recovery with\n");
  ui_print("fastboot / flash_image.\n");
  ui_print("\n\nPress any key...\n\n");
  for (;;)
  {
    int key = ui_wait_key();
    ui_print("Key: %i\n", key);
  }
}
  
int 
ui_start_menu (char **headers, char **items, int sel, int menu_only)
{
  int i;

  pthread_mutex_lock (&gUpdateMutex);
  if (text_rows > 0 && text_cols > 0)
	  {
	    for (i = 0; i < text_rows; ++i)
		    {
		      if (headers[i] == NULL)
			break;
		      strncpy (menu[i], headers[i], text_cols - 1);
		      menu[i][text_cols - 1] = '\0';
		    }
	    menu_top = i;
	    for (; i < MENU_MAX_ROWS; ++i)
		    {
		      if (items[i - menu_top] == NULL)
			break;
		      strncpy (menu[i], items[i - menu_top], text_cols - 1);
		      menu[i][text_cols - 1] = '\0';
		    }

	    if (menu_only) goto finish; // skip the part about adding the back button

	    if (virtualBack_toggled())
	    {
	      strcpy(menu[i], "<< Back <<");
	      ++i;
	    }

	
	    finish:
	    menu_items = i - menu_top;
	    show_menu = 1;
	     menu_show_start = 0;
	    menu_sel = sel;
	     update_screen_locked ();
	  }

  pthread_mutex_unlock (&gUpdateMutex);
  
  if (menu_only) goto end; // skip this too
  if (virtualBack_toggled()) 
  { 
    return menu_items - 1;
  }
  
  end:
  return menu_items;
}

  int
ui_menu_select (int sel)
{
  int old_sel;

  pthread_mutex_lock (&gUpdateMutex);
  if (show_menu > 0)
	  {
	    old_sel = menu_sel;
	    menu_sel = sel;
	     if (menu_sel < 0)
		    {
		      menu_sel = menu_items - 1;
		      menu_show_start = menu_items - (text_rows - menu_top);
		      if (menu_show_start < 0)
			      {
				menu_show_start = 0;
			      }
		    }
	    else if (menu_sel >= menu_items)
		    {
		      menu_sel = 0;
		      menu_show_start = 0;
		    }
	      if (menu_sel < menu_show_start && menu_show_start > 0)
		    {
		      menu_show_start--;
		    }
	     if (menu_sel - menu_show_start + menu_top >= text_rows)
		    {
		      menu_show_start++;
		    }
	     sel = menu_sel;
	     if (menu_sel != old_sel)
	      update_screen_locked ();
	  }
  pthread_mutex_unlock (&gUpdateMutex);
  return sel;
}

 void
ui_end_menu ()
{
  int i;

  pthread_mutex_lock (&gUpdateMutex);
  if (show_menu > 0 && text_rows > 0 && text_cols > 0)
	  {
	    show_menu = 0;
	    update_screen_locked ();
	  }
  pthread_mutex_unlock (&gUpdateMutex);
}

 int
ui_wait_key () 
{
  pthread_mutex_lock (&key_queue_mutex);
  while (key_queue_len == 0)
	  {
	    pthread_cond_wait (&key_queue_cond, &key_queue_mutex);
	  }
   int key = key_queue[0];

  memcpy (&key_queue[0], &key_queue[1], sizeof (int) * --key_queue_len);
  pthread_mutex_unlock (&key_queue_mutex);
  return key;
}

 int
ui_key_pressed (int key) 
{
  
    // This is a volatile static array, don't bother locking
    return key_pressed[key];
}

 void
ui_clear_key_queue ()
{
  pthread_mutex_lock (&key_queue_mutex);
  key_queue_len = 0;
  pthread_mutex_unlock (&key_queue_mutex);
} 
