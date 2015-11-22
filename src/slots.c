/* 
 * Playground+ - slots.c 
 * Copyright (c) 1997 by Chris Allegretta - reproduced in PG+ with permission
 * ----------------------------------------------------------------------------
 *
 *  Modifications:
 *    - Made to clean compile
 *    - Presentation changes
 *    - Graphical frontend (phypor)
 *    - Vegas scoring option (phypor)
 *       (for those that are not familiar with 'vegas', it refers to 
 *        Las Vegas, Nevada, a very large resort town with many casinos
 *        and the like out in the middle of the dessert... it should also
 *        be noted that Nevada is the last state in america that still
 *        has legal (and income taxed) prostitution and bordellos...
 *        fun for the whole family...;)
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "include/config.h"
#include "include/player.h"
#include "include/proto.h"
#include "include/slots.h"

int check_jackpot(player * p, int *slot)
{

  if (slot[0] == slot[1] && slot[1] == slot[2])
  {
    int oldp = p->pennies;

    if (slot[0] == 0)		/* sevens */
    {
      sprintf(stack, " -=*> Jackpot! You win %d %s!\007\n\n", (pot / 2),
	      get_config_msg("cash_name"));
      p->pennies += (pot / 2);
      pot = pot / 2;
    }
    else if (slot[0] == 1)	/* cheerys */
    {
      sprintf(stack, " -=*> Jackpot! You win %d %s!\007\n\n", pot,
	      get_config_msg("cash_name"));
      p->pennies += pot;
      pot = 0;
    }
    else
    {
      sprintf(stack, " -=*> Jackpot! You win %d %s!\007\n\n", (pot / 4),
	      get_config_msg("cash_name"));
      p->pennies += (pot / 4);
      pot = (pot * 3) / 4;
    }
    stack = strchr(stack, 0);
    LOGF("slots", "%s jackpots winning %d", p->name, (p->pennies - oldp));
    return 1;
  }
  return 0;
}


void resolve_old_slots(player * p, int *slot, int sw)
{
  if (check_jackpot(p, slot))
    return;

  if ((slot[0] == slot[1]) || (slot[0] == slot[2]) || (slot[1] == slot[2]))
  {
    stack += sprintf(stack, " -=*> Not too bad, you get %d %s back.\n\n",
		     sw * 2, get_config_msg("cash_name"));
    p->pennies += (sw * 2);
    return;
  }
  stack += sprintf(stack, " -=*> Oh well, better luck next time...\n\n");
  pot += sw;
}

void resolve_vegas_slots(player * p, int *slot, int sw)
{
  char *msg = "";
  int mod;


  if (check_jackpot(p, slot))
    return;

  if (slot[0] == 1)		/* first is a cherry */
  {
    switch (slot[1])
    {
      case 1:			/* second is a cherry */
	switch (slot[2])
	{
	  case 0:		/* cherry cherry seven - wager * 10 */
	    msg = " -=*> Woohoo... got back %d %s !\n\n";
	    mod = sw * 10;
	    break;
	  case 2:		/* cherry cherry coin - wager * 7 */
	    msg = " -=*> Excellent... got back %d %s ...\n\n";
	    mod = sw * 7;
	    break;
	  default:		/* cherry cherry anything - wager * 5 */
	    msg = " -=*> Very nice... got back %d %s ...\n\n";
	    mod = sw * 5;
	}
	break;
      case 0:			/* cherry seven anything - wager * 5 */
	msg = " -=*> Very cool... got back %d %s !\n\n";
	mod = sw * 5;
	break;
      case 2:			/* cherry coin anything - wager * 3 */
	msg = " -=*> Nifty... got back %d %s ...\n\n";
	mod = sw * 3;
	break;
      default:			/* cherry anything anything - wager * 2 */
	msg = " -=*> Not bad... got back %d %s ...\n\n";
	mod = sw * 2;
    }
    p->pennies += mod;
    if (pot > 100)		/* dont suck the pot too dry */
      pot -= mod;
    stack += sprintf(stack, msg, mod, get_config_msg("cash_name"));
    return;
  }
  if (slot[0] == slot[1] && slot[2] == 0)	/* anything anything seven */
  {
    stack += sprintf(stack, " -=*> Rockin!  Got back %d %s ...\n\n",
		     sw * 5, get_config_msg("cash_name"));
    p->pennies += (sw * 5);
    if (pot > 100)
      pot -= (sw * 5);
    return;
  }
  if (slot[0] == slot[1] && slot[2] == 1)	/* anything anything cherry */
  {
    stack += sprintf(stack, " -=*> Nice... got back %d %s ...\n\n",
		     sw * 3, get_config_msg("cash_name"));
    p->pennies += (sw * 3);
    if (pot > 100)
      pot -= (sw * 3);
    return;
  }

  stack += sprintf(stack, " -=*> Oh well, better luck next time...\n\n");
  pot += sw;
}



/* Some (large) presentation changes here --Silver */

void slots(player * p, char *str)
{
  int slot[3], j, sw;
  char top[70];
  char *oldstack = stack;

  sw = atoi(get_config_msg("slot_wager"));
  if (p->pennies < sw)
  {
    TELLPLAYER(p, " Sorry, you dont have enough money to play.\n"
	       " (You need at least %d %s)\n", sw,
	       get_config_msg("cash_name"));
    return;
  }
  p->pennies -= sw;

  sprintf(top, "%s Slots", get_config_msg("talker_name"));
  pstack_mid(top);

  stack += sprintf(stack, "\nYou take a chance at the slots for %d %s "
		   "and you get ...\n\n", sw, get_config_msg("cash_name"));

  srand(time(0) % rand());
  slot[0] = (rand() % 12);
  slot[1] = (rand() % 12);
  slot[2] = (rand() % 12);

  if ((rand() % 100) + 1 < atoi(get_config_msg("player_favor")))
  {
    if (slot[0] != 1)
      slot[0] = 1;
    else if (slot[1] != 1)
      slot[1] = 1;
    else
      slot[2] = 1;
  }

  stack += sprintf(stack,
   "    -==================-  -=================-  -==================-\n");

  for (j = 1; j < 8; j++)	/* loop through each line of pic */
  {
    stack += sprintf(stack, "   ||%s||%s||%s||\n",
       SlotsPics[slot[0]][j], SlotsPics[slot[1]][j], SlotsPics[slot[2]][j]);
  }

  stack += sprintf(stack,
  "    -==================-  -=================-  -==================-\n\n");


  if (config_flags & cfVEGASLOTS)
    resolve_vegas_slots(p, slot, sw);
  else
    resolve_old_slots(p, slot, sw);

  sprintf(top, "Your %s: %d | Pot value: %d",
	  get_config_msg("cash_name"), p->pennies, pot);
  pstack_mid(top);
  *stack++ = 0;

  tell_player(p, oldstack);
  stack = oldstack;
  return;
}

void view_slots_panels(player * p)
{
  char *oldstack = stack;
  int i, j;

  for (i = 0; i < 4; i++)
  {

    stack += sprintf(stack,
		     "            %-9s           %-9s            %-9s\n"
    "    -==================-  -=================-  -==================-\n",
		     SlotsPics[(i * 3)][0], SlotsPics[(i * 3) + 1][0], SlotsPics[(i * 3) + 2][0]);
    for (j = 1; j < 8; j++)
    {
      stack += sprintf(stack, "   ||%s||%s||%s||\n",
		       SlotsPics[(i * 3)][j], SlotsPics[(i * 3) + 1][j], SlotsPics[(i * 3) + 2][j]);
    }
    stack += sprintf(stack,
		     "    -==================-  -=================-  -==================-\n\n");
  }
  stack = end_string(stack);
  pager(p, oldstack);
  stack = oldstack;
}

void slots_version()
{
  sprintf(stack, " -=*> Slots v1.4 (by astyanax and phypor) enabled.\n");
  stack = strchr(stack, 0);
}
