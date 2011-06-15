#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "recovery.h"
#include "roots.h"
#include "recovery_ui.h"

void set_oc(char* speed) {
	FILE* f = fopen("/cache/oc","w");
    fputs(speed, f);
	fputs("\n",f);
	fclose(f);
	set_cpufreq(speed);
	ui_print("\nmax frequency set to ");
	ui_print("%s",speed);
}

int slot_count(char* s)
{                              
  int i=0;                  
  int counter = 0;            
  while (s[i] != '\0')       
  {                               
    if (s[i++] ==' ')
      counter++;
  }
  return counter;
}

char* get_available_frequencies() {
	FILE* available_frequencies_file = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies","r");
	char* available_frequencies = calloc(70,sizeof(char));
	fgets(available_frequencies, 70, available_frequencies_file);
	fclose(available_frequencies_file);
	return available_frequencies;
}

char* get_current_governor() {
	FILE* gs = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor","r");
    char* current_governor = calloc(20,sizeof(char));
    fgets(current_governor, 20, gs);
	fclose(gs);
	return current_governor;
}

char* get_max_freq() {
	FILE* fs = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq","r");
    char* freq = calloc(8,sizeof(char));
    fgets(freq, 8, fs);
	fclose(fs);
	return freq;
}

void show_overclock_menu() {
	if (access("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies",F_OK)) {
		ui_print("\nscaling_available_frequencies not found.");
		return;
	}
	
	char* available_frequencies = get_available_frequencies();
	char* current_governor = get_current_governor();
	char* freq = get_max_freq();
	
	int freqlen = strlen(freq);
	if( freq[freqlen-1] == '\n' ) {
	freq[freqlen-1] = 0;
	}
	
	freq = strcat(freq," KHz");
	freqlen = strlen(freq);
	if( freq[freqlen-1] == '\n' ) {
	freq[freqlen-1] = 0;
	}
	
	// create an array from available_frequencies delimited by spaces - crude and
	// I hope it works on all devices 
	char **ap, **slots;
	int arglen = 10;
	slots = calloc(arglen, sizeof(char*));
	for (ap = slots; (*ap = strsep(&available_frequencies, " \t")) != NULL;)
		if (**ap != '\0')
			if (++ap >= &slots[arglen])
			{
				arglen += 10;
				slots = realloc(slots, arglen);
				ap = &slots[arglen-10];
			}	
	
	//since there is no way to delete the last element, I will copy all the elements up to num_slots
	//into a new available_slots array and then free the slots array
	int num_slots = slot_count(get_available_frequencies());		
	char** available_slots;
	available_slots = calloc(arglen, sizeof(char*));
	int i;
	for (i=0; i<num_slots; i++) {
		available_slots[i]=slots[i];
	}
	free(slots);
	
	//create pretty header lines
	char governor_string[50];
	char max_string[50];
	strcpy(governor_string, "Current governor: ");
	strcat(governor_string, current_governor);	
	strcpy(max_string, "Current max speed: ");
	strcat(max_string, freq);
	char slot_string[25];
	if (num_slots == 5) strcpy(slot_string,"5 slots available:");
	if (num_slots == 7) strcpy(slot_string,"7 slots available:");
	
    char* headers[] = { "Recovery CPU settings",
				   max_string,
				   governor_string,
				   slot_string,
				   " ",
			       NULL };
			  
#define slot1         	0
#define slot2			1
#define slot3		    2
#define slot4     		3
#define slot5	    	4
#define slot6			5
#define slot7			6

int chosen_item = -1;

    while(chosen_item!=ITEM_BACK) {
	chosen_item = get_menu_selection(headers,available_slots,1,chosen_item<0?0:chosen_item);
	if (chosen_item == ITEM_BACK) {
        return;
	}

        switch (chosen_item) {
	case slot1:
		set_oc(available_slots[0]);
	    return;
	case slot2:
		set_oc(available_slots[1]);
		return;
	case slot3:
		set_oc(available_slots[2]);
		return;
	case slot4:
		set_oc(available_slots[3]);
		return;
	case slot5:
		set_oc(available_slots[4]);
		return;
	case slot6:
		set_oc(available_slots[5]);
		return;	
	case slot7:
		set_oc(available_slots[6]);
		return;
        }
    }
}
