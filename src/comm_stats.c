/*
 * Playground+ - comm_stats.c
 * Command stats code by Phyphor
 * ---------------------------------------------------------------------------
 *
 *  Changes to original:
 *    - Cleaned up presentation of commands
 *    - Usage of Admin and SU commands are not logged
 *    - Code tidely indented
 *    - Change of stack handling to be compliant with PG+
 *    - Change of header locations
 *
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

#include "include/config.h"
#include "include/player.h"
#include "include/proto.h"
#include "include/fix.h"

#ifdef COMMAND_STATS

 /* this should be larger than the amount of commands you have */
#define COM_ARRAY_SIZE		2000

typedef struct comentry
{
  char which[80];
  int hitz;
}
comentry;

comentry *ExistantCommands[COM_ARRAY_SIZE];



comentry *findExistantCommand(char *str)
{
  int i;

  for (i = 0; ExistantCommands[i]; i++)
    if (!strcasecmp(ExistantCommands[i]->which, str))
      return ExistantCommands[i];

  return (comentry *) NULL;
}

void addToExistantCommand(comentry * this)
{
  int i;

  for (i = 0; (ExistantCommands[i] && i < COM_ARRAY_SIZE); i++);

  if (i == COM_ARRAY_SIZE)	/* array is full dammit */
    return;

  ExistantCommands[i] = this;
}


void commandUsed(char *str)
{
  comentry *that;

  that = findExistantCommand(str);
  if (that)
  {
    that->hitz++;
    return;
  }
  that = (comentry *) MALLOC(sizeof(comentry));
  if (!that)
  {
    log("error", "malloc fails in commandUsed()");
    return;
  }
  memset(that, 0, sizeof(comentry));

  that->hitz = 1;
  strncpy(that->which, str, strlen(str));

  addToExistantCommand(that);
}

/* we have to loop through twice, the first time to get the total,
   the second to get ratios and stuff */
void statcommands(player * p, char *str)
{
  char *oldstack = stack;
  int total, masstotal;
  int i, l;
  int tp;
  char middle[80];


  pstack_mid("Command Statistics");

  for (masstotal = 0, total = 0; ExistantCommands[total]; total++)
    masstotal += ExistantCommands[total]->hitz;

/* the way this is formated for output isnt perfect (altho works)
   it wouldnt mind some tlc making it so
 */
  for (i = 0, l = 1; ExistantCommands[i]; i++, l++)
  {
    tp = ((ExistantCommands[i]->hitz * 10000) / masstotal);

    ADDSTACK("%13s %4d %3d%%", ExistantCommands[i]->which,
	     ExistantCommands[i]->hitz, tp / 100);
    if (l > 2)
    {
      ADDSTACK("\n");
      l = 0;
    }
  }

  if (l != 1)
    ADDSTACK("\n");

  sprintf(middle, "%d commands used %d times", total, masstotal);

  pstack_mid(middle);
  *stack++ = 0;

  pager(p, oldstack);

  stack = oldstack;
}

void command_stats_version()
{
  sprintf(stack, " -=*> Command stats v1.0 (by phypor) enabled.\n");
  stack = strchr(stack, 0);
}

#endif
