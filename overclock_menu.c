#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "recovery.h"
#include "roots.h"
#include "recovery_ui.h"

void set_oc (char *speed)
{
  FILE * f = fopen ("/tmp/.rzrpref_oc", "w");
  fputs (speed, f);
  fputs ("\n", f);
  fclose (f);
  set_cpufreq (speed);
  ui_print ("max frequency set to %s Hz\n", speed);
}

int slot_count (char *s) 
{
  int i = 0;
  int counter = 0;

  while (s[i] != '\0')
    
	  {
	    if (s[i++] == ' ')
	      counter++;
	  }
  return counter;
}

 char *
get_available_frequencies ()
{
  FILE * available_frequencies_file =
    fopen
    ("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies",
     "r");
  char *available_frequencies = calloc (255, sizeof (char));

  fgets (available_frequencies, 255, available_frequencies_file);
  fclose (available_frequencies_file);
  return available_frequencies;
}

 char *
get_max_freq ()
{
  FILE * fs =
    fopen ("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq", "r");
  char *freq = calloc (9, sizeof (char));

  fgets (freq, 9, fs);
  fclose (fs);
  return freq;
}

 void
show_overclock_menu ()
{
  if (access
       ("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies",
	F_OK))
	  {
	    ui_print ("scaling_available_frequencies not found.");
	    return;
	  }
   char *available_frequencies = get_available_frequencies ();
  char *freq = get_max_freq ();
   int freqlen = strlen (freq);

  if (freq[freqlen - 1] == '\n')
	  {
	    freq[freqlen - 1] = 0;
	  }
  freqlen = strlen (freq);
  if (freq[freqlen - 1] == '\n')
	  {
	    freq[freqlen - 1] = 0;
	  }
   
    // create an array from available_frequencies delimited by spaces - crude and
    // I hope it works on all devices 
  char **ap, **slots;
  int arglen = 35;

  slots = calloc (arglen, sizeof (char *));
  for (ap = slots; (*ap = strsep (&available_frequencies, " \t")) != NULL;)
    if (**ap != '\0')
      if (++ap >= &slots[arglen])
	
	      {
		arglen += 35;
		slots = realloc (slots, arglen);
		ap = &slots[arglen - 35];
	      }
  
    //since there is no way to delete the last element, I will copy all the elements up to num_slots
    //into a new available_slots array and then free the slots array
  int num_slots = slot_count (get_available_frequencies ());
  char **available_slots;

  available_slots = calloc (arglen, sizeof (char *));
  int i;

  for (i = 0; i < num_slots; i++)
	  {
	    available_slots[i] = slots[i];
	  }
  free (slots);
   
  //create pretty header lines
  char max_string[50]; 

  int freqMHz = atoi(freq)/1000;
  char freqstring[50];

  sprintf(freqstring,"%i",freqMHz);

  strcpy(max_string,"Curr Max Speed: ");
  strcat(max_string, freqstring);
  strcat(max_string," MHz");

    char *headers[] = { "Recovery CPU settings", max_string, " ", NULL
  };
   
   int chosen_item = -1;

   while (chosen_item != ITEM_BACK)
	  {
	    chosen_item =
	      get_menu_selection (headers, available_slots, 0,
				  chosen_item < 0 ? 0 : chosen_item);
	    if (chosen_item == ITEM_BACK)
		    {
		      return;
		    }
	     switch (chosen_item)
		{
		  default:
			set_oc(available_slots[chosen_item]);
		    return;				  
		}
	  }
}


