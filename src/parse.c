/*
 * Playground+ - parse.c
 * Command parsing code, timer code and more
 * ---------------------------------------------------------------------------
 */

#include <ctype.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <memory.h>
#ifndef BSDISH
#include <malloc.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include "include/config.h"
#include "include/player.h"
#include "include/fix.h"
#include "include/clist.h"
#include "include/proto.h"


/* varibles */
struct command *last_com;
char *stack_check;
int nsync = 0, synct = 0, sync_counter = 0, note_sync = NOTE_SYNC_TIME;
int account_wobble = 1;
int performance_timer = 0;
struct command *help_list = 0;
file help_file =
{0, 0};
int shutdown_count = -1;
int comm_match = 0;
int news_sync = NEWS_SYNC_INTERVAL;
int reload_count = 5;
int backup_hour;

/* interns */
void spam_warning(player *);
void show_clock(player *);
extern char *sys_time_prompt(int diff);

/* returns the first char of the input buffer */

char *first_char(player * p)
{
  char *scan;
  scan = p->ibuffer;
  while (*scan && isspace(*scan))
    scan++;
  return scan;
}

/* what happens if bad stack detected */

void bad_stack(void)
{
  int missing;
  missing = (int) stack - (int) stack_check;
  if (last_com)
    sprintf(stack_check, "Bad stack in function %s, missing %d bytes",
	    last_com->text, missing);
  else
    sprintf(stack_check, "Bad stack somewhere, missing %d bytes", missing);
  stack = end_string(stack_check);
  log("stack", stack_check);
  stack = stack_check;
}


/* flag changing routines */


/* returns the value of a flag from the flag list */

int get_flag(flag_list * list, char *str)
{
  for (; list->text; list++)
    if (!strcmp(list->text, str))
      return list->change;
  return 0;
}

int get_case_flag(flag_list * list, char *str)
{
  for (; list->text; list++)
    if (!strcasecmp(list->text, str))
      return list->change;
  return 0;
}

char *get_flag_name(flag_list * list, char *str)
{
  for (; list->text; list++)
    if (!strcasecmp(list->text, str))
      return list->text;
  return 0;
}

/* routine to get the next part of an arg */

char *next_space(char *str)
{
  while (*str && *str != ' ')
    str++;
  if (*str == ' ')
  {
    while (*str == ' ')
      str++;
    str--;
  }

  return str;
}



/* view command lists */



int command_prescan(player * p, char *str)
{
  char *oldstack = stack;

  if (!*str || !strcasecmp(str, "?") || !strcasecmp(str, "help"))
  {
    oldstack = stack;
    sprintf(stack, " Format: commands [a-z|all|comm|move|desc|info]\n"
	    "                  [item|sys|");
    stack = strchr(stack, 0);

    if (social_index >= 0)
      stack += sprintf(stack, "soc|");

    stack = strchr(stack, 0);
    if (p->residency & LIST)
    {
      strcpy(stack, "list|");
      stack = strchr(stack, 0);
    }
    if (p->residency & BUILD)
    {
      strcpy(stack, "room|");
      stack = strchr(stack, 0);
    }
    if (p->residency & (PSU | SU))
    {
      strcpy(stack, "su|");
      stack = strchr(stack, 0);
    }
    if (p->residency & (ADC | LOWER_ADMIN | ADMIN))
    {
      strcpy(stack, "ad|");
      stack = strchr(stack, 0);
    }
    strcpy(stack, "misc]\n");
    stack = end_string(stack);
    tell_player(p, oldstack);
    stack = oldstack;
    return -1;
  }
/* ok check for what area was inputted, and return appropriate value. */

  if (!strcasecmp(str, "all"))
    return 0;			/* do all commands =) */
  if (!strcasecmp(str, "comm") || !strcasecmp(str, "talk"))
    return COMMc;
  if (p->residency & LIST && (!strcasecmp(str, "list")))
    return LISTc;
  if (p->residency & BUILD && (!strcasecmp(str, "room")))
    return ROOMc;
  if (!strcasecmp(str, "go") || !strcasecmp(str, "move"))
    return MOVEc;
  if (!strcasecmp(str, "item"))
    return ITEMc;
  if (!strcasecmp(str, "info"))
    return INFOc;
  if (!strcasecmp(str, "sys") || !strcasecmp(str, "toggles"))
    return SYSc;
  if (!strcasecmp(str, "desc") || !strcasecmp(str, "personalize") || !strcasecmp(str, "personalise"))
    return DESCc;
  if ((!strcasecmp(str, "soc") || !strcasecmp(str, "social")) &&
      social_index >= 0)
    return SOCIALc;
  if (!strcasecmp(str, "misc"))
    return MISCc;
  if (p->residency & PSU &&
      (!strcasecmp(str, "su") || !strcasecmp(str, "super")))
    return SUPERc;
  if (p->residency & (LOWER_ADMIN | ADMIN | ADC) &&
      (!strcasecmp(str, "ad") || !strcasecmp(str, "admin")))
    return ADMINc;

  if (config_flags & cfNOSPAM)
  {
    if (p->residency & (LOWER_ADMIN | ADMIN | ADC) &&
	(!strcasecmp(str, "spam")))
      return SPAMc;
  }

  return THE_EVIL_Q;		/* a stupid return yes */
}

void view_commands(player * p, char *str)
{
  struct command *comlist;
  char *oldstack, middle[80];
  char *plyr;
  int s;
  int noc = 0, len = 0, l;
  int choice;
  player *p2 = 0;

  plyr = next_space(str);
  *plyr++ = 0;

  if (*plyr && p->residency & ADMIN)
  {
    p2 = find_player_absolute_quiet(plyr);
    if (!p2)
    {
      tell_player(p, " That player is not logged in right now ...\n");
      return;
    }
  }

  if (strlen(str) == 1)		/* all commands beginning with ... */
  {
    lower_case(str);
    if (!isalpha(*str))
    {
      (void) command_prescan(p, "help");
      return;
    }
    l = *str - 'a';
    if (l < 0 || l > 25)
    {
      (void) command_prescan(p, "help");
      return;
    }
    l++;
    oldstack = stack;
    sprintf(middle, "Commands beginning with '%c'", toupper(*str));
    pstack_mid(middle);
    sprintf(stack, "\n  ");
    stack = strchr(stack, 0);
    for (comlist = coms[l]; comlist->text; comlist++)
    {
      if ((!comlist->level || (p->residency & comlist->level)) &&
	  (!comlist->andlevel || (p->residency & comlist->andlevel)) && !(comlist->section & INVISc))
      {
	if (len + strlen(comlist->text) > 70)
	{
	  sprintf(stack, "\n  ");
	  stack = strchr(stack, 0);
	  len = 2;
	}
	sprintf(stack, "%s ", comlist->text);
	stack = strchr(stack, 0);
	len += (strlen(comlist->text) + 1);	/* +1 for the space */
	noc++;
      }
    }

    sprintf(stack, "\n\n");
    stack = strchr(stack, 0);

    sprintf(middle, "There are %d commands available to you", noc);
    pstack_mid(middle);
    *stack++ = 0;
    pager(p, oldstack);
    stack = oldstack;
    return;
  }

  choice = command_prescan(p, str);

  oldstack = stack;

  switch (choice)
  {
    case -1:
      return;
      break;
    case THE_EVIL_Q:
      tell_player(p, " That area not found. Type commands ? to list valid areas.\n");
      return;
      break;
    case 0:
      pstack_mid("Complete set of commands");
      break;
    case LISTc:
      pstack_mid("List commands");
      break;
    case COMMc:
      pstack_mid("Communication commands");
      break;
    case ROOMc:
      pstack_mid("Room commands");
      break;
    case MOVEc:
      pstack_mid("Movement commands");
      break;
    case ITEMc:
      pstack_mid("Item commands");
      break;
    case INFOc:
      pstack_mid("Info commands");
      break;
    case SYSc:
      pstack_mid("System toggle commands");
      break;
    case DESCc:
      pstack_mid("Personalisation commands");
      break;
    case SOCIALc:
      pstack_mid("Social commands");
      break;
    case MISCc:
      pstack_mid("Misc commands");
      break;
    case SUPERc:
      sprintf(middle, "%s commands", get_config_msg("staff_name"));
      pstack_mid(middle);
      break;
    case ADMINc:
      sprintf(middle, "%s commands", get_config_msg("admin_name"));
      pstack_mid(middle);
      break;
    case SPAMc:		/* leave un cased, i mean how do we get HERE if its not on? */
      pstack_mid("Commands which trigger spam code");
      break;
  }

  if (*plyr && p->residency & ADMIN)
    ADDSTACK("\n [for %s]\n\n", p2->name);
  else
  {
    ADDSTACK("\n");
    p2 = p;
  }

  ADDSTACK("  ");

  for (s = 0; s < 27; s++)
  {
    for (comlist = coms[s]; comlist->text; comlist++)
    {
      if ((!comlist->level || ((p2->residency) & comlist->level)) &&
	  (!comlist->andlevel || ((p2->residency) & comlist->andlevel))
	  && ((!choice || comlist->section & choice) && !(comlist->section & INVISc)))
      {
	if (len + strlen(comlist->text) > 70)
	{
	  ADDSTACK("\n  ");
	  stack = strchr(stack, 0);
	  len = 2;
	}
	ADDSTACK("%s ", comlist->text);
	len += (strlen(comlist->text) + 1);	/* +1 for the space */
	noc++;
      }
    }
  }
  if (choice == SOCIALc)
    noc += list_socials(p);

  ADDSTACK("\n\n");

  if (*plyr && p->residency & ADMIN)
    sprintf(middle, "There are %d commands listed here available to %s", noc, p2->name);
  else
    sprintf(middle, "There are %d commands listed here available to you", noc);
  pstack_mid(middle);
  *stack++ = 0;

  pager(p, oldstack);
  stack = oldstack;


}


void view_sub_commands(player * p, struct command *comlist)
{
  char *oldstack, middle[80];
  int noc = 0, len = 0;

  oldstack = stack;

  pstack_mid("Available sub commands");
  sprintf(stack, "\n");
  stack = strchr(stack, 0);

  for (; comlist->text; comlist++)
    if (((!comlist->level) || ((p->residency) & (comlist->level))) &&
	((!comlist->andlevel) || ((p->residency) & (comlist->andlevel))))
    {
      sprintf(stack, "%s ", comlist->text);
      stack = strchr(stack, 0);
      len += strlen(comlist->text);
      len++;
      if (len > 60)
      {
	sprintf(stack, "\n  ");
	len = 0;
      }
      noc++;
    }
  ADDSTACK("\n\n");

  sprintf(middle, "There are %d commands listed here", noc);
  pstack_mid(middle);
  *stack++ = 0;

  tell_player(p, oldstack);
  stack = oldstack;
}

/* initialise the hash array */

void init_parser()
{
  int i;
  struct command *scan;
  scan = complete_list;
  for (i = 0; i < 27; i++)
  {
    coms[i] = scan;
    while (scan->text)
      scan++;
    scan++;
  }
}

/* see if any commands fit the bill */

#ifndef COM_MATCH
char *do_match(char *str, struct command *com_entry)
{
  char *t;
  for (t = com_entry->text; *t; t++, str++)
    if (tolower(*str) != *t)
      return 0;
  if ((com_entry->space) && (*str) && (!isspace(*str)))
    return 0;
  while (*str && isspace(*str))
    str++;
  return str;
}
#else
char *do_match_old(char *str, struct command *com_entry)
{
  char *t;
  for (t = com_entry->text; *t; t++, str++)
    if (tolower(*str) != *t)
      return 0;
  if ((com_entry->space) && (*str) && (!isspace(*str)))
    return 0;
  while (*str && isspace(*str))
    str++;
  return str;
}

char *do_match(char *str, struct command *com_entry)
{
  char *scan;

  for (scan = com_entry->text; *scan && *str && !isspace(*str); scan++, str++)
    if (tolower(*str) != *scan)
      return 0;

  if ((com_entry->space) && (*str) && (!isspace(*str)))
    return 0;

  if (*scan)
  {
    if (com_entry->section & (NOMATCHc | INVISc))
      return 0;
    com_entry->section |= TAGGEDc;
    comm_match++;
    return 0;
  }

  /* while we are on spaces */
  while (*str && isspace(*str))
    str++;
  return str;
}
#endif

void clear_comlist_tags(struct command *comlist)
{
  if (!comm_match)
    return;
  while (comlist->text)
  {
    comlist->section &= ~TAGGEDc;
    comlist++;
  }
  comm_match = 0;
}

void execute_command(player * p, struct command *com, char *str)
{
  void (*fn) (player *, char *);

  if (str[strlen(str) - 1] != '^')
  {
    last_com = com;
    stack_check = stack;
    sys_flags &= ~ROOM_TAG;
    command_type &= ~ROOM;
    fn = com->function;
    (*fn) (p, str);
    if (stack != stack_check)
      bad_stack();
  }
  else
    TELLPLAYER(p, " '%s' command cancelled.\n", p->command_used->text);

  sys_flags &= ~(FAILED_COMMAND | PIPE | ROOM_TAG | FRIEND_TAG | OFRIEND_TAG | EVERYONE_TAG);
  command_type = 0;
}


/* execute a function from a sub command list */

void sub_command(player * p, char *str, struct command *comlist)
{
  struct command *comliststart;
  char *oldstack = stack, *rol, *space;

  /* Lowercase string here, line in case the person was rm_capped */
  if (p->system_flags & DECAPPED)
    lower_case(str);

  comliststart = comlist;

  while (comlist->text)
  {
    if (((!comlist->level) || ((p->residency) & (comlist->level))) &&
	((!comlist->andlevel) || ((p->residency) & (comlist->andlevel))))
    {
      rol = do_match(str, comlist);
      if (rol)
      {
	execute_command(p, comlist, rol);
	clear_comlist_tags(comliststart);
	return;
      }
    }
    comlist++;
  }

  if (comm_match)
  {
    if (comm_match == 1)	/* single match */
    {
      for (comlist = comliststart; comlist->text && !(comlist->section & TAGGEDc); comlist++)
	;
      for (space = comlist->text, rol = str; *space && *rol && !isspace(*rol); rol++)
	;
      while (*rol && isspace(*rol))
	rol++;
      comlist->section &= ~TAGGEDc;
      comm_match = 0;
      execute_command(p, comlist, rol);
    }
    else
    {
      strcpy(oldstack, " Multiple sub command matches: ");
      stack = strchr(stack, 0);
      for (comlist = comliststart; comlist->text; comlist++)
	if (comlist->section & TAGGEDc)
	{
	  comlist->section &= ~TAGGEDc;
	  sprintf(stack, "%s ", comlist->text);
	  stack = strchr(stack, 0);
	}
      stack--;
      strcpy(stack, ".\n");
      stack = end_string(stack);
      tell_player(p, oldstack);
      stack = oldstack;
      comm_match = 0;
    }
    return;
  }

  rol = str;
  while (*rol && !isspace(*rol))
    rol++;
  *rol = 0;
  if (p->location == prison)
    sprintf(oldstack, " They don't let you do that kinda thing down here.\n");
  else
#ifndef COM_MATCH
    sprintf(oldstack, " Cannot find sub command '%s'\n", str);
#else
    sprintf(oldstack, " Cannot match sub command '%s'\n", str);
#endif
  stack = end_string(oldstack);
  tell_player(p, oldstack);
  stack = oldstack;
}


/* match commands to the main command lists */

void old_match_commands(player * p, char *str)
{
  struct command *comlist, *comliststart;
  char *rol, *oldstack, *space;
  oldstack = stack;

  while (*str && *str == ' ')
    str++;
  space = strchr(str, 0);
  space--;
  while (*space == ' ')
    *space-- = 0;
  if (!*str)
    return;

  /* Lowercase string here, line in case the person was rm_capped */
  if (p->system_flags & DECAPPED)
    lower_case(str);

  if (isalpha(*str))
    comlist = coms[((int) (tolower(*str)) - (int) 'a' + 1)];
  else
    comlist = coms[0];

  comliststart = comlist;

  while (comlist->text)
  {
    if (((!comlist->level) || ((p->residency) & (comlist->level))) &&
	((!comlist->andlevel) || ((p->residency) & (comlist->andlevel))))
    {
      rol = do_match(str, comlist);
      if (rol)
      {
#ifdef COMMAND_STATS
	if (!(comlist->section & (SUPERc | ADMINc)))
#ifdef ROBOTS
	  if (!(comlist->section & (SUPERc | ADMINc)) && !(p->residency & ROBOT_PRIV))
#endif /* robots */
	    commandUsed(comlist->text);
#endif /* command_stats */

	if (config_flags & cfNOSPAM)
	  if (comlist->section & SPAMc)
	  {
	    if (last_com == comlist)
	      p->antispam += atoi(get_config_msg("spam_repeat"));
	    else
	      p->antispam += atoi(get_config_msg("spam_different"));
	  }

	p->command_used = comlist;

	if (!strncasecmp(rol, "/?", 2) || !strncasecmp(rol, "-h", 2))
	  help(p, p->command_used->text);
	else if (config_flags & cfNOSWEAR)
	{
	  if ((comlist->section & F_SWEARc) ||
	      ((comlist->section & M_SWEARc) &&
	       (!strcmp(p->location->owner->lower_name, SYS_ROOM_OWNER) ||
		!strcmp(p->location->owner->lower_name, "intercom"))))
	    execute_command(p, comlist, filter_rude_words(rol));
	  else
	    execute_command(p, comlist, rol);
	}
	else
	  execute_command(p, comlist, rol);
	clear_comlist_tags(comliststart);
	return;
      }
    }
    comlist++;
  }

  /* check for taggings */
  if (comm_match)
  {
    if (comm_match == 1)
    {
      for (comlist = comliststart; comlist->text && !(comlist->section & TAGGEDc); comlist++)
	;
      for (space = comlist->text, rol = str; *space && *rol && !isspace(*rol); rol++)
	;
      while (*rol && isspace(*rol))
	rol++;
      comlist->section &= ~TAGGEDc;
      comm_match = 0;
#ifdef COMMAND_STATS
      if (!(comlist->section & (SUPERc | ADMINc)))
#ifdef ROBOTS
	if (!(comlist->section & (SUPERc | ADMINc)) && !(p->residency & ROBOT_PRIV))
#endif /* robots */
	  commandUsed(comlist->text);
#endif /* command_stats */
  
        if (config_flags & cfNOSPAM)
          if (comlist->section & SPAMc)
          {
            if (last_com == comlist)
              p->antispam += atoi(get_config_msg("spam_repeat"));
            else
              p->antispam += atoi(get_config_msg("spam_different"));
          }

        p->command_used = comlist;

        if (!strncasecmp(rol, "/?", 2) || !strncasecmp(rol, "-h", 2))
          help(p, p->command_used->text);
        else
      execute_command(p, comlist, rol);

    }
    else
    {
      strcpy(oldstack, " Multiple command matches: ");
      stack = strchr(stack, 0);
      for (comlist = comliststart; comlist->text; comlist++)
	if (comlist->section & TAGGEDc)
	{
	  comlist->section &= ~TAGGEDc;
	  sprintf(stack, "%s ", comlist->text);
	  stack = strchr(stack, 0);
	}
      stack--;
      strcpy(stack, ".\n");
      stack = end_string(stack);
      tell_player(p, oldstack);
      stack = oldstack;
      comm_match = 0;
    }
    return;
  }

  rol = str;
  while (*rol && !isspace(*rol))
    rol++;
  *rol = 0;
#ifndef COM_MATCH
  sprintf(oldstack, " Cannot find command '%s'\n", str);
#else
  sprintf(oldstack, " Cannot match command '%s'\n", str);
#endif
  stack = end_string(oldstack);
  tell_player(p, oldstack);
  stack = oldstack;
}

void match_commands(player * p, char *str)
{

  char *rol, holder[1000];
  alias *al;

  if (!(p->saved))
  {
    old_match_commands(p, str);
    return;
  }
  /* lets try an alias match first */
  rol = do_alias_match(p, str);
  if (strcmp(rol, "\n"))
  {
    al = get_alias(p, str);
    if (!al)
    {
      tell_player(p, " Alias matching error\n");
      return;
    }
    /* NEED TO LOOP THROUGH HERE */
    strcpy(holder, splice_argument(p, al->sub, rol, 0));
    while (holder[0])
    {
      if (p->system_flags & DECAPPED)
	lower_case(holder);
      old_match_commands(p, holder);
      strcpy(holder, splice_argument(p, al->sub, rol, 1));
    }
  }
  else
    old_match_commands(p, str);
}

/* handle input from one player */

void input_for_one(player * p)
{
  char *pick;

  this_rand = (player *) NULL;	/* dynatext random player reset */
  if (p->input_to_fn)
  {
    p->idle = 0;
    p->idle_index = 0;
    p->idle_msg[0] = 0;
    last_com = &input_to;
    stack_check = stack;
    sys_flags &= ~ROOM_TAG;
    command_type &= ~ROOM;
    (*p->input_to_fn) (p, p->ibuffer);
    if (stack != stack_check)
      bad_stack();
    sys_flags &= ~(FAILED_COMMAND | PIPE | ROOM_TAG | FRIEND_TAG | OFRIEND_TAG | EVERYONE_TAG);
    command_type = 0;
    return;
  }
  if (!p->ibuffer[0])
    return;
  p->idle = 0;
  p->idle_index = 0;
  p->idle_msg[0] = 0;
  action = "doing command";
  if (p->ibuffer[0] != '#')
  {
    if (p->custom_flags & CONVERSE)
    {
      pick = p->ibuffer;
      while (*pick && isspace(*pick))
	pick++;
      if (*pick)
	if (*pick == '/' || *pick == '.')
	  if (current_room == prison && !(p->residency & (ADMIN | SU)))
	    sub_command(p, pick + 1, restricted_list);
	  else
	  {
	    if (!match_social(p, pick + 1))
	      match_commands(p, pick + 1);
	  }
	else if (!isalpha(*pick))
	  if (current_room == prison && !(p->residency & (ADMIN | SU)))
	    sub_command(p, pick, restricted_list);
	  else
	  {
	    if (!match_social(p, pick))
	      match_commands(p, pick);
	  }
	else
	  say(p, pick);
    }
    else if (current_room == prison && !(p->residency & (ADMIN | SU)))
      sub_command(p, p->ibuffer, restricted_list);
    else
    {
      if (!match_social(p, p->ibuffer))
	match_commands(p, p->ibuffer);
    }
  }
}

void su_quit_log(player * p)
{

  char *oldstack;
  int csu;

  csu = true_count_su();
  oldstack = stack;
  sprintf(stack, "%s leaves -- %d sus left", p->name, csu - 1);
  stack = end_string(stack);
  log("su", oldstack);
  stack = oldstack;
}

/* scan through the players and see if anything needs doing */

void process_players()
{
  player *scan, *sparky;
  char *oldstack, *hasta, thetime[10];

  if (current_players > max_ppl_on_so_far)
    max_ppl_on_so_far = current_players;
  for (scan = flatlist_start; scan; scan = sparky)
  {
    sparky = scan->flat_next;
    if (scan->flat_next)
      if (((player *) scan->flat_next)->flat_previous != scan)
      {
	raw_wall("\n\n   -=> Non-terminated flatlist <=-\n\n");
	raw_wall("\n\n   -=> Dumping end off of list <=-\n\n");
	scan->flat_next = NULL;
      }
#ifdef ROBOTS
    if ((scan->fd < 0 && !(scan->flags & ROBOT)) || (scan->flags & PANIC) || (scan->flags & CHUCKOUT))
#else
    if ((scan->fd < 0) || (scan->flags & PANIC) || (scan->flags & CHUCKOUT))
#endif
    {

      oldstack = stack;
      current_player = scan;

      if (scan->location && scan->name[0] && !(scan->flags & RECONNECTION))
      {
#ifdef LAST
	stampLogout(scan->name);
#endif
	if (scan->residency & SU && true_count_su() <= 1)
	  su_quit_log(scan);

	hasta = do_alias_match(scan, "_logoff");
	if (strcmp(hasta, "\n"))
	{
	  match_commands(scan, "_logoff");
	}
	if (scan == p_sess)
	{
	  session_reset = 0;
	}
	if (strlen(scan->logoffmsg) < 1)
	{
	  stack += sprintf(stack, "%s %s ",
			   get_config_msg("logoff_prefix"), scan->name);
	  stack += sprintf(stack, "%s ", get_config_msg("def_logout"));
	  stack += sprintf(stack, "%s\n", get_config_msg("logoff_suffix"));
	}
	else
	{
	  if (emote_no_break(*scan->logoffmsg))
	  {
	    stack += sprintf(stack, "%s %s%s ", get_config_msg("logoff_prefix"),
			     scan->name, scan->logoffmsg);
	    stack += sprintf(stack, "%s\n", get_config_msg("logoff_suffix"));
	  }
	  else
	  {
	    stack += sprintf(stack, "%s %s %s ", get_config_msg("logoff_prefix"),
			     scan->name, scan->logoffmsg);
	    stack += sprintf(stack, "%s\n", get_config_msg("logoff_suffix"));
	  }
	}

	stack = end_string(stack);
	command_type |= LOGOUT_TAG;
	tell_room(scan->location, oldstack);
	command_type &= ~LOGOUT_TAG;
	stack = oldstack;
	save_player(scan);
	command_type = 0;
	do_inform(scan, get_plists_msg("inform_logout"));
	if (scan->saved && !(scan->flags & NO_SAVE_LAST_ON))
	  scan->saved->last_on = time(0);
      }
      if (sys_flags & VERBOSE || scan->residency == 0)
      {
	/* Ignore "advanced" talker listings which spam the newconn log */
        if (!unlogged_site(scan->inet_addr) && !unlogged_site(scan->num_addr))
	{
	  if (scan->name[0])
	  {
	    if (scan->newbieinform)
	      SUWALL(" -=*> '%s' has disconnected (non-resident)\n", scan->name);
	    LOGF("newconn", "%s disconnects from %s", scan->name,
		 get_address(scan, NULL));
	  }
	  else
	    LOGF("nonconn", "%s disconnects from login", scan->inet_addr);
	}
      }
      destroy_player(scan);
      current_player = 0;
      stack = oldstack;
    }
    else if (scan->flags & INPUT_READY)
    {
      if (!(scan->lagged) && !(scan->system_flags & SAVE_LAGGED))
      {
	current_player = scan;
	current_room = scan->location;
	input_for_one(scan);
	action = "processing players";
	current_player = 0;
	current_room = 0;
	sys_color_atm = SYSsc;
      }
      if (scan->flags & PROMPT)
      {
	if (scan->custom_flags & CONVERSE)
	  do_prompt(scan, scan->converse_prompt);
	else if ((scan->system_flags & TIMEPROMPT) && !(scan->custom_flags & CONVERSE))
	{
	  sprintf(thetime, "[%s]> ", sys_time_prompt(scan->jetlag));
	  do_prompt(scan, thetime);
	}
	else if (!(scan->system_flags & TIMEPROMPT))
	  do_prompt(scan, scan->prompt);
      }
      memset(scan->ibuffer, 0, IBUFFER_LENGTH);
      scan->flags &= ~INPUT_READY;
    }
  }
}




/* timer things */


/* automessages */

void do_automessage(room * r)
{
  int count = 0, type;
  char *scan, *oldstack;
  oldstack = stack;
  scan = r->automessage.where;
  if (!scan)
  {
    r->flags &= ~AUTO_MESSAGE;
    return;
  }
  for (; *scan; scan++)
    if (*scan == '\n')
      count++;
  if (!count)
  {
    FREE(r->automessage.where);
    r->automessage.where = 0;
    r->automessage.length = 0;
    r->flags &= ~AUTO_MESSAGE;
    stack = oldstack;
    return;
  }
  count = rand() % count;
  for (scan = r->automessage.where; count; count--, scan++)
    while (*scan != '\n')
      scan++;
  while (*scan != '\n')
    *stack++ = *scan++;
  *stack++ = '\n';
  *stack++ = 0;
  type = command_type;
  command_type = AUTO;
  tell_room(r, oldstack);
  command_type = type;
  r->auto_count = r->auto_base + (rand() & 63);
  stack = oldstack;
}


/* file syncing */

void do_sync(void)
{
  int origin;
  action = "doing sync";
  sync_counter = SYNC_TIME;
  origin = synct;
  while (!update[synct])
  {
    synct = (synct + 1) % 26;
    if (synct == origin)
      break;
  }
  if (update[synct])
  {
    sync_to_file(synct + 'a', 0);
    synct = (synct + 1) % 26;
  }
}

/* this is the actual timer pling */

void actual_timer(int c)
{
  static int pling = TIMER_CLICK;
  player *scan;
  time_t t;
  int timeon;

  if (sys_flags & PANIC)
    return;

#if !defined(hpux) && !defined(linux)
  if ((int) signal(SIGALRM, actual_timer) < 0)
    handle_error("Can't set timer signal.");
#endif /* hpux */

  t = time(0);
  if ((splat_timeout - t) <= 0)
    splat1 = splat2 = 0;
  pling--;
  if (pling)
    return;

  pling = TIMER_CLICK;

  sys_flags |= DO_TIMER;

  /* if (mem_use_log_count > 0)
     mem_use_log_count--; */
  if (shutdown_count > 0)
    shutdown_count--;

#ifdef ROBOTS
  process_robot_counters();
#endif

#ifdef ALLOW_MULTIS
  update_multis();
#endif
  this_rand = (player *) NULL;	/* dynatext random player reset, here too, 
				   in case dynatext is not run from a command */

  for (scan = flatlist_start; scan; scan = scan->flat_next)
    if (!(scan->flags & PANIC))
    {
      if (scan->next_ping == 0)
      {
	scan->next_ping = -1;
	ping_timed(scan);
      }
      if (scan->next_ping > 0)
	scan->next_ping--;

      scan->idle++;
      scan->idle_index++;
      scan->total_login++;
      if (scan->total_login % ONE_HOUR == 0)
	scan->pennies += 10;
      if (scan->pennies > 100000)
	scan->pennies = 100000;
      if (scan->residency && !(scan->residency & NO_SYNC) && scan->total_login % 1200 == 800)
	save_player(scan);
      if (scan->location && !strcmp(scan->location->owner->lower_name, SYS_ROOM_OWNER))
	scan->time_in_main++;
      if (scan->script && scan->script > 1)
	scan->script--;
      if (scan->timer_fn && scan->timer_count > 0)
	scan->timer_count--;
      if (scan->no_shout > 0)
	scan->no_shout--;
      if (scan->no_move > 0 && !(scan->system_flags & SAVED_RM_MOVE))
	scan->no_move--;
      if (scan->lagged > 0)
	scan->lagged--;
      if (scan->shout_index > 0)
	scan->shout_index--;
      if (scan->social_index)
	scan->social_index--;

/* for sing */
      if (scan->no_sing > 0)
	scan->no_sing--;

/* for clock */
      if (time(0) % ONE_HOUR == 0 && !(scan->custom_flags & NO_CLOCK))
	show_clock(scan);

/* For max login time */
      timeon = time(0) - scan->on_since;
#ifdef ROBOTS
      if (scan->location && timeon > longest_time && timeon < ONE_DAY && !(scan->residency & ROBOT_PRIV))
#else
      if (scan->location && timeon > longest_time && timeon < ONE_DAY)
#endif
      {
	longest_time = timeon;
	strncpy(longest_player, scan->name, 20);
      }
/* For slots */
      if (pot <= 150)
	pot = atoi(get_config_msg("initial_pot"));

/* For residency things */
      if (scan->awaiting_residency == 1)
	make_resident(scan);
      else if (scan->awaiting_residency > 0)
      {
	scan->awaiting_residency--;
	if (scan->awaiting_residency == 31 || scan->awaiting_residency == 16)
	{
	  TELLPLAYER(scan, " -=*> You have %d seconds left before you become a resident.\n", scan->awaiting_residency - 1);
	  SUWALL(" -=*> %s will be granted residency in %d seconds ...\n", scan->name, scan->awaiting_residency - 1);
	}
      }
/* for asty's idle thang */
      if (scan->idle_index > 300 && !(scan->residency & ADMIN))
      {
	scan->total_idle_time += (290 + (rand() % 40));
	scan->idle_index = 0;
      }
      if (scan->jail_timeout > 0)
	scan->jail_timeout--;
      /* unidle admins */
      if (config_flags & cfADMINIDLE)
	if (scan->idle > 3000 && scan->residency & ADMIN)
	  scan->idle = 0;

      if (config_flags & cfNOSPAM)
      {
	/* Anti-spam code */
	scan->antispam -= 7;
	if (scan->antispam < 0)
	  scan->antispam = 0;
	if (scan->antispam > 50)
	  spam_warning(scan);
      }

      /* Channel stuff */
      if (strlen(scan->zchannel) > 0)
      {
	scan->zcidle++;
	if (scan->zcidle > ONE_HOUR)
	{
	  if (scan->misc_flags & CHAN_HI)
	    command_type |= HIGHLIGHT;
	  sys_color_atm = UCEsc;
	  TELLPLAYER(scan, " <(%s)> -- This channel is too idle and has been destroyed\n", scan->zchannel);
	  sys_color_atm = SYSsc;
	  if (scan->misc_flags & CHAN_HI)
	    command_type &= ~HIGHLIGHT;
	  strcpy(scan->zchannel, "");
	  scan->zcidle = 0;
	}
      }
      /* timing out REALLY idle gits... */
      if (config_flags & cfIDLEBAD)
      {
	if (scan->idle == 3000 || scan->idle == 3300 ||
	 (!(scan->residency) && (scan->idle == 1500 || scan->idle == 1620)))
	{
	  TELLPLAYER(scan, " -=*> Warning - you are now %d minutes idle.\007\n",
		     scan->idle / ONE_MINUTE);
	}
	if (scan->idle == 3540 || (!(scan->residency) && scan->idle == 1740))
	{
	  TELLPLAYER(scan, " -=*> You're %d minutes idle. 1 minute till auto-disconnect.\007\n",
		     scan->idle / ONE_MINUTE);
	}
	if (scan->idle >= ONE_HOUR || (!(scan->residency) && scan->idle >= (ONE_HOUR / 2)))
	{
	  TELLPLAYER(scan, "\n\n"
		     LINE
		     "\nThank you for visiting %s.\n\n"
		     "Next time though, when you decide to leave, you ought to quit for yourself.\n\n"
		     LINE
		     "\n\n", scan->name);
	  scan->idled_out_count++;
	  log("idle", scan->lower_name);
	  quit(scan, 0);
	}
      }
    }
  net_count--;
  if (!net_count)
  {
    net_count = 10;
    in_total += in_current;
    out_total += out_current;
    in_pack_total += in_pack_current;
    out_pack_total += out_pack_current;
    in_bps = in_current / 10;
    out_bps = out_current / 10;
    in_pps = in_pack_current / 10;
    out_pps = out_pack_current / 10;
    in_average = (in_average + in_bps) >> 1;
    out_average = (out_average + out_bps) >> 1;
    in_pack_average = (in_pack_average + in_pps) >> 1;
    out_pack_average = (out_pack_average + out_pps) >> 1;
    in_current = 0;
    out_current = 0;
    in_pack_current = 0;
    out_pack_current = 0;
  }
}

/* If they've been naughty - then they get kicked off. Heh heh heh --Silver */

void spam_warning(player * p)
{
  int no1, no2, no3, no4, gon;

  gon = atoi(get_config_msg("spam_eject"));
  TELLPLAYER(p, "\n\n"
	     LINE
  "\nYou have triggered the anti-spam code. As well as being pointless and\n"
  "irritating, spamming is also unnecessary and annoys other residents.\n\n"
    "You have been prevented from logging in for the next %d minute(s).\n\n"
	     LINE
	     "\n\n\007", gon);
  LOGF("spam", "%s is ejected for %d min(s) for spamming", p->name, gon);
  SW_BUT(p, " -=*> %s is ejected for %d min(s) for spamming\n", p->name, gon);
  p->sneezed = time(0) + (gon * 60);
  soft_timeout = time(0) + (gon * 60);
  sscanf(p->num_addr, "%d.%d.%d.%d", &no1, &no2, &no3, &no4);
  soft_splat1 = no1;
  soft_splat2 = no2;
  quit(p, "");
}



/* the timer function */

void timer_function()
{
  player *scan, *old_current;
  room *r, **list;
  char *oldstack;
  int count = 0, pcount = 0;
  char *action_cpy;
  struct tm *ts;
  time_t t;

#if !defined(linux)
#ifndef BSDISH
  struct mallinfo minfo;
#endif
#else
  /* struct mstats memstats; */
#endif /* LINUX */

#ifdef AUTOSHUTDOWN
  int nump = 0;
#endif

  if (!(sys_flags & DO_TIMER))
    return;
  sys_flags &= ~DO_TIMER;

  waitpid((pid_t) - 1, (int *) 0, WNOHANG);
  /* wait3(0,WNOHANG,0); */

  old_current = current_player;
  action_cpy = action;

  oldstack = stack;
  if (shutdown_count > -1)
  {
    command_type |= HIGHLIGHT;
    switch (shutdown_count)
    {
      case ONE_YEAR:
	raw_wall("\n\n -=*>           Your attention please.           <*=-\n"
		 " -=*>   We'll be rebooting in exactly one year   <*=-\n"
	       " -=*> Anyone still here at that time needs help! <*=-\n\n");
	break;
      case ONE_DAY:
	sprintf(stack, "\n %s\n\n", get_shutdowns_msg("1day"));
	stack = end_string(stack);
	raw_wall(oldstack);
	stack = oldstack;
	break;
      case (15 * ONE_MINUTE):
	sprintf(stack, "\n %s\n\n", get_shutdowns_msg("15mins"));
	stack = end_string(stack);
	raw_wall(oldstack);
	stack = oldstack;
	break;
      case (10 * ONE_MINUTE):
	sprintf(stack, "\n %s\n\n", get_shutdowns_msg("10mins"));
	stack = end_string(stack);
	raw_wall(oldstack);
	stack = oldstack;
	break;
      case (5 * ONE_MINUTE):
	sprintf(stack, "\n %s\n\n", get_shutdowns_msg("5mins"));
	stack = end_string(stack);
	raw_wall(oldstack);
	stack = oldstack;
	break;
      case (3 * ONE_MINUTE):
	sprintf(stack, "\n %s\n\n", get_shutdowns_msg("3mins"));
	stack = end_string(stack);
	raw_wall(oldstack);
	stack = oldstack;
	break;
      case (2 * ONE_MINUTE):
	sprintf(stack, "\n %s\n\n", get_shutdowns_msg("2mins"));
	stack = end_string(stack);
	raw_wall(oldstack);
	stack = oldstack;
	break;
      case ONE_MINUTE:
	sprintf(stack, "\n %s\n\n", get_shutdowns_msg("1min"));
	stack = end_string(stack);
	raw_wall(oldstack);
	stack = oldstack;
	break;
      case 45:
	sprintf(stack, "\n %s\n\n", get_shutdowns_msg("45secs"));
	stack = end_string(stack);
	raw_wall(oldstack);
	stack = oldstack;
	break;
      case 30:
	sprintf(stack, "\n %s\n\n", get_shutdowns_msg("30secs"));
	stack = end_string(stack);
	raw_wall(oldstack);
	stack = oldstack;
	break;
      case 15:
	sprintf(stack, "\n %s\n\n", get_shutdowns_msg("15secs"));
	stack = end_string(stack);
	raw_wall(oldstack);
	stack = oldstack;
	break;
      case 10:
	sprintf(stack, "\n %s\n\n", get_shutdowns_msg("10secs"));
	stack = end_string(stack);
	raw_wall(oldstack);
	stack = oldstack;
	break;
      case 9:
	sprintf(stack, "\n %s\n", get_shutdowns_msg("9secs"));
	stack = end_string(stack);
	raw_wall(oldstack);
	stack = oldstack;
	break;
      case 8:
	sprintf(stack, "\n %s\n", get_shutdowns_msg("8secs"));
	stack = end_string(stack);
	raw_wall(oldstack);
	stack = oldstack;
	break;
      case 7:
	sprintf(stack, "\n %s\n", get_shutdowns_msg("7secs"));
	stack = end_string(stack);
	raw_wall(oldstack);
	stack = oldstack;
	break;
      case 6:
	sprintf(stack, "\n %s\n", get_shutdowns_msg("6secs"));
	stack = end_string(stack);
	raw_wall(oldstack);
	stack = oldstack;
	break;
      case 5:
	sprintf(stack, "\n %s\n", get_shutdowns_msg("5secs"));
	stack = end_string(stack);
	raw_wall(oldstack);
	stack = oldstack;
	break;
      case 4:
	sprintf(stack, "\n %s\n", get_shutdowns_msg("4secs"));
	stack = end_string(stack);
	raw_wall(oldstack);
	stack = oldstack;
	break;
      case 3:
	sprintf(stack, "\n %s\n", get_shutdowns_msg("3secs"));
	stack = end_string(stack);
	raw_wall(oldstack);
	stack = oldstack;
	break;
      case 2:
	sprintf(stack, "\n %s\n", get_shutdowns_msg("2secs"));
	stack = end_string(stack);
	raw_wall(oldstack);
	stack = oldstack;
	break;
      case 1:
	sprintf(stack, "\n %s\n", get_shutdowns_msg("1sec"));
	stack = end_string(stack);
	raw_wall(oldstack);
	stack = oldstack;
	break;
      case 0:
	log("shutdown", shutdown_reason);
	sys_flags |= SHUTDOWN;
	stack = oldstack;
	return;
    }
    command_type &= ~HIGHLIGHT;
  }
  if (sync_counter)
    sync_counter--;
  else
    do_sync();

  if (note_sync)
    note_sync--;
  else
  {
    note_sync = NOTE_SYNC_TIME;
    sync_notes(1);
  }
  scan_news();

  if (news_sync)
    news_sync--;
  else
  {
    news_sync = NEWS_SYNC_INTERVAL;
    sync_all_news_headers();
  }

  if (reload_count)
    reload_count--;
  else
  {
    reload_count = 5;
    init_messages();
  }

  align(stack);
  list = (room **) stack;

  for (scan = flatlist_start; scan; scan = scan->flat_next)
  {
    if (!(scan->flags & PANIC))
    {
      if (scan->script && scan->script == 1)
	end_emergency(scan);

      if (scan->timer_fn && !scan->timer_count)
      {
	current_player = scan;
	sys_flags &= ~ROOM_TAG;
	command_type &= ~ROOM;
	(*scan->timer_fn) (scan);
	scan->timer_fn = 0;
	scan->timer_count = -1;
      }
      current_player = old_current;
      action = "processing autos";
      r = scan->location;
      if (r)
      {
	pcount++;
	if (r->flags & AUTO_MESSAGE && !(r->flags & AUTOS_TAG))
	{
	  if (!r->auto_count)
	    do_automessage(r);
	  else
	    r->auto_count--;
	  *(room **) stack = r;
	  stack += sizeof(room *);
	  count++;
	  r->flags |= AUTOS_TAG;
	}
      }
/* Jail timeout thang */
      if (scan->jail_timeout == 0 && scan->location == prison)
      {
	command_type |= HIGHLIGHT;
	tell_player(scan, " After serving your sentence you are flung out"
		    " to society again.\n");
	command_type &= ~HIGHLIGHT;
	move_to(scan, sys_room_id(ENTRANCE_ROOM), 0);
      }
    }
  }
  for (; count; count--, list++)
    (*list)->flags &= ~AUTOS_TAG;
  stack = oldstack;
  action = action_cpy;
  current_players = pcount;

  t = time(0);
  ts = localtime(&t);

  if ((backup_hour != -1) && (ts->tm_hour == backup_hour) &&
      !(ts->tm_min) && !(ts->tm_sec))
    do_backup(0, 0);

#ifdef AUTOSHUTDOWN
  if (auto_sd == 1)
  {
    nump = 0;
    for (scan = flatlist_start; scan; scan = scan->flat_next)
#ifdef ROBOTS
      if (!(scan->residency & ROBOT_PRIV))
#endif
	nump++;

    if (nump == 0)
    {
      LOGF("shutdown", "Auto-Shutdown sequence started");
      close_down();
    }
  }
#endif

#ifdef SEAMLESS_REBOOT
  if (awaiting_reboot)
  {
    awaiting_reboot = 0;
    for (scan = flatlist_start; scan; scan = scan->flat_next)
      if (scan->location && (scan->flags & IN_EDITOR || scan->mode & (MAILEDIT | ROOMEDIT | NEWSEDIT)))
	awaiting_reboot = 1;
    if (!awaiting_reboot)
      do_reboot();
  }
#endif

}

/* the help system (aargh argh argh) */


/* look through all possible places to find a bit of help */

struct command *find_help(char *str)
{
  struct command *comlist;
  if (isalpha(*str))
    comlist = coms[((int) (tolower(*str)) - (int) 'a' + 1)];
  else
    comlist = coms[0];

  for (; comlist->text; comlist++)
#ifdef COM_MATCH
    if (do_match_old(str, comlist))
#else
    if (do_match(str, comlist))
#endif
      return comlist;
  comlist = help_list;
  if (!comlist)
    return 0;
  for (; comlist->text; comlist++)
#ifdef COM_MATCH
    if (do_match_old(str, comlist))
#else
    if (do_match(str, comlist))
#endif
      return comlist;

  return 0;
}


void next_line(file * hf)
{
  while (hf->length > 0 && *(hf->where) != '\n')
  {
    hf->where++;
    hf->length--;
  }
  if (hf->length > 0)
  {
    hf->where++;
    hf->length--;
  }
}

void init_help()
{
  file hf;
  struct command *found, *hstart;
  char *oldstack, *start, *scan;
  int length;
  oldstack = stack;


  if (sys_flags & VERBOSE)
    log("boot", "Loading help pages");


  if (help_list)
    FREE(help_list);
  help_list = 0;

  if (help_file.where)
    FREE(help_file.where);
  help_file = load_file("doc/help");
  hf = help_file;

  align(stack);
  hstart = (struct command *) stack;

  while (hf.length > 0)
  {
    while (hf.length > 0 && *(hf.where) != ':')
      next_line(&hf);
    if (hf.length > 0)
    {
      scan = hf.where;
      next_line(&hf);
      *scan++ = 0;
      while (scan != hf.where)
      {
	start = scan;
	while (*scan != ',' && *scan != '\n')
	  scan++;
	*scan++ = 0;
	found = find_help(start);
	if (!found)
	{
	  found = (struct command *) stack;
	  stack += sizeof(struct command);
	  found->text = start;
	  found->function = 0;
	  found->level = 0;
	  found->andlevel = 0;
	  found->space = 1;
	  found->section = 0;
	}
	found->help = hf.where;
      }
    }
  }
  *(hf.where - 1) = 0;
  found = (struct command *) stack;
  stack += sizeof(struct command);
  found->text = 0;
  found->function = 0;
  found->level = 0;
  found->andlevel = 0;
  found->space = 0;
  found->help = 0;
  found->section = 0;
  length = (int) stack - (int) hstart;
  help_list = (struct command *) MALLOC(length);
  memcpy(help_list, hstart, length);
  stack = oldstack;
}


/* load that help file in  */

int get_help(player * p, char *str)
{
  int fail = 0;
  file text;
  char *oldstack = stack;

  char header[75];

  if (*str == '.')
  {
    stack = oldstack;
    return 0;
  }

  sprintf(stack, "doc/%s.help", str);
  stack = end_string(stack);
  text = load_file_verbose(oldstack, 0);
  if (text.where)
  {
    if (*(text.where))
    {
      stack = oldstack;
      snprintf(header, 70, "%s Online Help: %s",
	       get_config_msg("talker_name"), str);
      pstack_mid(header);

      sprintf(stack, "\n%s"
	      LINE, text.where);
      stack = end_string(stack);
      pager(p, oldstack);
      fail = 1;
    }
    else
      fail = 0;
    FREE(text.where);
  }
  stack = oldstack;
  return fail;
}



int get_victim(player * p, char *text)
{
  return 0;
}


/* the help command */

void help(player * p, char *str)
{
  char *oldstack, header[75];
  struct command *fn, *comlist;
  char command[100] = "";

  oldstack = stack;

  if (!*str)
  {
    if (p->residency)
      str = "general";
    else
      str = "newbie";
  }
  /* so ressies can see "help su" */
  if (!strcasecmp(str, "su"))
  {
    str = "superuser";
  }
  if (isalpha(*str))
    comlist = coms[((int) (tolower(*str)) - (int) 'a' + 1)];
  else
    comlist = coms[0];

  strncpy(command, str, 95);

  /* Here it is - check person's privs before helping them =) */
  for (; comlist->text; comlist++)
    if (!strcmp(comlist->text, str))
      if ((!(p->residency & comlist->level)) && comlist->level != 0)
	str = " ";
  fn = find_help(str);
  if (!fn || !(fn->help))
  {
    if (get_help(p, str))
      return;
    stack = oldstack;
    TELLPLAYER(p, " Cannot find any help on '%s'. \n", command);
    LOGF("help", "%s requested help on '%s'", p->name, command);
    return;
  }
  if (!strcasecmp(str, "newbie"))
    if (get_victim(p, fn->help))
    {
      stack = oldstack;
      return;
    }
  snprintf(header, 70, "%s Online Help: %s",
	   get_config_msg("talker_name"), command);
  pstack_mid(header);

  sprintf(stack, "%s"
	  LINE, fn->help);
  stack = end_string(stack);

  pager(p, oldstack);
  stack = oldstack;
}


void forcehelp(player * p, char *str)
{
  player *p2;
  char *temp;

  temp = next_space(str);
  *temp++ = 0;

  if (!*str)
  {
    tell_player(p, " Format: forcehelp <player> <help file>\n");
    return;
  }
  p2 = find_player_global(str);
  if (!p2)
    return;

  if (p2 == p)
  {
    tell_player(p, " You don't need to forcehelp yourself something!\n");
    return;
  }

  tell_player(p2, " -=> I think you need to read this ... \n");
  help(p2, temp);

  SUWALL(" -=*> %s shows help '%s' to %s\n", p->name, temp, p2->name);
  LOGF("forcehelp", "%s forcehelpped '%s' to %s.", p->name, temp, p2->name);
}

/* Credit goes to Renegade for spotting a bug with this code. The original
   PG one didn't check to see if the SU/Minister/etc was actually a person
   on the talker (indeed a person at all). Must amusement came from typing the
   command "push <name> a large cliff" or "push <name> a fluffy sheep" */

void redtape(player * p, char *str)
{
  player *p2, *p3;
  char *temp, *oldstack;

  temp = next_space(str);
  *temp++ = 0;

  oldstack = stack;
  if ((!*str) || (!*temp))
  {
    tell_player(p, " Format: redtape <git> <SuperUser/Minister/etc>\n");
    return;
  }
  p2 = find_player_global(str);
  if (!p2)
    return;

  p3 = find_player_global(temp);
  if (!p3)
    return;

  if (p2 == p3)
  {
    tell_player(p, "Are you sure about that??\n");
    return;
  }
  sprintf(stack, "\n -=*> %s may be able to help you better. \n\n", p3->name);
  stack = end_string(oldstack);
  command_type |= HIGHLIGHT;
  tell_player(p2, oldstack);
  command_type &= ~HIGHLIGHT;
  stack = oldstack;

  sprintf(stack, " -=*> %s pushes %s towards %s\n", p->name, p2->name, p3->name);
  stack = end_string(oldstack);
  su_wall(oldstack);
  stack = oldstack;
}


/* Show the clock - okay so this is one poxy little procedure but I
   like it this way, so there. --Silver
   (function seperation is a good thing, it leads to easily
   readable and modifiable code *grin* ~phy) */

void show_clock(player * p)
{
  char *oldstack = stack;
  time_t t;
  static char buf[80];

  if (p->jetlag)
    t = time(0) + (ONE_HOUR * p->jetlag);
  else
    t = time(0);
  strftime(buf, 79, "%I:%M %p", localtime(&t));

  sprintf(stack, get_config_msg("hourly_clock"), buf);
  stack = end_string(stack);
  TELLPLAYER(p, " %s\007\n", oldstack);
  stack = oldstack;
}


/* Specilised match code becuase strnomatch is no good */

int swear_match(char *str1, char *str2)
{

  char *s1p, *s2p;

  s1p = str1;
  s2p = str2;

  for (; *s1p; s1p++, s2p++)
  {
    if (tolower(*s1p) != tolower(*s2p))
      return 1;
  }
  return 0;
}


/* Swear words removal code :o) 
   A list of rude words can be found in admin.h - gasp! */

char rand_curse_char(void)
{
  srand(rand());
  return (char) (rand() % 6 + 33);
}


char *filter_rude_words(char *str)
{
  char *ptr = str, *rude;

  while ((rude = strcaseline(ptr, rude_msg.where)))
    while (*rude && isalpha(*rude))
      *rude++ = rand_curse_char();

  return str;
}

void swear_version(void)
{
  sprintf(stack, " -=*> Swear filtering v1.0 (by Silver) enabled.\n");
  stack = strchr(stack, 0);
}
