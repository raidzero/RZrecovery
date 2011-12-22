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

int num_volumes;

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
  //tmp should not be included, its always mounted
  if (strcmp(v->mount_point,"/tmp") ==0)
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
	      usb_volumes[mountable_volumes] = malloc(strlen(v->mount_point)+2);
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

char** get_mount_menu_options()
{
  Volume * device_volumes = get_device_volumes();
  num_volumes = get_num_volumes();
  
  char** volumes = malloc (num_volumes * sizeof (char *));

  int mountable_volumes = 0;
  int usb_storage_enabled = is_usb_storage_enabled();
  int i = 0;
 
  if (usb_storage_enabled != 3)
  {
	volumes[mountable_volumes] = "USB Mass Storage";
	printf("volumes[%i]: %s\n", mountable_volumes, volumes[mountable_volumes]);
	mountable_volumes++;
	i = 1;
  }  
 
  for (i; i<num_volumes; i++)
  {
    volumes[i] = "";
    Volume *v = &device_volumes[i];
	char* operation;
	if (is_path_mountable(v->mount_point) != -1)
	{	  
	  if (is_path_mounted(v->mount_point)) operation = "Unmount";
	  else operation = "Mount";
	  printf("volumes[%i]: %s %s\n", mountable_volumes, operation, v->mount_point);
	  volumes[mountable_volumes] = malloc(strlen(operation)+strlen(v->mount_point)+2);
	  sprintf(volumes[mountable_volumes], "%s %s", operation, v->mount_point);
	  mountable_volumes++;
	}
  }
  
  char **main_items = malloc (num_volumes * sizeof (char *));
  
  int z = 0;
  for (z; z<mountable_volumes; z++)
  {
	main_items[z] = malloc(strlen(volumes[z])+2);
	main_items[z] = volumes[z];
  }
  main_items[z] = NULL;
 
  return main_items;
}

void
show_mount_menu ()
{
  num_volumes = get_num_volumes(); 
   
  char** main_items = get_mount_menu_options();
  
  static char *headers[] = { "Choose a mount or unmount option",
    "",
    NULL
  };  
 
  int chosen_item = -1;
  char* selected;
  
  while (chosen_item != ITEM_BACK)
	  {
	    chosen_item =
	      get_menu_selection (headers, main_items, 0,
				  chosen_item < 0 ? 0 : chosen_item);
		
		switch (chosen_item)
		{
		
		  case ITEM_BACK:
		    return;
			
		  default:
		    //printf("selected: %s\n", main_items[chosen_item]);
		  
		    if (strcmp(main_items[chosen_item], "USB Mass Storage") ==0)
		    {
		      show_usb_menu();
		    }
		    else
		    {
		      char* partition=main_items[chosen_item];
		      char* strptr = strstr(partition, " ") + 1; //get a pointer to the string beginning 1 position after the space
		      char* result = calloc(strlen(strptr) + 1, sizeof(char)); // allocate some mem for the new string
		      strcpy(result, strptr); //copy the pointer's contents into the new string
		      printf("\npartition: %s\n", result);
		  
		      int mp = is_path_mounted(result);
		      printf("%s is mounted: %i\n", result, mp);
		      char* prefix;
		      main_items[chosen_item] = NULL;
		      if (mp)
		      {
		        printf("Unmounting %s...\n", result);
		        ensure_path_unmounted(result);
		      }
		      else
		      {
		        printf("Mounting %s...\n", result);
		        ensure_path_mounted(result);
		      }
		    }
	    }
	    main_items = get_mount_menu_options();
	  }
	return;
}
