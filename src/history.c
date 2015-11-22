/*
 * Playground+ - historee.c
 * History lib for creating, maintaining and cleaning dynamic
 *  history buffers... an excersise in pointer play (c) phypor 1998
 * ---------------------------------------------------------------------------
 *
 * Modifications to original release:
 *  Include paths
 *  commented out su history stufs, to be used as an example in creating others
 *  removed strcasestr func, as its found in xstring.c for pg+
 *  changed malloc to MALLOC and free to FREE
 *
 * Notes:
 *     to add a history to anything else, just give it
 *     a history * pointer, either in the struct for it
 *     or as a global var (depending on how you will need it),
 *     init_history to give it some memory,
 *     add items to it with the add_to_hist,
 *     use stack_hist as a possible method to retrieve the history,
 *     and free_hist when you are finished.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "include/config.h"
#include "include/player.h"
#include "include/proto.h"


history *init_hist(int max)
{
  history *nh;

  nh = (history *) MALLOC(sizeof(history));
  if (!nh)
  {
    log("error", "Failed to malloc history.");
    return (history *) NULL;
  }

  memset(nh, 0, sizeof(history));

  nh->hist = (char **) MALLOC((max + 1) * sizeof(char *));
  if (!nh->hist)
  {
    FREE(nh);
    log("error", "Failed to malloc history.");
    return (history *) NULL;
  }
  memset(nh->hist, 0, max * sizeof(char *));

  nh->max = max;
  return nh;
}


void add_to_hist(history * h, char *str)
{
  int i;

  if (!h)			/* malloc failed at some point... */
    return;

  for (i = 0; i < h->max; i++)	/* find the first free one */
    if (!h->hist[i])
      break;

  if (i == h->max)		/* if the handles are full */
  {
    FREE(h->hist[i - 1]);	/* get rid of the last one */
    h->hist[i - 1] = (char *) NULL;
  }

  for (i = h->max; i > 0; i--)	/* shift the handles down */
    h->hist[i] = h->hist[i - 1];

  /* malloc something to point to */
  h->hist[0] = (char *) MALLOC(strlen(str) + 1);

  if (!h->hist[0])		/* make sure we got it */
  {
    log("error", "Failed to malloc history");
    return;
  }
  /* clean it and put it in */
  memset(h->hist[0], 0, strlen(str) + 1);
  strcpy(h->hist[0], str);
}

int stack_hist(history * h, int lines, char *needle)
{
  int i;

  if (!h || !h->hist[0])
    return -1;

  for (i = 0; h->hist[i + 1]; i++);	/* get to the end to put the oldest top */

  if (lines > 0)
    i = i < lines ? i : lines - 1;	/* use the smallest */

  if (!needle || !*needle)
    for (; i >= 0; i--)
      stack += sprintf(stack, "%s", h->hist[i]);
  else
    for (; i >= 0; i--)
      if (strcasestr(h->hist[i], needle))
	stack += sprintf(stack, "%s", h->hist[i]);

  return 0;
}

void clean_hist(history * h)
{
  int i;

  if (!h || !h->hist)		/* make sure its got memory */
    return;

  for (i = 0; i < h->max; i++)
    if (h->hist[i])
    {
      FREE(h->hist[i]);
      h->hist[i] = (char *) NULL;
    }
}

void free_hist(history * h)
{
  clean_hist(h);
  FREE(h->hist);
  FREE(h);
}


void init_global_histories(void)
{
/* init any global histories here...such as 
   SuHistory = init_hist (HIST_SU);
 */

}




#ifdef EXAMPLES
  /* su history */
#define HIST_SU         50

history *SuHistory;


void zap_suhist(player * p, char *str)
{
  clean_hist(SuHistory);
  tell_player(p, " SuperUsers History zapped!\n");
}


void view_su_history(player * p, char *str)
{
  char *oldstack = stack, *needle = "";
  int lines = -1;

  if (*str && !strcmp(str, "?"))	/* use strcmp so ? can be first char of
					   pattern to be matched */
  {
    tell_player(p,
    " Format : suhist                to view the entire suhist, unabriged\n"
       "          suhist #              to view the mot recent # of lines\n"
		"          suhist <str>          to view all lines strcasestr'ing <str>\n"
		"          suhist ?              to view the format\n");
    return;
  }

  if (*str && isdigit(*str))
  {
    lines = atoi(str);
    if (lines < 2)
      lines = 2;
    stack += sprintf(stack, "--- SuperUsers History (last %d lines) ---\n",
		     lines);
  }
  else if (*str)
  {
    stack += sprintf(stack, "--- SuperUsers History (searching %s) ---\n",
		     str);
    needle = str;
  }
  else
    stack += sprintf(stack, "--- SuperUsers History --------------------\n")
      ;

  if (stack_hist(SuHistory, lines, needle) < 0)
  {
    tell_player(p, " SuperUsers History is empty.\n");
    stack = oldstack;
    return;
  }
  stack += sprintf(stack, "-------------------------------------------\n");
  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
}

#endif /* EXAMPLES */
