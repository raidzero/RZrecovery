#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/reboot.h>
#include <unistd.h>

#include "rz_funcs.h"


#undef _PATH_BSHELL
#define _PATH_BSHELL "/sbin/sh"

int
__system(const char *command)
{
  pid_t pid;
  sig_t intsave, quitsave;
  sigset_t mask, omask;
  int pstat;
  char *argp[] = { "sh", "-c", NULL, NULL };
  if (!command)			/* just checking... */
    return (1);
  argp[2] = (char *) command;
  sigemptyset(&mask);
  sigaddset(&mask, SIGCHLD);
  sigprocmask(SIG_BLOCK, &mask, &omask);
  switch (pid = vfork())
  {
  case -1:			/* error */
    sigprocmask(SIG_SETMASK, &omask, NULL);
    return (1);
  case 0:			/* child */
    sigprocmask(SIG_SETMASK, &omask, NULL);
    execve(_PATH_BSHELL, argp, environ);
    _exit(127);
  }

  intsave = (sig_t) bsd_signal(SIGINT, SIG_IGN);
  quitsave = (sig_t) bsd_signal(SIGQUIT, SIG_IGN);
  pid = waitpid(pid, (int *) &pstat, 0);
  sigprocmask(SIG_SETMASK, &omask, NULL);
  (void) bsd_signal(SIGINT, intsave);
  (void) bsd_signal(SIGQUIT, quitsave);
  return (pid == -1 ? -1 : pstat);
}

int
ask_question(char *question)
{
  char *headers[] = { question,
    "",
    NULL
  };

  char *items[] = { "No",
    "Yes",
    NULL
  };

  int chosen_item = get_menu_selection(headers, items, 0, 0);

  if (chosen_item == 1)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

int
confirm_selection(char *question, char *operation, int autoaccept)
{
  if (autoaccept)
    return 1;

  char *headers[] = { question,
    "THIS CANNOT BE UNDONE!",
    "",
    NULL
  };

  char *items[] = { "No",
    "No",
    operation,
    "No",
    NULL
  };

  int chosen_item = get_menu_selection(headers, items, 0, 0);

  if (chosen_item == 2)
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

char *
__replace(char *st, char *orig, char *repl)
{
  static char buffer[4096];
  char *ch;
  if (!(ch = strstr(st, orig)))
    return st;
  strncpy(buffer, st, ch - st);
  buffer[ch - st] = 0;
  sprintf(buffer + (ch - st), "%s%s", repl, ch + strlen(orig));
  return buffer;
}

int
runve(char *filename, char **argv, char **envp, int secs)
{
  puts(replace(filename, " ", "\\ "));
  printf("About to run %s...\n", filename);
  int opipe[2];
  int ipipe[2];

  pipe(opipe);
  pipe(ipipe);
  pid_t pid = fork();
  int status = 0;

  if (pid == 0)
  {
    dup2(opipe[1], 1);
    dup2(ipipe[0], 0);
    close(opipe[0]);
    close(ipipe[1]);
    execve(filename, argv, envp);
    char *error_msg = calloc(strlen(filename) + 20, sizeof(char));

    sprintf(error_msg, "Could not execute %s\n", filename);
    ui_print("%s", error_msg);
    free(error_msg);
    return (9999);
  }
  close(opipe[1]);
  close(ipipe[0]);
  FILE *from = fdopen(opipe[0], "r");
  FILE *to = fdopen(ipipe[1], "w");
  char *cur_line = calloc(100, sizeof(char));
  char *tok;
  int total_lines;
  int cur_lines;
  int num_items;
  int num_headers;
  int num_chks;
  float cur_progress;
  char **items = NULL;
  char **headers = NULL;
  char **chks = NULL;
  int i = 0;			// iterator for menu items
  int j = 0;			// iterator for menu headers
  int k = 0;			// iterator for check menu items
  int l = 0;			// iterator for outputting flags from check menu
  int flags = INT_MAX;
  int choice;

  while (fgets(cur_line, 100, from) != NULL)
  {
    printf("%s", cur_line);
    tok = strtok(cur_line, " \n");
    if (tok == NULL)
    {
      continue;
    }
    if (strcmp(tok, "*") == 0)
    {
      tok = strtok(NULL, " \n");
      if (tok == NULL)
      {
	continue;
      }
      if (strcmp(tok, "ptotal") == 0)
      {
	ui_set_progress(0.0);
	ui_show_progress(1.0, 0);
	total_lines = atoi(strtok(NULL, " "));
      }
      else if (strcmp(tok, "print") == 0)
      {
	ui_print("%s", strtok(NULL, ""));
      }
      else if (strcmp(tok, "reset_text_col") == 0)
      {
	ui_reset_text_col();
      }
      else if (strcmp(tok, "items") == 0)
      {
	num_items = atoi(strtok(NULL, " \n"));
	if (items != NULL)
	  free(items);
	items = calloc((num_items + 1), sizeof(char *));
	items[num_items] = NULL;
	i = 0;
      }
      else if (strcmp(tok, "item") == 0)
      {
	if (i < num_items)
	{
	  tok = strtok(NULL, "\n");
	  items[i] = calloc((strlen(tok) + 1), sizeof(char));
	  strcpy(items[i], tok);
	  i++;
	}
      }
      else if (strcmp(tok, "headers") == 0)
      {
	num_headers = atoi(strtok(NULL, " \n"));
	if (headers != NULL)
	  free(headers);
	headers = calloc((num_headers + 1), sizeof(char *));
	headers[num_headers] = NULL;
	j = 0;
      }
      else if (strcmp(tok, "header") == 0)
      {
	if (j < num_headers)
	{
	  tok = strtok(NULL, "\n");
	  if (tok)
	  {
	    headers[j] = calloc((strlen(tok) + 1), sizeof(char));
	    strcpy(headers[j], tok);
	  }
	  else
	  {
	    headers[j] = "";
	  }
	  j++;
	}
      }
      else if (strcmp(tok, "show_menu") == 0)
      {
	choice = get_menu_selection(headers, items, 0, 0);
	fprintf(to, "%d\n", choice);
	fflush(to);
      }
      else if (strcmp(tok, "pcur") == 0)
      {
	cur_lines = atoi(strtok(NULL, "\n"));
	if (cur_lines % 10 == 0 || total_lines - cur_lines < 10)
	{
	  cur_progress = (float) cur_lines / ((float) total_lines);
	  ui_set_progress(cur_progress);
	}
	if (cur_lines == total_lines)
	  ui_reset_progress();
      }
      else if (strcmp(tok, "check_items") == 0)
      {
	num_chks = atoi(strtok(NULL, " \n"));
	if (chks != NULL)
	  free(chks);
	chks = calloc((num_chks + 1), sizeof(char *));
	chks[num_chks] = NULL;
	k = 0;
      }
      else if (strcmp(tok, "check_item") == 0)
      {
	if (k < num_chks)
	{
	  tok = strtok(NULL, "\n");
	  chks[k] = calloc(strlen(tok) + 1, sizeof(char));
	  strcpy(chks[k], tok);
	  k++;
	}
      }
      else if (strcmp(tok, "show_check_menu") == 0)
      {
	show_check_menu(headers, chks, &flags);
	for (l = 0; l < num_chks; l++)
	{
	  fprintf(to, "%d\n", ! !(flags & (1 << l)));
	}
	fflush(to);
      }
      else if (strcmp(tok, "show_indeterminate_progress") == 0)
      {
	ui_show_indeterminate_progress();
      }
      else if (strcmp(tok, "reset_progress") == 0)
      {
	ui_reset_progress();
      }
      else if (strcmp(tok, "get_key") == 0)
      {
	return get_key;
      }

      else
      {
	ui_print("unrecognized command ");
	ui_print("%s", tok);
	ui_print("\n");
      }
    }
  }
  while (waitpid(pid, &status, 0) == 0)
  {
    sleep(1);
  }
  ui_print("\n");
  free(cur_line);
  return status;
}



int
get_key()
{
  int key, action;
  key = ui_wait_key();
  action = device_handle_key(key);
  return key;
}

void
ui_printf_int(const char *format, int arg)
{
  char *out = calloc(strlen(format) + 12, sizeof(char));

  sprintf(out, format, arg);
  ui_print("%s", out);
  free(out);
}

void
get_check_menu_opts(char **items, char **chk_items, int *flags)
{
  int i;
  int count;

  for (count = 0; chk_items[count]; count++);	// count it
  for (i = 0; i < count; i++)
  {
    if (items[i] != NULL)
      free(items[i]);
    items[i] = calloc(strlen(chk_items[i]) + 5, sizeof(char));	// 4 for "(*) " and 1 for the NULL-terminator
    sprintf(items[i], "(%s) %s", (*flags & (1 << i)) ? "*" : " ",
	    chk_items[i]);
  }
}

void
show_check_menu(char **headers, char **chk_items, int *flags)
{
  int chosen_item = -1;
  int i;

  for (i = 0; chk_items[i]; i++);	// count it
  i += 2;			// i is the index of NULL in chk_items[], and we want an array 1 larger than chk_items, so add 1 for that and 1 for the NULL at the end => i+2
  char **items = calloc(i, sizeof(char *));

  items[0] = "Finished";
  items[i] = NULL;
  while (chosen_item != 0)
  {
    get_check_menu_opts(items + 1, chk_items, flags);	// items+1 so we don't overwrite the Finished option
    chosen_item =
      get_menu_selection(headers, items, 0,
			 chosen_item == -1 ? 0 : chosen_item);
    if (chosen_item == 0)
    {
      continue;
    }
    chosen_item -= 1;		// make it 0-indexed for bit-flipping the int
    *flags ^= (1 << chosen_item);	// flip the bit corresponding to the chosen item
    chosen_item += 1;		// don't ruin the loop!
  }
}
