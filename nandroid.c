#include <stdio.h>
#include <ctype.h> //needed for isalpha, etc
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
  char* ANDROID_VERSION;
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
  fclose(vers); 
  ensure_path_unmounted("/system");
  
  //strip only the android version from it
  printf("RAW VERSION: %s\n", result);
  int LENGTH = strlen(result);
  printf("LENGTH: %d\n", LENGTH);  
  int i, k, found;
  for (i=0; i<LENGTH; i++)
  {
	 k = i;	
	 //Android versions follow this scheme: AAADD, A=A-Z, D=0-9, ICS has an extra digit at the end
	 if (isalpha(result[k]) && isalpha(result[k++]) && isalpha(result[k++]) && isdigit(result[k++]) && isdigit(result[k++]))
	 {
		printf("version number found at positions %d through %d:\n", i, k);
		found = 1;		
		break;
	 }
  }	
  if (found)
  {
    ANDROID_VERSION = calloc(strlen(result) + 1, sizeof(char)); //allocate the length of result plus 1 for null-terminator
	 int n = 0;	     
    for(i-=1; i<=k; i++)
    {		     
	   ANDROID_VERSION[n] = result[i];
		n++;  
    }	
    ANDROID_VERSION[n] = '\0'; //dont forget the almighty null-terminator!
  }
  else 
  {
    return NULL;
  }
  printf("Memory Address: %d\n", &ANDROID_VERSION);
  printf("ANDROID VERSION: %s\n", ANDROID_VERSION);
  return ANDROID_VERSION;
} 

void get_prefix(char partitions)
{
  const char* NANDROID_DIR = get_nandroid_dir();
  char* ANDROID_VERSION = get_android_version();
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
    //timestamp format 2011-12-28_1444
    printf("about to make the timestamp...\n");
    strftime(fmt, sizeof fmt, "%Y-%m-%d_%H%M", tm);
    snprintf(timestamp, sizeof timestamp, fmt, tv.tv_usec);
  }
	
  printf("NANDROID_DIR: %s\n", NANDROID_DIR);
  printf("Memory Address: %d\n", &ANDROID_VERSION);
  printf("ANDROID_VERSION: %s\n", ANDROID_VERSION);
  printf("PARTITIONS: %s\n", PARTITIONS);
  printf("TIMESTAMP: %s\n", timestamp);
 
  
  if (ANDROID_VERSION)
  {  
    int vers_length = strlen(ANDROID_VERSION);
    int i;
    for (i=0; i<vers_length; i++)
    {
      if (ANDROID_VERSION[i] == '\n') ANDROID_VERSION[i] = NULL; // no newline please
    }
    if (ANDROID_VERSION)
    {
      strcat(PARTITIONS,"-");
    }
  } 
  else ANDROID_VERSION = "";
  
  //build the prefix string
  char prefix[1024];
  sprintf(prefix, "%s/%s-%s%s", NANDROID_DIR, timestamp, PARTITIONS, ANDROID_VERSION);
  
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
	  partition_path = calloc(strlen(get_storage_root()) + strlen(".android_secure") + 1, sizeof(char));	  
	  sprintf(partition_path, "%s/.android_secure", get_storage_root());
	  valid = 1;
	}
	else partition_path = partition;
	
	printf("partition_path: %s\n", partition_path);
  
  if (valid == 1 || (strcmp(v->fs_type, "mtd") != 0 && strcmp(v->fs_type, "emmc") != 0 && strcmp(v->fs_type, "bml")))
  {
    printf("not mtd, emmc, or bml!\n");
    if (ensure_path_mounted(partition) && strcmp(partition, ".android_secure") != 0)
    {
      printf("Error mounting %s!\n", partition);
      return -1;
    }
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
	else ui_show_indeterminate_progress();	
	while (fgets(temp, PATH_MAX-1, in) != NULL)
	{	    
	  if (progress)
	  { 
	    printf("COUNTER: %ld / %ld\n", counter, totalfiles);	      
	    ui_set_progress((float)counter / (float)totalfiles);
	    counter++;
	  }	
	}
	ui_reset_progress();
   int retstatus = pclose(in);	
	
	if (retstatus != 0)
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
   ui_show_indeterminate_progress();
   printf("Must be raw...\n");
   char rawimg[PATH_MAX];
	strcpy(rawimg, PREFIX);
	strcat(rawimg, partition);
	strcat(rawimg, ".img");
	
	printf("backing up %s to %s\n", partition, rawimg);
	
	char dump_cmd[512] = { NULL };
	sprintf(dump_cmd, "dump_img %s %s", partition, rawimg);
	printf("dump_cmd: %s\n", dump_cmd);

   if (!__system(dump_cmd))
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
  ui_reset_progress();
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
  char* EXTENSION = calloc(strlen(".tar.gz") + 1, sizeof(char));
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
  
  ui_print("Restoring %s... ", partition);
  
  const char* restore_path = NULL;
  const char* restore_path2 = NULL;
  const char* partition_name = NULL;
  
  restore_path = calloc(strlen(get_storage_root()) + strlen(".android_secure") + 1, sizeof(char));
  restore_path2 = calloc(strlen(get_storage_root()) + strlen(".android_secure") + 1, sizeof(char)); 
  
	if (strcmp(partition, ".android_secure") == 0)
	{	  
	  sprintf(restore_path, "%s/.android_secure", get_storage_root());
	  valid = 1;
	}
	else restore_path = partition;
	
	strcpy(restore_path2, restore_path); 
	
	printf("restore_path: %s\n", restore_path);
	
  
  if (valid == 1 || (strcmp(v->fs_type, "mtd") != 0 && strcmp(v->fs_type, "emmc") != 0 && strcmp(v->fs_type, "bml")))
  {
    printf("not mtd, emmc, or bml!\n");
	 if (strcmp(partition, ".android_secure") != 0) 
	 {	       
      printf("Mounting %s...\n", partition);
		ensure_path_mounted(partition);
    }
     
	 if (strcmp(partition, ".android_secure") != 0)	 
	 {	 
	   ensure_path_mounted(get_storage_root());
	   char* strptr = strstr(partition, "/") + 1; 
	   if (strptr)
	   {
	     partition_name = calloc(strlen(strptr) + 1, sizeof(char));
	     strcpy(partition_name, strptr);    
	   }	 
	   printf("partition_name: %s\n", partition_name);
	 }
	 else partition_name="secure";
	 
	 //check to be sure the filename is there, so it doesnt just error out	
	 char* FILENAME = calloc(strlen(PREFIX2) + strlen(partition_name) + strlen(EXTENSION) + 1, sizeof(char));
	 sprintf(FILENAME, "%s/%s.%s", PREFIX2, partition_name, EXTENSION);
	 if (access(FILENAME, F_OK) == -1)
	 {
	   printf("%s does not exist. skipping.\n", FILENAME);
	   return 0;
 	 }
		 
    if (strcmp(partition, ".android_secure") != 0 && !is_path_mounted(partition))
	 {  
	   printf("Error mounting %s!\n", partition);
		printf("Mount status: %d\n", is_path_mounted(partition));	     
	   ui_print("Failed!\n");
	   return -1;
	 }
	   	 
	 if (strcmp(partition, ".android_secure") != 0)	
	 {	
	   //wipe first! 
	   ensure_path_unmounted(partition);   
		printf("Wiping %s...\n", partition);      
      erase_volume(partition);	 
	   ensure_path_mounted(partition);
	 }	
	 
   long totalfiles = 0;
	if (progress) 
	{  
	  printf("Progress bar enabled. Computing number of items in %s.%s...\n", partition_name, EXTENSION);
	  ui_show_indeterminate_progress();	  	  
     totalfiles = tarsize(PREFIX, partition_name, EXTENSION, compress);
     printf("totalfiles received from tarsize(): %ld\n", totalfiles);  
	  ui_reset_progress();	  
     set_clearFilesTotal_intent(1);
	}
	
	printf("TAR_OPTS: %s\nPREFIX: %s\npartition_name: %s\nEXTENSION: %s\nrestore_path: %s\n", TAR_OPTS, PREFIX2, partition_name, EXTENSION, restore_path2);

	
	const char tar_cmd[1024] = { NULL };
	
	sprintf(tar_cmd, "tar %s %s/%s.%s -C %s", TAR_OPTS, PREFIX2, partition_name, EXTENSION, restore_path2); 
	printf("tar_cmd: %s\n", tar_cmd);

	//use popen to capture output from system call and act on it
	FILE* in=popen(tar_cmd, "r");
	char temp[PATH_MAX];
	long counter = 0;
	if (progress) ui_show_progress(1.0, 0);
	else ui_show_indeterminate_progress();
	while (fgets(temp, PATH_MAX-1, in) != NULL)
	{	    
	  if (progress)
	  { 
	    printf("COUNTER: %ld / %ld\n", counter, totalfiles);	      
	    ui_set_progress((float)counter / (float)totalfiles);
	    counter++;
	  }	
	}
	ui_reset_progress();   
	int retstatus = pclose(in); 
   printf("TAR return status: %d\n", retstatus);
	
	if (retstatus != 0)
	{
	  ui_print("Failed!\n");
	  if (strcmp(partition, ".android_secure") != 0) ensure_path_unmounted(restore_path);
	  status = -1;
	} 
	else
	{  
	  ui_print("Success!");
	  ui_reset_text_col();
	  if (strcmp(partition, ".android_secure") != 0) ensure_path_unmounted(restore_path);
	  status = 0;
	}
  }
  else //must be mtd, bml, or emmc - restore raw
  {
    printf("must be raw...\n");
   ui_show_indeterminate_progress(); 
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
	  ensure_path_unmounted(restore_path);
	  status = -1;
	}
	else
	{
	  ui_print("Success!");
	  ui_reset_text_col();
	  ensure_path_unmounted(restore_path);
	  status = 0;
	}
  }
  printf("restore_path: %s\n", restore_path);
  ui_reset_progress();
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
	ui_reset_progress();
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
  ui_reset_text_col();
  ui_print("Finished.\n");
  printf("%s finished.\n", operation);
  endtime = time(NULL);
  printf("END: %ld\n", endtime);
  elapsed = endtime - starttime;
  printf("ELAPSED: %ld\n", elapsed);
  
  
  if (failed != 1) if (reboot) reboot_android();
  
  if (strcmp(operation, "backup") == 0) 
  {
    ensure_path_mounted(STORAGE_ROOT);
	printf("PREFIX: %s\n", PREFIX);
	set_clearTotal_intent(1); //clear the previous running total so it doesnt get added to the backup total
    ui_print("Space used: %ld MB\n", compute_size(PREFIX, 1)/1024/1024);
  }
  ui_print("Elapsed time: %ld seconds\n", elapsed);
}