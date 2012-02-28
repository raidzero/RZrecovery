#include <stdio.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

#include "roots.h"
#include "popen.h"

uint64_t totalbytes = 0;
uint64_t totalfiles = 0;
int clearTotal = 0;
int clearFilesTotal = 0;

char** files_list;
uint64_t list_position = 0;

#define PATH_MAX 4096

int get_clearTotal_intent()
{
   return clearTotal;
}

void set_clearTotal_intent(int value)
{
   clearTotal = value;
}

int get_clearFilesTotal_intent()
{
   return clearFilesTotal;
}

void set_clearFilesTotal_intent(int value)
{
   clearFilesTotal = value;
}

uint64_t dirsize(const char* directory, int verbose)
{
  if (clearTotal) 
  {
    printf("Clear total intent received.\n");
	totalbytes = 0;
	clearTotal = 0;
  }
  
  struct dirent *de;
  struct stat s;
  char pathname[PATH_MAX];
  DIR * dir; 
  //long total_items = 0;
  uint64_t filesize = 0;
  
  dir = opendir(directory);
  if (dir == NULL)
  {
    printf("Failed to open %s.\n", directory);
	return -1;
  }
  
  while ((de = readdir (dir)) != NULL)
  {
	if (de->d_type == DT_REG)
	{
	  filesize = 0; //be sure to reset this each time to avoid inaccuracy
	  sprintf(pathname, "%s/%s", directory, de->d_name);
	  if (stat(pathname, &s))
	  {
		printf("Error running stat!\n");
		return -1;
	  }
	  if (verbose) 
	  {
	    printf("%s/%s : %llu bytes\n", directory, de->d_name, s.st_size);
	  }
	  filesize = s.st_size; //put file size into filesize variable
	  totalbytes += filesize; //increment totalbytes
	}
	if (de->d_type == DT_DIR && strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0)
	{
	  totalbytes += 4096; //the size of a directory in ls
	  sprintf(pathname, "%s/%s", directory, de->d_name);
	  dirsize(pathname, verbose); //recursion: keep looping until no more subdirs remain
	}
  }
  closedir(dir);
  return totalbytes;
}
	
long dirfiles(const char* directory)
{
  if (clearFilesTotal) 
  {
    printf("Clear total files intent received.\n");
	 totalfiles = 0;
	 clearFilesTotal = 0;
  }  
  
  struct dirent *de;
  char pathname[PATH_MAX];
  DIR * dir; 

  dir = opendir(directory);
  if (dir == NULL)
  {
    printf("Failed to open %s.\n", directory);
	return 0;
  }
  
  while ((de = readdir (dir)) != NULL)
  {
	if (de->d_type == DT_REG)
	{
	  totalfiles += 1; //increment totalfiles
	}
	if (de->d_type == DT_LNK)
	{
	  totalfiles += 1;
	}
	
	if (de->d_type == DT_DIR && strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0)
	{
	  totalfiles += 1; //increment totalfiles
	  sprintf(pathname, "%s/%s", directory, de->d_name);
	  dirfiles(pathname); //recursion: keep looping until no more subdirs remain
	}
  }
  closedir(dir);
  return totalfiles;
}

void listfiles(const char* directory)
{
  struct dirent *de;
  char pathname[PATH_MAX];
  DIR * dir; 

  dir = opendir(directory);
  if (dir == NULL)
  {
    printf("Failed to open %s.\n", directory);
	return;
  }
  
  while ((de = readdir (dir)) != NULL)
  {
	if (de->d_type == DT_REG)
	{	  	  
	  sprintf(pathname, "%s/%s\0", directory, de->d_name);
	  files_list[list_position] = (char *) malloc (strlen(pathname) + strlen (de->d_name) +2);
	  strcpy(files_list[list_position], pathname);
	  list_position++;
	}
	if (de->d_type == DT_DIR && strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0)
	{
	  sprintf(pathname, "%s/%s", directory, de->d_name);
	  listfiles(pathname); //recursion: keep looping until no more subdirs remain
	}
  }
  closedir(dir);
}

long tarsize(const char* PREFIX, const char* FILENAME, const char* EXTENSION, const int COMPRESSED)
{
  char TAR_TVF[1024] = { NULL };
  char *TVF_OPTS = NULL;
  char files_in_tar[10] = { NULL };  
  long result = NULL;  
  if (COMPRESSED) TVF_OPTS = "tvzf";
  else TVF_OPTS="tvf";
	  
  sprintf(TAR_TVF, "tar %s %s/%s.%s | wc -l", TVF_OPTS, PREFIX, FILENAME, EXTENSION);  
  printf("TAR_TVF: %s\n", TAR_TVF);	  
  
  FILE* in=__popen(TAR_TVF, "r"); //use popen to execute the command
  //check to be sure the operation succeeded
  if (in == NULL)
  {
    printf("Null pointer! :(\n");
    return -1;
  }
  fgets(files_in_tar, PATH_MAX-1, in); // fgets to grab the output
  result = atol(files_in_tar);
  int filesretstatus = __pclose(in);     
  printf("File pclose exit code: %d\n", filesretstatus);
  if (filesretstatus == 0)
  {
    printf("Number of files in tar: %ld\n", result);  
    return result;
  }
  else return -1;
}

uint64_t compute_size(const char* directory, int verbose)
{
  uint64_t space = dirsize(directory, verbose);
  printf("RETURNING: %llu\n", space);
  return space;
}

long compute_files(const char* directory)
{
  clearTotal = get_clearTotal_intent();
  long files = dirfiles(directory);
  return files;
}

long freespace(const char* PATH)
{
	struct statfs s;
	Volume * storage_volume = volume_for_path(PATH);
	statfs(storage_volume->mount_point, &s);
	uint64_t sd_bsize = s.f_bsize;
	uint64_t sd_freeblocks = s.f_bavail;
	long available_mb = sd_bsize * sd_freeblocks / (long) (1024*1024);
	printf("free space: %ld\n", available_mb);
	return available_mb;
}  

int compute_size_main(int argc, char* argv[])
{
  if (argc != 2)
  {
    printf("Usage: dirsize DIRECTORY\n");
	return -1;
  }

  int verbose = 0; //show or hide individual computations

  uint64_t space = compute_size(argv[1], verbose);
  if (space != -1)
  {
    float space_mb = (float) space / 1024 / 1024;
    printf("space occupied: %llu bytes\n", space);
    printf("(%.2f MB)\n", space_mb);
  }
  return 0;
}

int freespace_main(int argc, char* argv[])
{
  if (argc != 2)
  {
    printf("Usage: freespace DIRECTORY\n");
	return -1;
  }


  long space = freespace(argv[1]);
  if (space != -1)
  {
    printf("free space: %ld\n", space);
  }
  return 0;
}

int compute_files_main(int argc, char* argv[])
{
  if (argc != 2)
  {
    printf("Usage: compute_files DIRECTORY\n");
	return -1;
  }

  long files = compute_files(argv[1]);
  printf("%ld files in %s\n", files, argv[1]);
  return 0;
}

char** get_files_list(const char* directory)
{
  long numfiles = compute_files(directory);
  files_list = (char **) malloc ((numfiles + 1) * sizeof (char *));
  listfiles(directory);
  
  return files_list;
}

int list_files_main(int argc, char* argv[])
{
  if (argc != 2)
  {
    printf("Usage: list_files DIRECTORY\n");
	return -1;
  }
  
  long numfiles = compute_files(argv[1]);
  files_list = (char **) malloc ((numfiles + 1) * sizeof (char *));
  listfiles(argv[1]);
  
  int i;
  for (i=0; i<numfiles; i++)
  {
    printf("%s\n", files_list[i]);
  }
  
  return 0;
}
