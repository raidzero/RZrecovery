#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#include "recovery.h"
#include "roots.h"
#include "recovery_ui.h"

static int
is_usb_storage_enabled ()
{
  FILE *fp = fopen ("/sys/devices/platform/usb_mass_storage/lun0/file", "r");
  char *buf = malloc (11 * sizeof (char));

  fgets (buf, 11, fp);
  fclose (fp);
  if (strcmp (buf, "/dev/block") == 0)
	  {
	    return (1);
	  }
  else
	  {
	    return (0);
	  }
}

static void
get_mount_menu_options (char **items, int usb, int ms, int md, int msd, int mb, int me)
{
  static char *items1[7];
  static char *items2[7];
  
  items1[0] = "Enable USB Mass Storage";
  items1[1] = "Mount /system";
  items1[2] = "Mount /data";
  items1[3] = "Mount /sdcard";
  items1[4] = "Mount /boot";
  if (me != NULL) {
  	items1[5] = "Mount /emmc";
  	items1[6] = NULL;
  } else {
        items1[5] = NULL;
  }

  items2[0] = "Disable USB Mass Storage";
  items2[1] = "Unmount /system";
  items2[2] = "Unmount /data";
  items2[3] = "Unmount /sdcard";
  items2[4] = "Unmount /boot";
  if (me != NULL ) { 
  	items2[5] = "Unmount /emmc";
  	items2[6] = NULL;
  } else {
  	items2[5] = NULL;
  }

  items[0] = usb ? items2[0] : items1[0];
  items[1] = ms ? items2[1] : items1[1];
  items[2] = md ? items2[2] : items1[2];
  items[3] = msd ? items2[3] : items1[3];
  items[4] = mb ? items2[4] : items1[4];
  if (me != NULL ) {
  	items[5] = me ? items2[5] : items1[5];
  	items[6] = NULL;
  } else {
  	items[5] = NULL;
  }	
}

#ifndef BOARD_UMS_LUNFILE
#define BOARD_UMS_LUNFILE	"/sys/devices/platform/usb_mass_storage/lun0/file"
#endif

static void
enable_usb_mass_storage ()
{
  int fd;
  Volume *vol = volume_for_path ("/sdcard");

  if ((fd = open (BOARD_UMS_LUNFILE, O_WRONLY)) < 0)
	  {
	    LOGE ("\nUnable to open ums lunfile (%s)", strerror (errno));
	    return -1;
	  }

  if ((write (fd, vol->device, strlen (vol->device)) < 0) &&
      (!vol->device2 || (write (fd, vol->device, strlen (vol->device2)) < 0)))
	  {
	    LOGE ("\nUnable to write to ums lunfile (%s)", strerror (errno));
	    close (fd);
	    return -1;
	  }
}

static void
disable_usb_mass_storage ()
{
  FILE *fp = fopen ("/sys/devices/platform/usb_mass_storage/lun0/file", "w");

  fprintf (fp, "\n");
  fclose (fp);
}

void
show_mount_menu ()
{
  static char *headers[] = { "Choose a mount or unmount option",
    "",
    NULL
  };


  int usb = is_usb_storage_enabled ();
  int ms = is_path_mounted ("/system");
  int md = is_path_mounted ("/data");
  int msd = is_path_mounted ("/sdcard");
  int mb = is_path_mounted ("/boot");
  int me = is_path_mounted ("/emmc");

  char **items = malloc (7 * sizeof (char *));

  get_mount_menu_options (items, usb, ms, md, msd, mb, me);

#define ITEM_USB 0
#define ITEM_S   1
#define ITEM_D   2
#define ITEM_SD  3
#define ITEM_B   4
#define ITEM_E   5

  int chosen_item = -1;

  while (chosen_item != ITEM_BACK)
	  {
	    chosen_item =
	      get_menu_selection (headers, items, 0,
				  chosen_item < 0 ? 0 : chosen_item);

	    switch (chosen_item)
		    {
		    case ITEM_S:
		      if (ms) { 
		        ensure_path_unmounted ("/system");
		      } else {
		        ensure_path_mounted ("/system");
		      }
		      ms ^= 1;
		      break;
		    case ITEM_D:
		      if (md) {
		        ensure_path_unmounted ("/data");
		      } else {
		        ensure_path_mounted ("/data");
		      }
		      md ^= 1;
		      break;
		    case ITEM_SD:
		      if (msd) {
		        ensure_path_unmounted ("/sdcard");
		      } else { 
		        disable_usb_mass_storage ();
		        usb = 0;
		        ensure_path_mounted ("/sdcard");
		      }
		      msd ^= 1;
		      break;
		    case ITEM_USB:
		      if (usb) {
		        disable_usb_mass_storage ();
		      } else { 
		        enable_usb_mass_storage ();
		        msd = 0;
		      }
		      usb ^= 1;
		      break;
		    case ITEM_B:
		      if (mb) { 
		        ensure_path_unmounted("/boot");
		      } else { 
		        ensure_path_mounted("/boot");
		      }
		      mb ^= 1;
		      break;
		    case ITEM_E:
		      if (me) {
		        ensure_path_unmounted("/emmc");
		      } else { 
		        ensure_path_mounted("/emmc");
		      }
		      me ^= 1;
		      break;
		    }
	    get_mount_menu_options (items, usb, ms, md, msd, mb, me);
	  }
}
