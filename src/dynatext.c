/*
 * Playground+ - dynatext.c
 * Code for converting dynatext into character dependant strings
 * ---------------------------------------------------------------------------
 *
 * Written by Silver and bug fixed by Blimey (-grin-)
 *  (with few extras added by phypor)
 */

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "include/config.h"
#include "include/player.h"
#include "include/fix.h"
#include "include/proto.h"

/* Interns */
char *the_time(int, int);
char *my_rank(player *);
char *my_partner(player *);
player *get_random_room_player(void);
player *get_random_talker_player(void);

player *this_rand;		/* we preserve the random player so that when you
				   call it once, all other calls in the same thread
				   will be the same player */

char talker_name[80];
char talker_email[80];

void dynatext_version()
{
  sprintf(stack, " -=*> Dynatext v2.0 (by Silver, phypor and blimey) enabled.\n");
  stack = strchr(stack, 0);
}

char *convert_dynatext(player * p, char *str)
{
  player *target = (player *) NULL;
  int z = (p->tag_flags & TAG_DYNATEXT);
  char *oldstack = stack;
  char *scan;

  /* Check there is no dynatext in the string being sent first or that
     people don't want them */

  if (!strchr(str, '&') || p->custom_flags & NO_DYNATEXT)
    return str;

  /* Parse the string looking for (case sensitive) dynatext matches */

  scan = str;
  while (*scan)
  {
    if (*scan == '&')
    {
      if (z)
      {
	strcpy(stack, "{");
	stack = strchr(stack, 0);
      }
      switch (*(scan + 1))
      {
	  /* these are non player specfic */
	case 'T':
	  sprintf(stack, "%s %d", talker_alpha, active_port);
	  break;
	case 't':
	  sprintf(stack, "%s", talker_name);
	  break;
	case 'e':
	  sprintf(stack, "%s", talker_email);
	  break;
	case 'j':
	  sprintf(stack, "%d", pot);
	  break;

	  /* ahh, we have a player */
	case 'm':
	  target = p;
	  break;
	case 'c':
	  target = current_player;
	  break;
	case 'r':
	  target = get_random_room_player();
	  break;
	case 'a':
	  target = get_random_talker_player();
	  break;

	  /* or maybe not */
	case '&':
	  strcpy(stack, "&");
	  break;
	default:		/* If its an unknown char we just print it as normal */
	  sprintf(stack, "&%c", *(scan + 1));
	  break;
      }
      if (target && target->lower_name[0] &&
	  *(scan + 2) == '-' && *(scan + 3) == '>')
      {
	switch (*(scan + 4))
	{
	  case 'n':
	    strcpy(stack, target->name);
	    break;
	  case 'f':
	    strcpy(stack, time_diff(target->jetlag));
	    break;
	  case 'h':
	    sprintf(stack, "%s", the_time(target->jetlag, 1));
	    break;
	  case 'm':
	    sprintf(stack, "%s", the_time(target->jetlag, 2));
	    break;
	  case 's':
	    sprintf(stack, "%s", the_time(target->jetlag, 3));
	    break;
	  case 'w':
	    sprintf(stack, "%s", the_time(target->jetlag, 4));
	    break;
	  case 'o':
	    sprintf(stack, "%s", the_time(target->jetlag, 5));
	    break;
	  case 'd':
	    sprintf(stack, "%s", the_time(target->jetlag, 6));
	    break;
	  case 'y':
	    sprintf(stack, "%s", the_time(target->jetlag, 7));
	    break;
	  case 'r':
	    sprintf(stack, "%s", my_rank(target));
	    break;
	  case 'c':
	    sprintf(stack, "%d", target->pennies);
	    break;
	  case 'g':
	    sprintf(stack, "%s", gstring_possessive(target));
	    break;
	  case 'G':
	    sprintf(stack, "%s", gstring(target));
	    break;
	  case 'i':
	    sprintf(stack, "%s", get_gender_string(target));
	    break;
	  case 'I':
	    sprintf(stack, "%s", gender_raw(target));
	    break;
	  case 'p':
	    sprintf(stack, "%s", my_partner(target));
	    break;
	  case 'l':
	    sprintf(stack, "%ld.%02ld", target->last_ping / 1000000,
		    (target->last_ping / 10000) % 1000000);
	    break;
	  case 'L':
	    sprintf(stack, "%s", ping_string(target));
	    break;
	  default:
	    sprintf(stack, "%s", get_config_msg("bad_dynatext"));	/* they arsed up the thinger */
	    /*scan--;   */
	    break;
	}
	scan += 3;
      }

      else if (*(scan + 2) == '-' && *(scan + 3) == '>')	/* bad target */
      {
	sprintf(stack, "%s", get_config_msg("bad_dynatext"));
	scan += 3;
      }
      else if (target)		/* well, let them get just name if alone */
	strcpy(stack, target->name);
      scan += 2;
      stack = strchr(stack, 0);
      if (z)
      {
	strcpy(stack, "}");
	stack = strchr(stack, 0);
      }
      target = (player *) NULL;
    }
    else
      *stack++ = *scan++;
  }

  *stack++ = 0;

  return oldstack;
}

/* Loads of dynatext options :o) */

/* Time stuff */

char *the_time(int diff, int type)
{
  time_t t;
  static char time_string[20];

  t = time(0) + (3600 * diff);
  switch (type)
  {
    case 1:
      strftime(time_string, 19, "%H", localtime(&t));
      break;
    case 2:
      strftime(time_string, 19, "%M", localtime(&t));
      break;
    case 3:
      strftime(time_string, 19, "%S", localtime(&t));
      break;
    case 4:
      strftime(time_string, 19, "%A", localtime(&t));
      break;
    case 5:
      strftime(time_string, 19, "%B", localtime(&t));
      break;
    case 6:
      strftime(time_string, 19, "%d", localtime(&t));
      break;
    case 7:
      strftime(time_string, 19, "%Y", localtime(&t));
      break;
  }
  return time_string;
}

/* Rank */

char *my_rank(player * p)
{
  /* This isn't a case simply becuase it seemed to ignore the ranks and
     print "newbie" regardless - bad programing I know -sigh- */
  /* tell the truth, unless you implemented an array to do it, 
     this is the best way your gonna get ... -phy */
  if (p->residency & CODER)
    return get_config_msg("coder_name");
  else if (p->residency & HCADMIN)
    return get_config_msg("hc_name");
  else if (p->residency & ADMIN)
    return get_config_msg("admin_name");
  else if (p->residency & LOWER_ADMIN)
    return get_config_msg("la_name");
  else if (p->residency & ASU)
    return get_config_msg("asu_name");
  else if (p->residency & SU)
    return get_config_msg("su_name");
  else if (p->residency & PSU)
    return get_config_msg("psu_name");
  else if (p->residency)
    return "resident";
  else
    return "newbie";
}

/* Returns the players 'other-half' */

char *my_partner(player * p)
{
  if (p->system_flags & (MARRIED | ENGAGED))
    return (p->married_to);
  else
    return "no-one";
}

player *get_random_room_player(void)
{
  player *scan;
  int c = 0;

  if (this_rand)
    return this_rand;

  if (!current_player)
    return (player *) NULL;

  if (!current_player->location)
    return current_player;

  for (scan = current_player->location->players_top; scan;
       scan = scan->room_next, c++);

  c = rand() % c;

  for (scan = current_player->location->players_top; scan && c;
       scan = scan->room_next, c--);

  this_rand = scan;
  return scan;
}

player *get_random_talker_player(void)
{
  player *scan;
  int i;

  if (this_rand)
    return this_rand;
  if (!current_player)
    return (player *) NULL;

  if (!current_players || !current_player->location)
    return current_player;
  i = (rand() % current_players);

  for (scan = flatlist_start; i; scan = scan->flat_next, i--);

  this_rand = scan;
  return scan;
}
