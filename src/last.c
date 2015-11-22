/*
 * Playground+ - last.c
 * Tracing of last connections to the talker by Phypor
 * -----------------------------------------------------------------------
 *
 *  Modifications made:
 *    - Tidyed up code and presentation
 *    - Changed location of header files
 */

#include "include/config.h"
#ifdef LAST

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "include/player.h"
#include "include/proto.h"

 /* how many entries are kept total */
#define LAST_KEPT 200

 /* non configuration define */
#define STILL_IN -1

struct lastinfo
{
  char name[MAX_NAME];
  int in;
  int out;
};
typedef struct lastinfo lastinfo;

lastinfo Wtmp[LAST_KEPT];

char *diff_time(int, int);

char *diff_time(int start, int end)
{
  int ne;
  static char be[40];


  start -= time(0);
  end -= time(0);
  ne = end - start;
  if (ne < 1)
    return "Quick";

  memset(be, 0, 40);
  sprintf(be, "%.2d:%.2d.%.2d", (ne / (60 * 60)), ((ne / 60) % 60),
	  (ne % 60));

  return be;
}


void stampLogin(char *name)
{
  int i = 0;

  /* I'm gonna beat whoever forgot the - 1 here to death ;-) -- astyanax */
  /* It was phypor! Not me! -- Silver */
  for (i = LAST_KEPT - 1; i > 0; i--)
    memcpy(&Wtmp[i], &Wtmp[i - 1], sizeof(lastinfo));

  strncpy(Wtmp[0].name, name, MAX_NAME - 1);
  Wtmp[0].in = time(0);
  Wtmp[0].out = STILL_IN;
}

void stampLogout(char *name)
{
  int i = 0;

  while (i < LAST_KEPT)
  {
    if (!strcasecmp(Wtmp[i].name, name))
      break;
    i++;
  }
  if (i == LAST_KEPT)
    return;

  Wtmp[i].out = time(0);
}

void viewLast(player * p, char *str)
{
  int i, j, hit = 0;
  char *oldstack = stack, top[70];

  sprintf(top, "Last %d connections to %s", LAST_SHOW,
	  get_config_msg("talker_name"));
  pstack_mid(top);

  if (*str && isdigit(*str))
  {
    j = atoi(str);
    if (j < 1)
      j = LAST_SHOW;
    if (j > LAST_KEPT)
      j = LAST_KEPT;
    *str = '\0';
  }
  else if (*str)
    j = LAST_KEPT;
  else
    j = LAST_SHOW;


  for (i = 0; (i < LAST_KEPT && hit < j); i++)
  {
    if (Wtmp[i].name && Wtmp[i].name[0])
    {
      if (Wtmp[i].out == STILL_IN)
	continue;

      if (*str && strcasecmp(str, Wtmp[i].name))
	continue;

      ADDSTACK("%-15s %-42s ", Wtmp[i].name,
	       convert_time((Wtmp[i].out + (p->jetlag * 3600))));

      ADDSTACK("%s\n",
	       diff_time(Wtmp[i].in, Wtmp[i].out));

      hit++;
    }
  }
  if (!hit)
    tell_player(p, "No connections logged.\n");
  else
  {
    ENDSTACK(LINE);
    pager(p, oldstack);
  }
  stack = oldstack;
}

void last_version()
{
  sprintf(stack, " -=*> Last site logging v1.0 (by phypor) enabled.\n");
  stack = strchr(stack, 0);
}

#endif
