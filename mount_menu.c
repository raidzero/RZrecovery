#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#include "recovery.h"
#include "roots.h"
#include "recovery_ui.h"

static char *
get_mounted_partitions_string (char *destination, int md, int msd, int ms)
{
  static const char *mounted_prefix = "Mounted partitions: ";
  static const char *mounted_data = "/data ";
  static const char *mounted_sdcard = "/sdcard ";
  static const char *mounted_system = "/system";
  static const char *newline = "\n";

  strcpy (destination, mounted_prefix);
  if (md)
	  {
	    strcat (destination, mounted_data);
	  }
  if (msd)
	  {
	    strcat (destination, mounted_sdcard);
	  }
  if (ms)
	  {
	    strcat (destination, mounted_system);
	  }
  strcat (destination, newline);

  return (destination);
}

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
get_mount_menu_options (char **items, int md, int msd, int ms, int usb)
{
  static char *items1[] = { "Mount /system",
    "Mount /data",
    "Mount /sdcard",
    "Enable USB Mass Storage",
    NULL
  };
  static char *items2[] = { "Umount /system",
    "Umount /data",
    "Umount /sdcard",
    "Disable USB Mass Storage",
    NULL
  };

  items[0] = ms ? items2[0] : items1[0];
  items[1] = md ? items2[1] : items1[1];
  items[2] = msd ? items2[2] : items1[2];
  items[3] = usb ? items2[3] : items1[3];
  items[4] = NULL;
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
	    LOGE ("Unable to open ums lunfile (%s)", strerror (errno));
	    return -1;
	  }

  if ((write (fd, vol->device, strlen (vol->device)) < 0) &&
      (!vol->device2 || (write (fd, vol->device, strlen (vol->device2)) < 0)))
	  {
	    LOGE ("Unable to write to ums lunfile (%s)", strerror (errno));
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
    "",
    NULL
  };

  // "Mounted partitions: /data /sdcard /system\n"
  //  123456789012345678901234567890123456789012345678
  //  0        1         2         3        
  char *mounted = malloc (50 * sizeof (char));

  int md = is_path_mounted ("/data");
  int msd = is_path_mounted ("/sdcard");
  int ms = is_path_mounted ("/system");
  int usb = is_usb_storage_enabled ();

  char **items = malloc (6 * sizeof (char *));

  get_mount_menu_options (items, md, msd, ms, usb);

  headers[1] = get_mounted_partitions_string (mounted, md, msd, ms);

#define ITEM_S   0
#define ITEM_D   1
#define ITEM_SD  2
#define ITEM_USB 3

  int chosen_item = -1;

  while (chosen_item != ITEM_BACK)
	  {
	    chosen_item =
	      get_menu_selection (headers, items, 0,
				  chosen_item < 0 ? 0 : chosen_item);

	    /*      char* act_str = malloc(18*sizeof(char));
	       sprintf(act_str, "Action is %d\n", chosen_item);
	       ui_print(act_str); */

	    switch (chosen_item)
		    {
		    case ITEM_S:
		      if (ms)
			      {
				ensure_path_unmounted ("/system");
				ui_print ("%s", "Unm");
			      }
		      else
			      {
				ensure_path_mounted ("/system");
				ui_print ("M");
			      }
		      ui_print ("ounted /system\n");
		      ms ^= 1;
		      break;
		    case ITEM_D:
		      if (md)
			      {
				ensure_path_unmounted ("/data");
				ui_print ("Unm");
			      }
		      else
			      {
				ensure_path_mounted ("/data");
				ui_print ("M");
			      }
		      ui_print ("ounted /data\n");
		      md ^= 1;
		      break;
		    case ITEM_SD:
		      if (msd)
			      {
				ensure_path_unmounted ("/sdcard");
				ui_print ("Unm");
			      }
		      else
			      {
				disable_usb_mass_storage ();
				usb = 0;
				ensure_path_mounted ("/sdcard");
				ui_print ("M");
			      }
		      ui_print ("ounted /sdcard\n");
		      msd ^= 1;
		      break;
		    case ITEM_USB:
		      if (usb)
			      {
				disable_usb_mass_storage ();
			      }
		      else
			      {
				enable_usb_mass_storage ();
				msd = 0;
			      }
		      usb ^= 1;
		      break;
		    }
	    get_mounted_partitions_string (mounted, md, msd, ms);
	    get_mount_menu_options (items, md, msd, ms, usb);
	  }
}
