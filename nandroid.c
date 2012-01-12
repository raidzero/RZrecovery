#include <stdio.h>
#include <errno.h>
#include <linux/input.h>
#include <sys/wait.h>
#include <sys/limits.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/vfs.h>
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
#include "dirsize.h"

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
    //timestamp format 20111228-1444
    strftime(fmt, sizeof fmt, "%Y%m%d-%H%M", tm);
    snprintf(timestamp, sizeof timestamp, fmt, tv.tv_usec);
  }
	
  printf("NANDROID_DIR: %s\n", NANDROID_DIR);
  printf("ANDROID_VERSION: %s\n", ANDROID_VERSION);
  printf("PARTITIONS: %s\n", PARTITIONS);
  printf("TIMESTAMP: %s\n", timestamp);
  
  //build the prefix string
  char prefix[1024];
  sprintf(prefix, "%s/%s-%s-%s", NANDROID_DIR, ANDROID_VERSION, PARTITIONS, timestamp);
  
  strcpy(PREFIX, prefix);
  printf("internal prefix: %s\n", PREFIX); 
}
  
  
int backup_partition(const char* partition, const char* PREFIX, int compress, int progress)
{ 
  Volume *v = volume_for_path(partition);
  char* NANDROID_DIR = get_nandroid_dir();
  char TAR_OPTS[5]="c";
  if (progress) 
  {
	strcat(TAR_OPTS, "v");
  }
  char* EXTENSION = NULL;
  if (compress) {
    strcat(TAR_OPTS, "z");
	EXTENSION = "tar.gz";
  }
  if (!compress) 
  {
    EXTENSION = "tar";
  }
  strcat(TAR_OPTS, "f");
  int status;
  int valid = 0;  
  long totalfiles = 0;
  char totalfiles_string[7] = "";
  
  
  
  ui_print("Backing up %s... ", partition);
  char* partition_path;
  
	if (strcmp(partition, ".android_secure") == 0)
	{
	  partition_path = calloc(strlen(get_storage_root()) + 1, sizeof(char));	  
	  sprintf(partition_path, "%s/.android_secure", get_storage_root());
	  valid = 1;
	}
	else partition_path = partition;
	
	printf("partition_path: %s\n", partition_path);
  
  if (valid == 1 || (strcmp(v->fs_type, "mtd") != 0 && strcmp(v->fs_type, "emmc") != 0 && strcmp(v->fs_type, "bml")))
  {
    printf("not mtd, emmc, or bml!\n");
    ensure_path_mounted(partition);
    char* partition_name;
    
	 if (strcmp(partition, ".android_secure") != 0)	 
	 {	 
	   char* strptr = strstr(partition, "/") + 1; 
	   if (strptr)
	   {
	     partition_name = calloc(strlen(strptr) + 1, sizeof(char));
	     strcpy(partition_name, strptr);    
	   }	 
	   printf("partition_name: %s\n", partition_name);
	 }
	 else partition_name="secure";
		
	if (progress) 
	{
	  ui_show_indeterminate_progress();
	  printf("Progress bar enabled. Computing number of items in %s...\n", partition_path);
	  totalfiles = compute_files(partition_path) + 1;
	  printf("totalfiles: %ld\n", totalfiles);
	  sprintf(totalfiles_string, "%ld", totalfiles);
	  set_clearFilesTotal_intent(1);
	}
	
	int status = 0;
	char* EXCLUSION;	
	if (strcmp(partition,"/data") ==0) EXCLUSION="--exclude ./media";
	else EXCLUSION = "";
	
	char tar_cmd[1024];
	sprintf(tar_cmd, "cd %s && tar %s %s/%s.%s %s .", partition_path, TAR_OPTS, PREFIX, partition_name, EXTENSION, EXCLUSION);
	printf("tar_cmd: %s\n", tar_cmd);
	
	//use popen to capture output from system call and act on it
	FILE* in=popen(tar_cmd, "r");
	
	char temp[PATH_MAX];
	long counter = 0;
	if (progress) ui_show_progress(1.0, 0);	
	while (fgets(temp, PATH_MAX-1, in) != NULL)
	{	    
	  if (progress)
	  { 
	    printf("COUNTER: %ld / %ld\n", counter, totalfiles);	      
	    ui_set_progress((float)counter / (float)totalfiles);
	    counter++;
	  }	
	}
	if (progress) ui_reset_progress();
	
	if (in == NULL)
	{
	  ui_print("Failed!\n");
	  ensure_path_unmounted(partition_path);
	  status = -1;
	} 
	else
	{
	  ui_print("Success!");
	  ui_reset_text_col();
	  ensure_path_unmounted(partition_path);
	  status = 0;
	}
  }
  else //must be mtd, bml, or emmc - dump raw
  {
   if (progress) ui_show_indeterminate_progress();
   printf("Must be raw...\n");
   char rawimg[PATH_MAX];
	strcpy(rawimg, PREFIX);
	strcat(rawimg, partition);
	strcat(rawimg, ".img");
	
	printf("backing up %s to %s\n", partition, rawimg);

   if (backup_raw_partition(v->fs_type, v->device, rawimg))
	{
	  ui_print("Failed!\n");
	  ensure_path_unmounted(partition_path);
	  status = -1;
	}
	else
	{
	  ui_print("Success!");
	  ui_reset_text_col();
	  ensure_path_unmounted(partition_path);
	  status = 0;
	}
  }
  if (progress) ui_reset_progress();
  return status;
}  

int restore_partition(const char* partition, const char* PREFIX, int progress)
{
  char* STORAGE_ROOT = get_storage_root();
  Volume *v = volume_for_path(partition);
  char* PREFIX2 = calloc(strlen(PREFIX) + 1, sizeof(char));
  strcpy(PREFIX2, PREFIX);
  printf("PREFIX: %s\nPREFIX2: %s\n", PREFIX, PREFIX2);
  
  
  char TAR_OPTS[5]="x";
  if (progress) strcat(TAR_OPTS, "v");
  char* EXTENSION = NULL;
  int compress;
  char tarfilename[PATH_MAX];
  char tgzfilename[PATH_MAX];
  if (strstr(partition, ".android_secure"))
  {
    sprintf(tarfilename, "%s/secure.tar", PREFIX, partition);
    sprintf(tgzfilename, "%s/secure.tar.gz", PREFIX, partition);
  }
  else
  {
    sprintf(tarfilename, "%s%s.tar", PREFIX, partition);
    sprintf(tgzfilename, "%s%s.tar.gz", PREFIX, partition);
  }
  
  if (access(tgzfilename, F_OK) != -1 && access(tarfilename, F_OK) == -1) compress = 1;
  if (access(tarfilename, F_OK) != -1 && access(tgzfilename, F_OK) == -1) compress = 0;
  if (compress) {
    strcat(TAR_OPTS, "z");
	EXTENSION = "tar.gz";
  }
  else
  {
    EXTENSION = "tar";
  }
  strcat(TAR_OPTS, "f");
  int status;
  int valid = 0;
  long totalfiles = 0;
  char totalfiles_string[PATH_MAX] = { NULL };
  
  ui_print("Restoring %s... ", partition);
  
  const char* partition_path = NULL;
  const char* partition_name = NULL;
  
	if (strcmp(partition, ".android_secure") == 0)
	{
	  partition_path = calloc(strlen(get_storage_root()) + 1, sizeof(char));	  
	  sprintf(partition_path, "%s/.android_secure", get_storage_root());
	  valid = 1;
	}
	else partition_path = partition;
	printf("partition_path: %s\n", partition_path);
  
  if (valid == 1 || (strcmp(v->fs_type, "mtd") != 0 && strcmp(v->fs_type, "emmc") != 0 && strcmp(v->fs_type, "bml")))
  {
    printf("not mtd, emmc, or bml!\n");
    ensure_path_mounted(partition);
	 //wipe first!    
    erase_volume(partition);
    
	 if (strcmp(partition, ".android_secure") != 0)	 
	 {	 
	   char* strptr = strstr(partition, "/") + 1; 
	   if (strptr)
	   {
	     partition_name = calloc(strlen(strptr) + 1, sizeof(char));
	     strcpy(partition_name, strptr);    
	   }	 
	   printf("partition_name: %s\n", partition_name);
	 }
	 else partition_name="secure";
		
	if (progress) 
	{  
	  printf("Progress bar enabled. Computing number of items in %s.%s...\n", partition_name, EXTENSION);
	  if (progress) ui_show_indeterminate_progress();
	  char TAR_TVF[1024] = { NULL };
	  char *TVF_OPTS = NULL;
	  if (compress) TVF_OPTS = "tvzf";
	  else TVF_OPTS="tvf";
	  
	  sprintf(TAR_TVF, "tar %s %s/%s.%s | wc -l", TVF_OPTS, PREFIX, partition_name, EXTENSION); 
	  printf("partition_name: %s\n", partition_name);	  
	  printf("TAR_TVF: %s\n", TAR_TVF);	  
     char temp[PATH_MAX] = { NULL };
     
     FILE* in=popen(TAR_TVF, "r"); //use popen to execute the command
     fgets(totalfiles_string, PATH_MAX-1, in); // fgets to grab the output
     printf("totalfiles: %s\n", totalfiles_string);
     totalfiles = atoi(totalfiles_string);	  
     TVF_OPTS=NULL;
     set_clearFilesTotal_intent(1);
	}
	
	printf("TAR_OPTS: %s\nPREFIX: %s\npartition_name: %s\nEXTENSION: %s\npartition_path: %s\n", TAR_OPTS, PREFIX2, partition_name, EXTENSION, partition_path);

	
	const char tar_cmd[1024] = { NULL };
	sprintf(tar_cmd, "tar %s %s/%s.%s -C %s", TAR_OPTS, PREFIX2, partition_name, EXTENSION, partition_path); 
	printf("tar_cmd: %s\n", tar_cmd);

	//use popen to capture output from system call and act on it
	FILE* in=popen(tar_cmd, "r");
	char temp[PATH_MAX];
	long counter = 0;
	if (progress) ui_show_progress(1.0, 0);
	while (fgets(temp, PATH_MAX-1, in) != NULL)
	{	    
	  if (progress)
	  { 
	    printf("COUNTER: %ld / %ld\n", counter, totalfiles);	      
	    ui_set_progress((float)counter / (float)totalfiles);
	    counter++;
	  }	
	}
	if (progress) ui_reset_progress();   
	
	if (in == NULL)
	{
	  ui_print("Failed!\n");
	  ensure_path_unmounted(partition_path);
	  status = -1;
	} 
	else
	{
	  ui_print("Success!");
	  ui_reset_text_col();
	  ensure_path_unmounted(partition_path);
	  status = 0;
	}
  }
  else //must be mtd, bml, or emmc - restore raw
  {
    printf("must be raw...\n");
   if (progress) ui_show_indeterminate_progress(); 
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
	  ensure_path_unmounted(partition_path);
	  status = -1;
	}
	else
	{
	  ui_print("Success!");
	  ui_reset_text_col();
	  ensure_path_unmounted(partition_path);
	  status = 0;
	}
  }
  if (progress) ui_reset_progress();
  return status;
} 

void nandroid_native(const char* operation, char* subname, char partitions, int show_progress, int compress)
{
  time_t starttime, endtime, elapsed;
  char* STORAGE_ROOT = get_storage_root();
  printf("STORAGE_ROOT: %s\n", STORAGE_ROOT);
  
  int ENERGY;
  
  FILE *fs = fopen ("/sys/class/power_supply/battery/status", "r");
  char *bstat = calloc (14, sizeof (char));
  fgets (bstat, 14, fs);
  
  if (strcmp(bstat, "Charging") == 0) ENERGY = 100;
 
  FILE *fc = fopen ("/sys/class/power_supply/battery/capacity", "r");
  char *bcap = calloc (14, sizeof (char));
  fgets (bcap, 4, fc);
  
  ENERGY = atoi(bcap);
  
  if (ENERGY < 20)
  {
    if (!ask_question("Insufficient battery power. Continue?")) return;
  }
  fclose(fc);
  fclose(fs);
  

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
	ui_print("Calculating space requirements...");
	if (show_progress) ui_show_indeterminate_progress();
	ui_reset_text_col();
	struct statfs s;
	//sd space
	Volume * storage_volume = volume_for_path(PREFIX);
	statfs(storage_volume->mount_point, &s);
	uint64_t sd_bsize = s.f_bsize;
	uint64_t sd_freeblocks = s.f_bavail;
	long available_mb = sd_bsize * sd_freeblocks / (long) (1024*1024);
	
	long bytesrequired = 0;
	long totalbytes = 0;
	ensure_path_mounted(STORAGE_ROOT);
	
	if (system) 
	{
	  ui_print("/system...");
	  ensure_path_mounted("/system");
	  bytesrequired = compute_size("/system", 0);
	  totalbytes += bytesrequired;
	  ui_reset_text_col();
	}
	if (asecure) 
	{
	  ui_print("android_secure...");
	  char SECURE_PATH[1024];
	  sprintf(SECURE_PATH, "%s/.android_secure", get_storage_root());
	  ensure_path_mounted(STORAGE_ROOT);
	  printf("SECURE_PATH: %s\n", SECURE_PATH);
	  bytesrequired += compute_size(SECURE_PATH, 0);
	  totalbytes += bytesrequired;
	  ui_reset_text_col();
	}
	if (sdext) 
	{
	  ui_print("/sdext...");
	  ensure_path_mounted("/sd-ext");
	  bytesrequired = compute_size("/sd-ext", 0);
	  totalbytes += bytesrequired;
	  ui_reset_text_col();
	}	
	if (data)
	{	
	  ui_print("/data...");
	  ensure_path_mounted("/data");
	  bytesrequired = compute_size("/data", 0);  
	  totalbytes += bytesrequired;
	  ui_reset_text_col();
	  if (volume_present("/datadata")) 
	  {
	    ui_print("/datadata...");
	    ensure_path_mounted("/datadata");
		bytesrequired = compute_size("/datadata", 0);
		totalbytes += bytesrequired;
		ui_reset_text_col();
	  }
	  //subtract data/media
	  ui_print("/data/media...");
	  bytesrequired -= compute_size("/data/media", 0);
	  ui_reset_text_col();
	}
	if (cache) 
	{
	  ui_print("/cache...");
	  ensure_path_mounted("/cache");
	  bytesrequired = compute_size("/cache", 0);
	  totalbytes += bytesrequired;
	  ui_reset_text_col();
	}
		
	long mb_required =  bytesrequired / 1024 / 1024;
	if (show_progress) ui_reset_progress();
	ui_print("~%ld MB required\n", mb_required);
	ui_print("%ld MB available\n", available_mb);
	  
    char tmp[PATH_MAX];
    sprintf(tmp, "mkdir -p %s", PREFIX);
    __system(tmp);
	
	if (compress)
	{
	  ui_print("\nCompression activated. Go make a sandwich.\n");
	  ui_print("This will take forever, but your\n");
	  ui_print("SD Card will thank you :)\n\n");
	}
	
    if (boot) 
	{
	  if (backup_partition("/boot", PREFIX, compress, show_progress)) failed = 1;
	}
	if (system) 
	{
	  if (backup_partition("/system", PREFIX, compress, show_progress)) failed = 1;
	}
    if (data) 
	{
	  if (backup_partition("/data", PREFIX, compress, show_progress)) failed = 1;
	  if (volume_present("/datadata"))
	  {
	    if (backup_partition("/datadata", PREFIX, compress, show_progress)) failed = 1;
	  }
	}
    if (cache) 
	{
	  if (backup_partition("/cache", PREFIX, compress, show_progress)) failed = 1;
	}
    if (asecure) 
    {
	  printf("About to back up .android_secure...\n");
	  if (backup_partition(".android_secure", PREFIX, compress, show_progress)) failed = 1;
    }

    if (sdext) 
	{
	  if (backup_partition("/sd-ext", PREFIX, compress, show_progress)) failed = 1;
	}
  }
  if (strcmp(operation, "restore") == 0)
  {
    printf("SUBNAME: %s\n", subname);
    if (strcmp(subname, "") == 0) 
    {
      ui_print("No backup selected. Exiting.\n");
      return;
    }
    char PREFIX[PATH_MAX];
	char* NANDROID_DIR = get_nandroid_dir();
	starttime = time(NULL);
	sprintf(PREFIX, "%s/%s", NANDROID_DIR, subname);
	printf("PREFIX: %s\n", PREFIX);
	printf("START: %ld\n", starttime);
	
    if (boot) 
	{
	  if (restore_partition("/boot", PREFIX, show_progress)) failed = 1;
	}    
	if (system) 
	{
	  if (restore_partition("/system", PREFIX, show_progress)) failed = 1;
	}
    if (data) 
	{
	  if (restore_partition("/data", PREFIX, show_progress)) failed = 1;
	  if (volume_present("/datadata"))
	  {
	    if (restore_partition("/datadata", PREFIX, show_progress)) failed = 1;
	  }
	}
    if (cache) 
	{
	  if (restore_partition("/cache", PREFIX, show_progress)) failed = 1;
	}
    if (asecure) 
    {
	  printf("About to restore .android_secure...\n");
	  if (restore_partition(".android_secure", PREFIX, show_progress)) failed = 1;
    }
    if (sdext) 
	{
	  if (restore_partition("/sd-ext", PREFIX, show_progress)) failed = 1;
	}
  }
  printf("%s finished.\n", operation);
  endtime = time(NULL);
  printf("END: %ld\n", endtime);
  elapsed = endtime - starttime;
  printf("ELAPSED: %ld\n", elapsed);
  
  
  if (failed != 1) if (reboot) reboot_android();
  
  //this part isnt quite working yet - gets 0 byte filesizes in PREFIX?
  if (strcmp(operation, "backup") == 0) 
  {
    ensure_path_mounted(STORAGE_ROOT);
	printf("PREFIX: %s\n", PREFIX);
	set_clearTotal_intent(1); //clear the previous running total so it doesnt get added to the backup total
    ui_print("Space used: %ld MB\n", compute_size(PREFIX, 1)/1024/1024);
  }
  ui_print("Elapsed time: %ld seconds\n", elapsed);
}
