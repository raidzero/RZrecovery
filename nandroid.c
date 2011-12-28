#include <stdio.h>
#include <linux/input.h>
#include <sys/wait.h>
#include <sys/limits.h>
#include <dirent.h>
#include <fcntl.h>

#include "common.h"
#include "recovery.h"
#include "recovery_ui.h"
#include "minui/minui.h"
#include "roots.h"
#include "strings.h"
#include "nandroid_menu.h"
#include "nandroid.h"
#include "install_menu.h"

int reboot_afterwards;
char timestamp[64];
char PREFIX[PATH_MAX];

char* get_android_version()
{
  char* result;
  ensure_path_mounted("/system");
  FILE * vers = fopen("/system/build.prop", "r");
  if (vers == NULL) 
  {
	return NULL;
  }

  char line[512];
  while(fgets(line, sizeof(line), vers) != NULL && fgets(line, sizeof(line), vers) != EOF) //read a line
  {
	if (strstr(line, "ro.build.display.id") != NULL)
	{
	  char* strptr = strstr(line, "=") + 1; 
	  result = calloc(strlen(strptr) + 1, sizeof(char));
	  strcpy(result, strptr);  
	  break; //leave the loop, we found what we're after
	}
  }
  //replace spaces with _
  int count;
  for (count=0; count<strlen(result); count++)
  {
	if (result[count] == ' ') result[count] = '_';  
	if (result[count] == '(' || result[count] == ')') result[count] = '.';
	if (result[count] == '\n') result[count] = NULL; //dont want a newline...
  }
  fclose(vers); 
  ensure_path_unmounted("/system");
  return result;
} 

void get_prefix(char partitions)
{
  const char* NANDROID_DIR = get_nandroid_dir();
  const char* ANDROID_VERSION = get_android_version();
  const char PARTITIONS[6] = "";
  
  //identify directory with what partitions are going to be backed up:
  int boot = partitions & BOOT;
  int data = partitions & DATA;
  int cache = partitions & CACHE;
  int system = partitions & SYSTEM;
  int asecure = partitions & ASECURE;
  int sdext = partitions & SDEXT;
  
  if (boot) strcat(PARTITIONS, "B");
  if (data) strcat(PARTITIONS, "D");
  if (cache) strcat(PARTITIONS, "C");
  if (system) strcat(PARTITIONS, "S");
  if (asecure) strcat(PARTITIONS, "A");
  if (sdext) strcat(PARTITIONS, "E");
  
  //timestamp generation
  char            fmt[64], timestamp[64];
  struct timeval  tv;
  struct tm       *tm;

  gettimeofday(&tv, NULL);
  if((tm = localtime(&tv.tv_sec)) != NULL)
  {
    strftime(fmt, sizeof fmt, "%Y%m%d-%H%M", tm);
    snprintf(timestamp, sizeof timestamp, fmt, tv.tv_usec);
  }
	
  printf("NANDROID_DIR: %s\n", NANDROID_DIR);
  printf("ANDROID_VERSION: %s\n", ANDROID_VERSION);
  printf("PARTITIONS: %s\n", PARTITIONS);
  printf("TIMESTAMP: %s\n", timestamp);
  
  //build the prefix string
  char prefix[1024];
  strcpy(prefix, NANDROID_DIR);
  strcat(prefix, "/");
  strcat(prefix, ANDROID_VERSION);
  strcat(prefix, "-");
  strcat(prefix, PARTITIONS);
  strcat(prefix, "-");
  strcat(prefix, timestamp);
  
  strcpy(PREFIX, prefix);
  printf("internal prefix: %s\n", PREFIX); 
}
  
  
int backup_partition(const char* partition, const char* PREFIX)
{
  Volume *v = volume_for_path(partition);
  char* NANDROID_DIR = get_nandroid_dir();
  char* TAR_OPTS="cvf";
  int status;
  
  ui_print("Backing up %s... ", partition);
  
  if (strstr(partition, ".android_secure"))
  {
    char* STORAGE_ROOT = get_storage_root();
	char tar_cmd[1024];
	sprintf(tar_cmd, "cd %s/.android_secure && tar %s %s/secure.tar .", STORAGE_ROOT, TAR_OPTS, PREFIX, STORAGE_ROOT);
	printf("tar_cmd: %s\n", tar_cmd);
	if (__system(tar_cmd))
	{
	  ui_print("Failed!\n");
	  status = -1;
	} 
	else
	{
	  ui_print("Success!\n");
	  status = 0;
	}
	return status;
  }	
	 
  if (strcmp(v->fs_type, "mtd") != 0 && strcmp(v->fs_type, "emmc") != 0 && strcmp(v->fs_type, "bml"))
  {
    ensure_path_mounted(partition);
    char tar_cmd[1024];
	if (strcmp(partition, "/data") == 0)
	{
	  sprintf(tar_cmd, "cd %s && tar %s %s%s.tar --exclude 'media' .", partition, TAR_OPTS, PREFIX, partition, partition);
	}
	else
	{
	  sprintf(tar_cmd, "cd %s && tar %s %s%s.tar .", partition, TAR_OPTS, PREFIX, partition, partition);
	}
	printf("tar_cmd: %s\n", tar_cmd);
	if (__system(tar_cmd))
	{
	  ui_print("Failed!\n");
	  ensure_path_unmounted(partition);
	  status = -1;
	} 
	else
	{
	  ui_print("Success!\n");
	  ensure_path_unmounted(partition);
	  status = 0;
	}
  }
  else //must be mtd, bml, or emmc - dump raw
  {
    printf("Must be raw...\n");
    char rawimg[PATH_MAX];
	strcpy(rawimg, PREFIX);
	strcat(rawimg, partition);
	strcat(rawimg, ".img");
	
	printf("backing up %s to %s\n", partition, rawimg);
	//sprintf(rawimg, "%s/%s.img", PREFIX, partition);
    if (backup_raw_partition(v->fs_type, v->device, rawimg))
	{
	  ui_print("Failed!\n");
	  ensure_path_unmounted(partition);
	  status = -1;
	}
	else
	{
	  ui_print("Success!\n");
	  ensure_path_unmounted(partition);
	  status = 0;
	}
  }
  return status;
}  

int restore_partition(const char* partition, const char* PREFIX)
{
  char* STORAGE_ROOT = get_storage_root();
  Volume *v = volume_for_path(partition);
  
  char* TAR_OPTS="xvf";
  int status;
  
  ui_print("Restoring %s... ", partition);
  
  if (strstr(partition, ".android_secure"))
  {
	char rm_cmd[PATH_MAX];
	sprintf(rm_cmd, "rm -rf %s/.android_secure/*", STORAGE_ROOT);
	__system(rm_cmd);
	char tar_cmd[PATH_MAX];	
	sprintf(tar_cmd, "tar %s %s/secure.tar -C %s/.android_secure", TAR_OPTS, PREFIX, STORAGE_ROOT);
	printf("tar_cmd: %s\n", tar_cmd);
	if (__system(tar_cmd))
	{
	  ui_print("Failed!\n");
	  status = -1;
	} 
	else
	{
	  ui_print("Success!\n");
	  status = 0;
	}
	ui_reset_progress();
	return status;
  }
  
  if (strcmp(v->fs_type, "mtd") != 0 && strcmp(v->fs_type, "emmc") != 0 && strcmp(v->fs_type, "bml"))
  { 
    format_volume(partition);
	ensure_path_mounted(partition);
    char tar_cmd[1024];
	sprintf(tar_cmd, "tar %s %s%s.tar -C %s", TAR_OPTS, PREFIX, partition, partition);
	printf("tar_cmd: %s\n", tar_cmd);
	if (__system(tar_cmd))
	{
	  ui_print("Failed!\n");
	  ensure_path_unmounted(partition);
	  status = -1;
	} 
	else
	{
	  ui_print("Success!\n");
	  ensure_path_unmounted(partition);
	  status = 0;
	}
  }
  else //must be mtd, bml, or emmc - restore raw
  {
    printf("must be raw...\n");
    
	//must pull the / off the partition name for flash_img...
	char* strptr = strstr(partition, "/") +  1;
	char* result = calloc(strlen(strptr) + 1, sizeof(char));
	strcpy(result, strptr);  
	

	char rawimg[PATH_MAX];
	char flash_cmd[PATH_MAX];
	strcpy(rawimg, PREFIX);
	strcat(rawimg, partition);
	strcat(rawimg, ".img");
	printf("restoring %s to %s...\n", rawimg, partition);
	sprintf(flash_cmd, "flash_img %s %s", result, rawimg);
	printf("flash_cmd: %s\n", flash_cmd);
    if (!__system(flash_cmd))
	{
	  ui_print("Failed!\n");
	  ensure_path_unmounted(partition);
	  status = -1;
	}
	else
	{
	  ui_print("Success!\n");
	  ensure_path_unmounted(partition);
	  status = 0;
	}
  }
  ui_reset_progress();
  return status;
} 

void nandroid_native(const char* operation, char* subname, char partitions, int show_progress, int compress)
{
  time_t starttime, endtime, elapsed;
  char* STORAGE_ROOT = get_storage_root();
  printf("STORAGE_ROOT: %s\n", STORAGE_ROOT);
  
  ui_print ("\nStarting Nandroid %s.\n", operation);
  
  int reboot = get_reboot_after();
  int failed = 0;

  int boot = partitions & BOOT;
  int data = partitions & DATA;
  int cache = partitions & CACHE;
  int system = partitions & SYSTEM;
  int asecure = partitions & ASECURE;
  int sdext = partitions & SDEXT;
  
  if (strcmp(operation, "backup") == 0)
  {
    starttime = time(NULL);
	printf("START: %ld\n", starttime);
    get_prefix(partitions);
	ui_print("%s\n", PREFIX);
    char tmp[PATH_MAX];
    sprintf(tmp, "mkdir -p %s", PREFIX);
    __system(tmp);
	
    if (boot) 
	{
	  if (backup_partition("/boot", PREFIX)) failed = 1;
	}
	if (system) 
	{
	  if (backup_partition("/system", PREFIX)) failed = 1;
	}
    if (data) 
	{
	  if (backup_partition("/data", PREFIX)) failed = 1;
	}
    if (cache) 
	{
	  if (backup_partition("/cache", PREFIX)) failed = 1;
	}
    if (asecure) 
    {
	  printf("About to back up .android_secure...\n");
	  if (backup_partition(".android_secure", PREFIX)) failed = 1;
    }

    if (sdext) 
	{
	  if (backup_partition("/sd-ext", PREFIX)) failed = 1;
	}
  }
  if (strcmp(operation, "restore") == 0)
  {
    char PREFIX[PATH_MAX];
	char* NANDROID_DIR = get_nandroid_dir();
	starttime = time(NULL);
	strcpy(PREFIX, NANDROID_DIR);
	strcat(PREFIX, "/");
	strcat(PREFIX, subname);
	printf("backup path: %s\n", PREFIX);
	printf("START: %ld\n", starttime);
	
    if (boot) 
	{
	  if (restore_partition("/boot", PREFIX)) failed = 1;
	}    
	if (system) 
	{
	  if (restore_partition("/system", PREFIX)) failed = 1;
	}
    if (data) 
	{
	  if (restore_partition("/data", PREFIX)) failed = 1;
	}
    if (cache) 
	{
	  if (restore_partition("/cache", PREFIX)) failed = 1;
	}
    if (asecure) 
    {
	  printf("About to restore .android_secure...\n");
	  if (restore_partition(".android_secure", PREFIX)) failed = 1;
    }
    if (sdext) 
	{
	  if (restore_partition("/sd-ext", PREFIX)) failed = 1;
	}
  }
  printf("%s finished.\n", operation);
  endtime = time(NULL);
  printf("END: %ld\n", endtime);
  elapsed = endtime - starttime;

  printf("ELAPSED: %ld\n", elapsed);
  if (failed != 1) if (reboot) reboot_android();
  ui_print("%s took %ld seconds.\n", operation, elapsed);
}
