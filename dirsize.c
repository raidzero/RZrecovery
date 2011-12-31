#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

long totalbytes = 0;
#define PATH_MAX 4096

long dirsize(const char* directory, int verbose)
{
  struct dirent *de;
  struct stat s;
  char pathname[PATH_MAX];
  DIR * dir; 
  //long total_items = 0;
  long filesize = 0;
  
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
		printf("Error in stat!\n");
		return -1;
	  }
	  if (verbose) 
	  {
	    printf("%s/%s : %ld bytes\n", directory, de->d_name, s.st_size);
	  }
	  filesize = s.st_size; //put file size into filesize variable
	  totalbytes += filesize; //increment totalbytes
	}
	if (de->d_type == DT_DIR && strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0)
	{
	  sprintf(pathname, "%s/%s", directory, de->d_name);
	  dirsize(pathname, verbose); //recursion: keep looping until no more subdirs remain
	}
  }
  closedir(dir);
  return totalbytes;
}
	
long compute_size(const char* directory, int verbose)
{
  long space = dirsize(directory, verbose);
  return space;
}
  
int dirsize_main(int argc, char* argv[])
{
  if (argc != 2)
  {
    printf("Usage: dirsize DIRECTORY\n");
	return -1;
  }

  int verbose = 0; //show or hide individual computations

  long space = compute_size(argv[1], verbose);
  if (space != -1)
  {
    float space_mb = (float) space / 1024 / 1024;
    printf("space occupied: %ld bytes\n", space);
    printf("(%f MB)\n", space_mb);
  }
  return 0;
}
