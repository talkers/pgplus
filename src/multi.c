/*
 * Playground+ - multi.c
 * Multi (or chains) code written by Segtor
 * ---------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "include/config.h"

#ifdef ALLOW_MULTIS

#define MULTI_VERSION "1.00.1"

#include "include/player.h"
#include "include/proto.h"


multi *all_multis;


void init_multis(void)
{
  all_multis = NULL;
}

void destroy_all_multis(void)
{
  multi *mscan = all_multis, *mscan2 = NULL;
  multiplayer *pscan, *pscan2;

  while (mscan)
  {
    pscan = mscan->players_list;
    pscan2 = NULL;

    while (pscan)
    {
      pscan2 = pscan;
      pscan = pscan->next_player;
      FREE(pscan2);
    }

    mscan2 = mscan;
    mscan = mscan->next_multi;
    FREE(mscan2);
  }

  all_multis = 0;
}

void multi_change_entries(player * p1, player * p2)
{
  multi *mscan = all_multis;
  multiplayer *pscan, *prev;

  while (mscan)
  {
    pscan = mscan->players_list;
    prev = NULL;

    while (pscan && (pscan->the_player != p1))
    {
      prev = pscan;
      pscan = pscan->next_player;
    }
    if (pscan->the_player == p1)
      pscan->the_player = p2;

    mscan = mscan->next_multi;
  }
}

int get_number(int real_number)
{
  return ((real_number - 1) / 32);
}

int get_bit(int real_number)
{
  return ((real_number - 1) % 32);
}

int get_real_number(int number, int bit)
{
  return ((number * 32 + bit) + 1);
}

int multi_get_new_number(void)
{
  multi *mscan = all_multis;
  int inuse[32];		/* 32 * 32 = 1024 possible multis */
  int i, j;

  for (i = 0; i < 32; i++)
    inuse[i] = 0;

  /* create list of multis in use */
  while (mscan)
  {
    inuse[get_number(mscan->number)] |= (2 << get_bit(mscan->number));
    mscan = mscan->next_multi;
  }

  /* find free number */
  for (i = 0; i < 32; i++)
    for (j = 0; j < 32; j++)
      if (!(inuse[i] & 2 << j))
	return (get_real_number(i, j));

  /* none found */
  return 0;
}

void remove_from_multis(player * p)
{
  multi *mscan = all_multis;
  multiplayer *pscan;

  while (mscan)
  {
    pscan = mscan->players_list;

    while (pscan)
    {
      if (pscan->the_player == p)
	pscan->the_player = NULL;

      pscan = pscan->next_player;
    }

    mscan = mscan->next_multi;
  }
}

void remove_from_multi(player * p, multi * m)
{
  multiplayer *pscan;

  if (!(m->players_list))
    return;

  pscan = m->players_list;

  while (pscan)
  {
    if (pscan->the_player == p)
      pscan->the_player = NULL;

    pscan = pscan->next_player;
  }
}

void tell_multi(int number, player * p, char *str)
{
  multi *mscan = find_multi_from_number(number);
  multiplayer *pscan;
  int curr_sys_col;
  int is_emote = 0, br;
  int curr_mode;

  sys_color_atm = TELsc;
  command_type |= MULTI_COM;

  if (number < 0)
  {
    number = -number;
    is_emote = 1;
    mscan = find_multi_from_number(number);
  }

  if (mscan)
  {
    mscan->multi_idle = 0;
    mscan->multi_flags &= ~MULTI_WARNED;

    if (mscan->multi_flags & MULTI_FRIENDSLIST)
      sys_color_atm = FRTsc;

    curr_sys_col = sys_color_atm;

    pscan = mscan->players_list;

    while (pscan)
    {
      curr_mode = command_type;
      if ((pscan->the_player) && (pscan->the_player->tag_flags & BLOCK_MULTIS))
      {
	command_type = 0;
	sys_color_atm = SYSsc;
	if (p)
	  TELLPLAYER(p, " %s is blocking multis\n",
		     pscan->the_player->name);
	command_type = curr_mode;
      }
      else if ((pscan->the_player) && (pscan->the_player->tag_flags & BLOCK_FRIENDS))
      {
	command_type = 0;
	sys_color_atm = SYSsc;
	if (p)
	  TELLPLAYER(p, " %s is blocking friends\n",
		     pscan->the_player->name);
	command_type = curr_mode;
      }
      else if ((pscan->the_player) && (pscan->the_player->tag_flags & BLOCK_TELLS))
      {
	command_type = 0;
	sys_color_atm = SYSsc;
	if (p)
	  TELLPLAYER(p, " %s is blocking tells\n",
		     pscan->the_player->name);
	command_type = curr_mode;
      }
      else
      {
	if (pscan->the_player != p)
	{
	  if (pscan->the_player)
	  {
	    if (p && !(command_type & ECHO_COM))
	    {
	      if (is_emote && emote_no_break(*str))
	      {
		TELLPLAYER(pscan->the_player, "(%d) %s%s", number, p->name, str);
#ifdef INTELLIBOTS
		if (pscan->the_player->residency & ROBOT_PRIV)
		  intelligent_robot(p, pscan->the_player, str, IR_PRIVATE);
#endif
	      }
	      else
	      {
		br = 0;
		if (*str == '\007' && pscan->the_player->custom_flags & NOBEEPS)
		{
		  br = 1;
		  str++;
		}
		TELLPLAYER(pscan->the_player, "(%d) %s %s", number, p->name, str);
#ifdef INTELLIBOTS
		if (pscan->the_player->residency & ROBOT_PRIV)
		  intelligent_robot(p, pscan->the_player, str, IR_PRIVATE);
#endif
		if (br)
		  str--;
	      }
	    }
	    else
	    {
	      TELLPLAYER(pscan->the_player, "(%d) %s", number, str);
#ifdef INTELLIBOTS
	      if (p && (pscan->the_player->residency & ROBOT_PRIV))
		intelligent_robot(p, pscan->the_player, str, IR_PRIVATE);
#endif
	    }
	  }
	}
      }
      sys_color_atm = curr_sys_col;

      pscan = pscan->next_player;
    }
  }

  command_type &= ~MULTI_COM;
  sys_color_atm = SYSsc;

  if (mscan)
  {
    pscan = mscan->players_list;

    while (pscan)
    {
      if (pscan->the_player)
	pscan->the_player->flags &= ~TAGGED;
      pscan = pscan->next_player;
    }
  }
}

void vtell_multi(int number, player * p, char *format,...)
{
  va_list argum;
  char *oldstack;

  oldstack = stack;
  va_start(argum, format);
  vsprintf(stack, format, argum);
  va_end(argum);
  stack = end_string(stack);
  tell_multi(number, p, oldstack);
  stack = oldstack;
}

multi *remove_multi(multi * m1, multi * m2, char *reason)
{
  multi *m3 = m1->next_multi;
  multiplayer *p, *n;

  if (!m1)
    return (multi *) 0;

  if (config_flags & cfMULTI_INFORM)
    vtell_multi(m1->number, NULL, "Multi removed%s\n", reason);

  if (m2)
    m2->next_multi = m3;
  else
    all_multis = m3;

  p = m1->players_list;
  while (p)
  {
    n = p;
    p = p->next_player;
    FREE(n);
  }
  FREE(m1);

  if (m2)
    return (m2->next_multi);
  return m3;
}

void update_multis(void)
{
  multi *mscan = all_multis, *mscan2 = NULL;
  multiplayer *temppl = NULL, *pscan = NULL;
  char ti1[250], ti2[250];
  int ci;

  while (mscan)
  {
    action = "multi: list empty";
    mscan->multi_idle++;

    /* remove multi if player list empty */
    if (!(mscan->players_list))
    {
      mscan = remove_multi(mscan, mscan2, "");
      continue;
    }

    action = "multi: check idle";
    /* remove multi for being idle too long */
    ci = atoi(get_config_msg("multi_idle_out"));
    ci *= ONE_MINUTE;
    if (ci > 0 && mscan->multi_idle >= ci)
    {
      if (!(mscan->multi_flags & MULTI_NOIDLEOUT))
      {
	mscan = remove_multi(mscan, mscan2, " due to being idle");
	continue;
      }
    }

    action = "multi: owner lost";
    /* remove friends multi that lost its owner */
    if (mscan->multi_flags & MULTI_FRIENDSLIST)
    {
      if (!(mscan->players_list->the_player))
      {
	mscan = remove_multi(mscan, mscan2, " due to owner leaving");
	continue;
      }
    }

    action = "multi: enough users";
    /* remove multi if too few people on */
    if (players_on_multi(mscan) < 2)
    {
      mscan = remove_multi(mscan, mscan2, " due to too few users");
      continue;
    }

    if (config_flags & cfMULTI_INFORM)
    {
      action = "multi: warn idle";
      /* warn idle multis */
      ci = atoi(get_config_msg("multi_idle_out"));
      ci *= ONE_MINUTE;
      if (ci > 0)
      {
	if ((mscan->multi_idle >= (ci - 60)) &&
	    (!(mscan->multi_flags & MULTI_WARNED)))
	{
	  if (!(mscan->multi_flags & MULTI_NOIDLEOUT))
	  {
	    strcpy(ti1, word_time(ci - 60));
	    strcpy(ti2, word_time(ci));
	    vtell_multi(mscan->number, NULL, "Multi has been idle for %s, "
			"will timeout in %s\n", ti1, ti2);

	    mscan->multi_flags |= MULTI_WARNED;
	    mscan->multi_idle = ci - 60;
	  }
	}
      }
    }

    action = "multi: catenate first";
    /* catenate players list */
    while (mscan->players_list)
    {
      if (mscan->players_list->the_player)
	break;
      temppl = mscan->players_list;
      mscan->players_list = mscan->players_list->next_player;
      FREE(temppl);
    }

    action = "multi: catenate next";
    pscan = mscan->players_list;
    temppl = NULL;
    while (pscan)
    {
      if (!(pscan->the_player))
      {
	temppl->next_player = pscan->next_player;
	FREE(pscan);
	pscan = temppl;
      }
      temppl = pscan;
      pscan = pscan->next_player;
    }

    mscan2 = mscan;
    mscan = mscan->next_multi;
  }
  action = "";
}

int player_on_multi(player * p, multi * m)
{
  multiplayer *pscan = m->players_list;

  while (pscan)
  {
    if (pscan->the_player == p)
      return 1;
    pscan = pscan->next_player;
  }

  return 0;
}

multi *find_multi_from_number(int number)
{
  multi *mscan = all_multis;

  while (mscan)
  {
    if (mscan->number == number)
      return mscan;
    mscan = mscan->next_multi;
  }

  return NULL;
}

int find_friend_multi_number_name(char *pl_name)
{
  multi *mscan = all_multis;

  while (mscan)
  {
    if (mscan->multi_flags & MULTI_FRIENDSLIST)
      if (mscan->players_list)
	if (mscan->players_list->the_player)
	  if (!(strcasecmp(mscan->players_list->the_player->lower_name, pl_name)))
	    return mscan->number;

    mscan = mscan->next_multi;
  }

  return 0;
}

int find_friend_multi_number(player * p)
{
  return find_friend_multi_number_name(p->lower_name);
}

int players_on_multi(multi * m)
{
  int count = 0;
  multiplayer *pscan;

  if (m)
  {
    pscan = m->players_list;

    while (pscan)
    {
      if (pscan->the_player)
	count++;

      pscan = pscan->next_player;
    }
  }

  return count;
}

int multi_count(void)
{
  multi *mscan = all_multis;
  int count = 0;

  while (mscan)
  {
    count++;
    mscan = mscan->next_multi;
  }

  return count;
}

int assign_friends_multi(player * p)
{
  int mn;

  mn = find_friend_multi_number(p);
  if (mn)
    return mn;

  mn = multi_get_new_number();

  create_friends_multi(p, mn);
  return mn;
}

void add_to_friends_multis(player * p)
{
  player *scan;
  list_ent *l;
  int mn;

  for (scan = flatlist_start; scan; scan = scan->flat_next)
  {
    l = find_list_entry(scan, p->name);
    if (l)
      if (l->flags & FRIEND)
      {
	mn = find_friend_multi_number(scan);
	if (mn)
	  add_to_multi(p, find_multi_from_number(mn));
      }
  }
}

void add_to_multi(player * p, multi * m)
{
  multiplayer *pscan = NULL, *t = NULL;

  pscan = m->players_list;

  while (pscan)
  {
    t = pscan;
    pscan = pscan->next_player;
  }

  pscan = (multiplayer *) MALLOC(sizeof(multiplayer));
  pscan->next_player = NULL;
  pscan->the_player = p;
  if (t)
    t->next_player = pscan;
  else
    m->players_list = pscan;
}

/* creating multis */
void create_friends_multi(player * p, int number)
{
  multi *new_multi = (multi *) MALLOC(sizeof(multi));
  list_ent *l;
  player *p2;

  new_multi->multi_flags = MULTI_FRIENDSLIST;

  new_multi->number = number;
  new_multi->multi_idle = 0;

  new_multi->next_multi = all_multis;
  all_multis = new_multi;

  new_multi->players_list = NULL;

  add_to_multi(p, new_multi);

  /* loop through players list */
  if (p->saved)
  {
    l = p->saved->list_top;

    if (l)
    {
      do
      {
	if (l->flags & FRIEND && strcasecmp(l->name, "everyone"))
	{
	  p2 = find_player_absolute_quiet(l->name);
	  if (p2)
	    add_to_multi(p2, new_multi);
	}
	l = l->next;
      }
      while (l);
    }
  }
}

/* multi talking stuff */
void multi_number_to_names(player * p, int number, char *str)
{
  char *oldstr = str;
  multi *m = find_multi_from_number(number);
  multiplayer *mp;
  int pls, listed;

  if (!m)
    return;

  pls = players_on_multi(m);
  listed = 0;
  if (p == NULL)
    pls++;
  mp = m->players_list;
  while (mp)
  {
    if (!(mp->the_player == p))
    {
      if (listed < (pls - 3))
	sprintf(str, "%s, ", mp->the_player->name);
      else if (listed == (pls - 3))
	sprintf(str, "%s and ", mp->the_player->name);
      else
	sprintf(str, "%s", mp->the_player->name);
      str = strchr(str, 0);
      listed++;
    }
    mp = mp->next_player;
  }
  str = oldstr;
}

int solve_multi(player * p, char *str)
{
  char *endp, *start, *oldstack = stack;
  int mn;
  player *p2;
  multi *mnew;

  if (isdigit(*str))
  {
    if (strchr(str, ','))
    {
      start = stack;
      while (*str && *str != ',')
	*stack++ = *str++;
      *stack++ = 0;
      if (*str)
	str++;
      mn = (int) strtol(start, &endp, 10);
      if (*endp)
      {
	tell_player(p, "Please enter a correct multi number\n");
        stack = oldstack;
	return -1;
      }
      mnew = find_multi_from_number(mn);
      if (!mnew)
      {
	tell_player(p, "That multi does not exist\n");
        stack = oldstack;
	return -1;
      }
      if (!(player_on_multi(p, mnew)))
      {
	tell_player(p, "You're not on that multi\n");
        stack = oldstack;
	return -1;
      }
      if (mnew->multi_flags & MULTI_FRIENDSLIST)
      {
	tell_player(p, "You can't add people to friends multis\n");
        stack = oldstack;
	return -1;
      }
      stack = start;
      while (*str)
      {
	start = stack;
	while (*str && *str != ',')
	  *stack++ = *str++;
	*stack++ = 0;
	if (*str)
	  str++;
	if (!strcasecmp(start, "me"))
	  p2 = NULL;
	else
	{
	  command_type |= NO_P_MATCH;
	  p2 = find_player_global(start);
	  command_type &= ~NO_P_MATCH;
	}
	if (p2 && !player_on_multi(p2, mnew))
	  add_to_multi(p2, mnew);
	stack = start;
      }
      return mn;
    }
    mn = (int) strtol(str, &endp, 10);
    if (*endp)
    {
      tell_player(p, "Please enter a correct multi number\n");
      return -1;
    }
    mnew = find_multi_from_number(mn);
    if (!mnew)
    {
      tell_player(p, "No such multi exists\n");
      return -1;
    }
    if (!(player_on_multi(p, mnew)))
    {
      tell_player(p, "You're not on that multi!\n");
      return -1;
    }
  }
  else if (!(strcasecmp(str, "friends")))
  {
    mn = find_friend_multi_number(p);
  }
  else if (strchr(str, ','))
  {
    mnew = (multi *) MALLOC(sizeof(multi));
    mnew->next_multi = all_multis;
    mn = multi_get_new_number();
    mnew->number = mn;
    mnew->multi_idle = 0;
    mnew->multi_flags = MULTI_NOIDLEOUT;
    mnew->players_list = NULL;
    add_to_multi(p, mnew);
    while (*str)
    {
      start = stack;
      while (*str && *str != ',')
	*stack++ = *str++;
      *stack++ = 0;
      if (*str)
	str++;
      if (!strcasecmp(start, "me"))
	p2 = NULL;
      else
      {
	command_type |= NO_P_MATCH;
	p2 = find_player_global(start);
	command_type &= ~NO_P_MATCH;
      }
      if (p2 && !player_on_multi(p2, mnew))
	add_to_multi(p2, mnew);
      stack = start;
    }
    mnew->multi_flags &= ~MULTI_NOIDLEOUT;
    all_multis = mnew;
  }
  else
  {
    p2 = find_player_global(str);
    mn = find_friend_multi_number(p2);
  }
  stack = oldstack;
  return mn;
}

void multi_tell(player * p, char *str, char *msg)
{
  char *scan, *mid, pnames[1000];
  int mn;
  multi *m;

  mn = solve_multi(p, str);
  if (mn < 1)
    return;

  m = find_multi_from_number(mn);

  for (scan = msg; *scan; scan++);

  switch (*(--scan))
  {
    case '?':
      mid = "asks of";
      break;
    case '!':
      mid = "exclaims to";
      break;
    default:
      mid = "tells";
      break;
  }

  if (m->multi_flags & MULTI_FRIENDSLIST)
  {
    if (m->players_list->the_player == p)
      vtell_multi(mn, p, "%s %s friends '%s'\n", mid, their_player(p),
		  msg);
    else
      vtell_multi(mn, p, "%s %s's friends '%s'\n", mid,
		  m->players_list->the_player->name, msg);
  }
  else
  {
    pnames[0] = 0;
    multi_number_to_names(p, mn, pnames);
    if (config_flags & cfMULTIVERBOSE)
      vtell_multi(mn, p, "%s %s '%s'\n", mid, pnames, msg);
    else
      vtell_multi(mn, p, "%s you all '%s'\n", mid, msg);
  }

  switch (*scan)
  {
    case '?':
      mid = "ask of";
      break;
    case '!':
      mid = "exclaim to";
      break;
    default:
      mid = "tell";
      break;
  }

  sys_color_atm = SYSsc;

  if (m->multi_flags & MULTI_FRIENDSLIST)
  {
    if (m->players_list->the_player == p)
      TELLPLAYER(p, "(%d) You %s your friends '%s'\n", mn, mid, msg);
    else
      TELLPLAYER(p, "(%d) You %s %s's friends '%s'\n", mn, mid,
		 m->players_list->the_player->name, msg);
  }
  else
  {
    if (config_flags & cfMULTIVERBOSE)
      TELLPLAYER(p, "(%d) You %s %s '%s'\n", mn, mid, pnames, msg);
    else
      TELLPLAYER(p, "(%d) You %s them '%s'\n", mn, mid, msg);
  }
}

void multi_remote(player * p, char *str, char *msg)
{
  int mn;
  multi *m;
  char pnames[1000];
  char tostr[512];

  mn = solve_multi(p, str);
  if (mn < 1)
    return;

  m = find_multi_from_number(mn);

  if (m->multi_flags & MULTI_FRIENDSLIST)
  {
    if (m->players_list->the_player == p)
    {
      if (config_flags & cfMULTIVERBOSE)
	vtell_multi(-mn, p, "%s (to %s friends)\n", msg, their_player(p));
      else
	vtell_multi(-mn, p, "%s\n", msg);
      sys_color_atm = SYSsc;
      if (emote_no_break(*msg))
      {
	if (config_flags & cfMULTIVERBOSE)
	  TELLPLAYER(p, "(%d) You emote: %s%s (to your friends)\n", mn,
		     p->name, msg);
	else
	  TELLPLAYER(p, "(%d) You emote: %s%s\n", mn, p->name, msg);
      }
      else
      {
	if (config_flags & cfMULTIVERBOSE)
	  TELLPLAYER(p, "(%d) You emote: %s %s (to your friends)\n", mn,
		     p->name, msg);
	else
	  TELLPLAYER(p, "(%d) You emote: %s %s\n", mn, p->name, msg);
      }
    }
    else
    {
      if (config_flags & cfMULTIVERBOSE)
	vtell_multi(-mn, p, "%s (to %s's friends)\n", msg,
		    m->players_list->the_player->name);
      else
	vtell_multi(-mn, p, "%s\n", msg);

      sys_color_atm = SYSsc;
      if (emote_no_break(*msg))
      {
	if (config_flags & cfMULTIVERBOSE)
	  TELLPLAYER(p, "(%d) You emote: %s%s (to %s's friends)\n", mn,
		     p->name, msg, m->players_list->the_player->name);
	else
	  TELLPLAYER(p, "(%d) You emote: %s%s\n", mn, p->name, msg);
      }
      else
      {
	if (config_flags & cfMULTIVERBOSE)
	  TELLPLAYER(p, "(%d) You emote: %s %s (to %s's friends)\n", mn,
		     p->name, msg, m->players_list->the_player->name);
	else
	  TELLPLAYER(p, "(%d) You emote: %s %s\n", mn, p->name, msg);
      }
    }
  }
  else
  {
    pnames[0] = 0;
    multi_number_to_names(p, mn, pnames);
    if (config_flags & cfMULTIVERBOSE)
      sprintf(tostr, " (to %s)", pnames);
    else
      memset(tostr, 0, 512);

    vtell_multi(-mn, p, "%s%s\n", msg, tostr);
    sys_color_atm = SYSsc;
    if (emote_no_break(*msg))
      TELLPLAYER(p, "(%d) You emote: %s%s%s\n", mn, p->name,
		 msg, tostr);
    else
      TELLPLAYER(p, "(%d) You emote: %s %s%s\n", mn, p->name,
		 msg, tostr);
  }
}

void multi_rsing(player * p, char *str, char *msg)
{
  char *oldstack = stack;

  sprintf(stack, "sings o/~ %s o/~", msg);
  stack = end_string(stack);
  multi_remote(p, str, oldstack);
  stack = oldstack;
}

void multi_yell(player * p, char *str, char *msg)
{
  char *scan, *mid, pnames[1000];
  int mn;
  multi *m;

  mn = solve_multi(p, str);
  if (mn < 1)
    return;

  m = find_multi_from_number(mn);

  for (scan = msg; *scan; scan++);

  switch (*(--scan))
  {
    case '?':
      mid = "boggles at";
      break;
    case '!':
      mid = "hollers at";
      break;
    default:
      mid = "yells at";
      break;
  }

  if (m->multi_flags & MULTI_FRIENDSLIST)
  {
    if (m->players_list->the_player == p)
      vtell_multi(mn, p, "\007%s %s friends '%s'\n", mid, their_player(p),
		  msg);
    else
      vtell_multi(mn, p, "\007%s %s's friends '%s'\n", mid,
		  m->players_list->the_player->name, msg);
  }
  else
  {
    pnames[0] = 0;
    multi_number_to_names(p, mn, pnames);
    vtell_multi(mn, p, "\007%s %s '%s'\n", mid, pnames, msg);
  }

  switch (*scan)
  {
    case '?':
      mid = "boggle at";
      break;
    case '!':
      mid = "holler at";
      break;
    default:
      mid = "yell at";
      break;
  }

  sys_color_atm = SYSsc;

  if (m->multi_flags & MULTI_FRIENDSLIST)
  {
    if (m->players_list->the_player == p)
      TELLPLAYER(p, "(%d) You %s your friends '%s'\n", mn, mid, msg);
    else
      TELLPLAYER(p, "(%d) You %s %s's friends '%s'\n", mn, mid,
		 m->players_list->the_player->name, msg);
  }
  else
    TELLPLAYER(p, "(%d) You %s %s '%s'\n", mn, mid, pnames, msg);
}

void multi_recho(player * p, char *str, char *msg)
{
  int mn;
  multi *m;
  char pnames[1000];
  char tostr[512];

  mn = solve_multi(p, str);
  if (mn < 1)
    return;

  m = find_multi_from_number(mn);
  command_type |= ECHO_COM;

  if (m->multi_flags & MULTI_FRIENDSLIST)
  {
    if (m->players_list->the_player == p)
    {
      if (config_flags & cfMULTIVERBOSE)
	sprintf(tostr, " (to %s friends)", their_player(p));
      else
	memset(tostr, 0, 512);

      vtell_multi(mn, p, "%s%s\n", msg, tostr);
      sys_color_atm = SYSsc;
      TELLPLAYER(p, "(%d) You echo: %s (to your friends)\n", mn, msg);
    }
    else
    {
      if (config_flags & cfMULTIVERBOSE)
	sprintf(tostr, " (to %s's friends)",
		m->players_list->the_player->name);
      else
	memset(tostr, 0, 512);

      vtell_multi(mn, p, "%s%s\n", msg, tostr);
      sys_color_atm = SYSsc;
      TELLPLAYER(p, "(%d) You echo: %s (to %s's friends)\n", mn, msg,
		 m->players_list->the_player->name);
    }
  }
  else
  {
    pnames[0] = 0;
    multi_number_to_names(p, mn, pnames);
    if (config_flags & cfMULTIVERBOSE)
      sprintf(tostr, " (to %s)", pnames);
    else
      memset(tostr, 0, 512);
    vtell_multi(mn, p, "%s%s\n", msg, tostr);
    sys_color_atm = SYSsc;

    TELLPLAYER(p, "(%d) You echo: %s%s\n", mn, msg, tostr);
  }
  command_type &= ~ECHO_COM;
}

/* misc commands */
void multi_block(player * p, char *str)
{
  if (*str)
  {
    if (strcasecmp(str, "on"))
    {
      if (strcasecmp(str, "off"))
      {
	tell_player(p, " Format: blockmulti [on/off]\n");
	return;
      }
      p->tag_flags |= BLOCK_MULTIS;
    }
    else
      p->tag_flags &= ~BLOCK_MULTIS;
  }
  else
    p->tag_flags ^= BLOCK_MULTIS;

  if (!(p->tag_flags & BLOCK_MULTIS))
    tell_player(p, " You decide to get multis again\n");
  else
    tell_player(p, " You block multis\n");
}

/* list multis player is on */
void list_multis_me_on(player * p)
{
  multi *mscan = all_multis;
  char *oldstack = stack;
  int inuse[32];
  int i, j, any = 0;

  for (i = 0; i < 32; i++)
    inuse[i] = 0;

  while (mscan)
  {
    if (player_on_multi(p, mscan))
      any++;
    inuse[get_number(mscan->number)] |= (2 << get_bit(mscan->number));
    mscan = mscan->next_multi;
  }

  if (!any)
    tell_player(p, " You aren't on any multis\n");
  else
  {
    sprintf(stack, " You are on %d multis: ", any);
    stack = strchr(stack, 0);

    for (i = 0; i < 32; i++)
      for (j = 0; j < 32; j++)
	if (inuse[i] & 2 << j)
	  if (player_on_multi(p, find_multi_from_number(get_real_number(i, j))))
	  {
	    sprintf(stack, "%d, ", get_real_number(i, j));
	    stack = strchr(stack, 0);
	  }

    stack--;
    stack--;
    *stack++ = '.';
    *stack++ = '\n';
    *stack++ = 0;

    tell_player(p, oldstack);
  }

  stack = oldstack;
}

/* staff list multis, only lists those multis that are in use */
void list_all_in_use(player * p)
{
  multi *mscan = all_multis;
  char *oldstack = stack;
  int inuse[32];
  int i, j, any = 0;

  for (i = 0; i < 32; i++)
    inuse[i] = 0;

  while (mscan)
  {
    any++;
    inuse[get_number(mscan->number)] |= (2 << get_bit(mscan->number));
    mscan = mscan->next_multi;
  }

  if (!any)
    tell_player(p, " There are no multis currently in use\n");
  else
  {
    sprintf(stack, " There are currently %d multis in use: ", any);
    stack = strchr(stack, 0);

    for (i = 0; i < 32; i++)
      for (j = 0; j < 32; j++)
	if (inuse[i] & 2 << j)
	{
	  sprintf(stack, "%d, ", get_real_number(i, j));
	  stack = strchr(stack, 0);
	}

    stack--;
    stack--;
    *stack++ = '.';
    *stack++ = '\n';
    *stack++ = 0;

    tell_player(p, oldstack);
  }

  stack = oldstack;
}

/*
 * list multis / multi
 * -without parameters list all multis user is on
 * -with parameter list members of the multi
 * -for LOWER_ADMIN and higher, list also all multis in use, if used without
 * parameters
 */
void multi_list(player * p, char *str)
{
  char *endp, pnames[1000];
  int num;
  multi *m;

  if (*str)
  {
    num = (int) strtol(str, &endp, 10);
    if (*endp)
    {
      tell_player(p, "Multi number not correct\n");
      return;
    }

    m = find_multi_from_number(num);
    if (!m)
    {
      tell_player(p, "No such multi exists\n");
      return;
    }

    if (!(player_on_multi(p, m)))
    {
      tell_player(p, "You're not on that multi\n");
      return;
    }

    if (m->multi_flags & MULTI_FRIENDSLIST)
    {
      if (m->players_list && m->players_list->the_player)
      {
	if (m->players_list->the_player == p)
	{
	  pnames[0] = 0;
	  multi_number_to_names(NULL, num, pnames);
	  TELLPLAYER(p, "Multi %d: Your friends (%s)\n", num, pnames);
	}
	else
	  TELLPLAYER(p, "Multi %d: Friends of %s\n", num,
		     m->players_list->the_player->name);
      }
      else
	TELLPLAYER(p, "Multi %d has lost its owner\n", num);
    }
    else
    {
      pnames[0] = 0;
      multi_number_to_names(NULL, num, pnames);
      TELLPLAYER(p, "Multi %d: %s\n", num, pnames);
    }
  }
  else
  {
    list_multis_me_on(p);
    if (p->residency & LOWER_ADMIN)
      list_all_in_use(p);
  }
}

void multi_idle(player * p, char *str)
{
  char *endp;
  int num;
  multi *m;

  if (!(*str))
  {
    tell_player(p, " Format: idle_multi <number of multi>\n");
    return;
  }

  num = (int) strtol(str, &endp, 10);
  if (*endp)
  {
    tell_player(p, " Multi number not correct\n");
    return;
  }

  m = find_multi_from_number(num);
  if (!m)
  {
    tell_player(p, " No such multi in existence\n");
    return;
  }

  if (!(player_on_multi(p, m)))
  {
    tell_player(p, " You're not on that multi\n");
    return;
  }

  TELLPLAYER(p, " That multi has been idle for %s\n", word_time(m->multi_idle));
}

void multis_version(void)
{
  sprintf(stack, " -=*> PG+ Multis/Chains v%s (by Segtor) enabled.\n", MULTI_VERSION);
  stack = strchr(stack, 0);
}

#endif
