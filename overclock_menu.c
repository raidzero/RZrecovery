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

  strcpy(max_string,"Current Max Speed: ");
  strcat(max_string, freqstring);
  strcat(max_string," MHz");

    char *headers[] = { "Recovery CPU settings", max_string, " ", NULL
  };
   
#define slot1         		0
#define slot2			1
#define slot3		    	2
#define slot4     		3
#define slot5	    		4
#define slot6			5
#define slot7			6
#define slot8			7
#define slot9			8
#define slot10			9
#define slot11			10
#define slot12                  11
#define slot13                  12
#define slot14                  13
#define slot15                  14
#define slot16                  15
#define slot17                  16
#define slot18                  17
#define slot19                  18
#define slot20                  19
#define slot21                  20
#define slot22                  21
#define slot23                  22
#define slot24                  23
#define slot25                  24
#define slot26                  25
#define slot27                  26
#define slot28                  27
#define slot29                  28
#define slot30                  29
#define slot31                  30
#define slot32                  31
#define slot33                  32
#define slot34                  33
#define slot35                  34



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
		    case slot1:
		      set_oc (available_slots[0]);
		      return;
		    case slot2:
		      set_oc (available_slots[1]);
		      return;
		    case slot3:
		      set_oc (available_slots[2]);
		      return;
		    case slot4:
		      set_oc (available_slots[3]);
		      return;
		    case slot5:
		      set_oc (available_slots[4]);
		      return;
		    case slot6:
		      set_oc (available_slots[5]);
		      return;
		    case slot7:
		      set_oc (available_slots[6]);
		      return;
		    case slot8:
		      set_oc (available_slots[7]);
		      return;
		    case slot9:
		      set_oc (available_slots[8]);
		      return;
		    case slot10:
		      set_oc (available_slots[9]);
		      return;
		    case slot11:
		      set_oc (available_slots[10]);
		      return;
		    case slot12:
		      set_oc (available_slots[11]);
		      return;
		    case slot13:
		      set_oc (available_slots[12]);
		      return;			  
		    case slot14:
		      set_oc (available_slots[13]);
		      return;			  
		    case slot15:
		      set_oc (available_slots[14]);
		      return;
		    case slot16:
		      set_oc (available_slots[15]);
		      return;			  
		    case slot17:
		      set_oc (available_slots[16]);
		      return;
		    case slot18:
		      set_oc (available_slots[17]);
		      return;
		    case slot19:
		      set_oc (available_slots[18]);
		      return;	
		    case slot20:
		      set_oc (available_slots[19]);
		      return;			  
		    case slot21:
		      set_oc (available_slots[20]);
		      return;	
		    case slot22:
		      set_oc (available_slots[21]);
		      return;				  
		    case slot23:
		      set_oc (available_slots[22]);
		      return;	
		    case slot24:
		      set_oc (available_slots[23]);
		      return;	
		    case slot25:
		      set_oc (available_slots[24]);
		      return;	
		    case slot26:
		      set_oc (available_slots[25]);
		      return;	
		    case slot27:
		      set_oc (available_slots[26]);
		      return;	
		    case slot28:
		      set_oc (available_slots[27]);
		      return;	
		    case slot29:
		      set_oc (available_slots[28]);
		      return;				  
		    case slot30:
		      set_oc (available_slots[29]);
		      return;		
		    case slot31:
		      set_oc (available_slots[30]);
		      return;	
		    case slot32:
		      set_oc (available_slots[31]);
		      return;	
		    case slot33:
		      set_oc (available_slots[32]);
		      return;	
		    case slot34:
		      set_oc (available_slots[33]);
		      return;	
		    case slot35:
		      set_oc (available_slots[34]);
		      return;				  
		}
	  }
}


