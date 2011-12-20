#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#include "recovery.h"
#include "roots.h"
#include "recovery_ui.h"

#ifndef BOARD_UMS_LUNFILE
#define BOARD_UMS_LUNFILE	"/sys/devices/platform/usb_mass_storage/lun0/file"
#endif

int is_path_mountable(char* path)
{
  /*
  this function will return:
  -1 if volume does not exist/not mountable,
  2 if it is not mtd, is vfat or ext
  1 if it is not mtd, is vfat
  0 if it is not mtd
  If just looking for path mountable, check for ! -1
  for USB on windows, check for 1
  for USB on linux, check for 2
  */
  Volume* v = volume_for_path(path);
  if (v == NULL) 
  {
	return -1; 
  } 
  //vfat only
  if (strcmp(v->fs_type, "vfat") == 0 && strcmp(v->fs_type, "ext2") != 0 && strcmp(v->fs_type, "ext3") != 0 &&strcmp(v->fs_type, "ext4") != 0) 
  {
	return 1;
  }	  
  //vfat or ext
  else if (strcmp(v->fs_type, "vfat") == 0 || strcmp(v->fs_type, "ext2") == 0 || strcmp(v->fs_type, "ext3") == 0 || strcmp(v->fs_type, "ext4") == 0) 
  {
	return 2;
  }
  //mtd
  if (strcmp(v->fs_type, "mtd") == 0) 
  {
	return -1;
  }
  else
  //not mtd
  {
    return 0;
  }
  return 0;
}

static int
is_usb_storage_enabled ()
{
  if (access(BOARD_UMS_LUNFILE,F_OK) != -1) 
  {
    printf("LUN file present.\n");
    FILE *fp = fopen (BOARD_UMS_LUNFILE, "r");
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
  else
  {
    printf("USB Mass storage will not work.\n");
    return (3);
  }
}

void show_usb_menu()
{
  char *mode = calloc(5, sizeof(char));
  if (access("/tmp/.rzrpref_usb", F_OK) != -1)
  {
    FILE *fp = fopen("/tmp/.rzrpref_usb", "r");	
	fgets(mode, 4, fp);
  }
  else
  {
    mode = "fat";
  }
  printf("USB Mode: %s\n", mode);
  
  int num_volumes = get_num_volumes();
  Volume* device_volumes;
  device_volumes = get_device_volumes();
  
  int i;
  char** usb_volumes = malloc (num_volumes * sizeof (char *));
  
  int mountable_volumes = 0;
  for (i=0; i<num_volumes; i++)
  {
    usb_volumes[i] = "";
    Volume *v = &device_volumes[i];
	if (is_path_mountable(v->mount_point) != -1)
	{	  
	  if (strcmp(mode, "fat") == 0)
	  {
	    if (is_path_mountable(v->mount_point) == 1)
		{
		  printf("%s is USB-mountable.\n", v->mount_point);
	      usb_volumes[mountable_volumes] = malloc(sizeof(char*));
	      printf("usb_volumes[%d]: %s\n", mountable_volumes, v->mount_point);
	      sprintf(usb_volumes[mountable_volumes], "%s", v->mount_point);
	      mountable_volumes++;
		}
	  }
	  else
	  {
		if (is_path_mountable(v->mount_point) == 1 || is_path_mountable(v->mount_point) ==2)
		{
		  printf("%s is USB-mountable.\n", v->mount_point);
	      usb_volumes[mountable_volumes] = malloc(sizeof(char*));
	      printf("usb_volumes[%d]: %s\n", mountable_volumes, v->mount_point);
	      sprintf(usb_volumes[mountable_volumes], "%s", v->mount_point);
	      mountable_volumes++;	
		}
	  }
	}  
  }

  char* headers[] = { "Choose a USB storage device",
	"",
    NULL
  };
  
  char** items = malloc (mountable_volumes * sizeof (char *));
  int z;
  for (z=0; z<mountable_volumes; z++)
  {
    items[z] = usb_volumes[z];
	printf("items[%d]: %s\n", z, items[z]);
	items[z+1] = NULL;
  }
  
	  	
  int chosen_item = -1;
	
  while (chosen_item != ITEM_BACK)
  {	
    chosen_item = get_menu_selection (headers, items, 0, chosen_item < 0 ? 0 : chosen_item);
	char* selection = usb_volumes[chosen_item];
    switch(chosen_item)
	{
	  case ITEM_BACK:
	    return;
	  default:	     
	    show_usb_storage_enabled_screen(selection);
	    return;
    }
	return;
  }	
} 

void show_usb_storage_enabled_screen(char* selection)
{
  enable_usb_mass_storage(selection);
  char *headers[] = { "",
    "is in USB mass storage mode",
	"Go back to disable",
	"",
	NULL
  };
  
  char* items[] = { "Disable", NULL };
  
#define ITEM_DISABLE 0

  headers[0] = selection;
  
  
  int chosen_item = -1;
  
  while (chosen_item != ITEM_BACK)
  {
    chosen_item = get_menu_selection (headers, items, 0, chosen_item < 0 ? 0 : chosen_item);
    switch (chosen_item) 
	{
	  case ITEM_DISABLE:
	    disable_usb_mass_storage();
		return;	   
    }
  disable_usb_mass_storage();
  return;
  }
}  
  
void
enable_usb_mass_storage (char* volume)
{
  int fd;
  Volume *vol = volume_for_path (volume);
   
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

void
disable_usb_mass_storage ()
{
  printf("Disabling USB Mass storage...\n");
  if (access(BOARD_UMS_LUNFILE, F_OK) != -1)
  {
    FILE *fp = fopen (BOARD_UMS_LUNFILE, "w");
    fprintf (fp, "\n");
    fclose (fp);
	printf("Disabled!\n");
  }
}

void
get_mount_menu_options (char **items, int ms, int md, int mc, int msd, int mb, int me)
{
 
  int num_volumes = get_device_volumes();
  char **items1 = malloc (num_volumes * sizeof (char *));
  char **items2 = malloc (num_volumes * sizeof (char *));
  
  int usb_mountable = is_usb_storage_enabled();
  int system_mountable = is_path_mountable("/system");
  int data_mountable = is_path_mountable("/data");
  int cache_mountable = is_path_mountable("/cache");
  int sd_mountable = is_path_mountable("/sdcard");
  int boot_mountable = is_path_mountable("/boot");
  int emmc_mountable = is_path_mountable("/emmc");
    
  int i = 0;
  if (usb_mountable != 3) items[i] = "USB Mass Storage"; i++;
  if (system_mountable != -1) items1[i] = "Mount /system", i++;
  if (data_mountable != -1) items1[i] = "Mount /data"; i++;
  if (cache_mountable != -1) items1[i] = "Mount /cache"; i++;
  if (sd_mountable != -1) items1[i] = "Mount /sdcard"; i++;
  if (boot_mountable != -1) items1[i] = "Mount /boot"; i++;
  if (emmc_mountable != -1) items1[i] = "Mount /emmc"; i++;
  items1[i] = NULL;
  
  int j = 0;
  if (usb_mountable != 3) items[j] = "USB Mass Storage"; j++;
  if (system_mountable != -1) items2[j] = "Unmount /system"; j++;
  if (data_mountable != -1) items2[j] = "Unmount /data"; j++;
  if (cache_mountable != -1) items2[j] = "Unmount /cache"; j++;
  if (sd_mountable != -1) items2[j] = "Unmount /sdcard"; j++;
  if (boot_mountable != -1) items2[j] = "Unmount /boot"; j++;
  if (emmc_mountable != -1) items2[j] = "Unmount /emmc"; j++;
  items2[j] = NULL;
  
  int k = 0;
  if (usb_mountable != 3) items[k] = "USB Mass Storage"; k++;
  if (system_mountable != -1) items[k] = ms ? items2[k] : items1[k]; k++;
  if (data_mountable != -1) items[k] = md ? items2[k] : items1[k]; k++;
  if (cache_mountable != -1) items[k] = mc ? items2[k] : items1[k]; k++;
  if (sd_mountable != -1) items[k] = msd ? items2[k] : items1[k]; k++;
  if (boot_mountable != -1) items[k] = mb ? items2[k] : items1[k]; k++;
  if (emmc_mountable != -1) items[k] = me ? items2[k] : items1[k]; k++;
  items[k] = NULL;
}

void
show_mount_menu ()
{
  static char *headers[] = { "Choose a mount or unmount option",
    "",
    NULL
  };

  int ms = is_path_mounted ("/system");
  int md = is_path_mounted ("/data");
  int mc = is_path_mounted ("/cache");
  int msd = is_path_mounted ("/sdcard");
  int mb = is_path_mounted ("/boot");
  int me = is_path_mounted ("/emmc");

  int num_volumes = get_device_volumes();
  char **items = malloc (num_volumes * sizeof (char *));

  get_mount_menu_options (items, ms, md, mc, msd, mb, me);

#define ITEM_USB 0
#define ITEM_S   1
#define ITEM_D   2
#define ITEM_C	 3
#define ITEM_SD  4
#define ITEM_B   5
#define ITEM_E   6

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
		        if (!ensure_path_unmounted ("/system")) ms ^= 1;
		      } else {
		        if (!ensure_path_mounted ("/system")) ms ^= 1;
		      }
		      break;
		    case ITEM_D:
		      if (md) {
		        if (!ensure_path_unmounted ("/data")) md ^= 1;
		      } else {
		        if (!ensure_path_mounted ("/data")) md ^= 1;
		      }
		      
		      break;
		    case ITEM_C:
		      if (mc) {
		        if (!ensure_path_unmounted ("/cache")) mc ^= 1;
		      } else {
		        if (!ensure_path_mounted ("/cache")) mc ^= 1;
		      }
		      
		      break;
		    case ITEM_SD:
		      if (msd) {
		        if (!ensure_path_unmounted ("/sdcard")) msd ^= 1;
		      } else { 
		        if (!ensure_path_mounted ("/sdcard")) msd ^= 1;
		      }
		      break;
		    case ITEM_USB:
		      show_usb_menu();
		      break;
		    case ITEM_B:
		      if (mb) { 
		        if (!ensure_path_unmounted("/boot")) mb ^= 1;
		      } else { 
		        if (!ensure_path_mounted("/boot")) mb ^= 1;
		      }
		      break;
		    case ITEM_E:
		      if (me) {
		        if (!ensure_path_unmounted("/emmc")) me ^= 1;
		      } else { 
		        if (!ensure_path_mounted("/emmc")) me ^= 1;
		      }
		      break;
		    }
	    get_mount_menu_options (items, ms, md, mc, msd, mb, me);
	  }
}
