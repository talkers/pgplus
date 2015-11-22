/*
 * Playground+ - commands.c
 * General commands for residents
 * ---------------------------------------------------------------------------
 */

#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <sys/stat.h>

#include "include/config.h"
#include "include/player.h"
#include "include/fix.h"
#include "include/proto.h"

extern int set_players_email(player *, char *);
extern void begin_ressie_doemail(player *, char *);
extern char *is_invalid_email(char *);

/* birthday and age stuff */

void set_age(player * p, char *str)
{
  char *oldstack;
  int new_age;
  oldstack = stack;

  if (!*str)
  {
    tell_player(p, " Format: age <number>\n");
    return;
  }
  new_age = atoi(str);
  if (new_age < 0)
  {
    tell_player(p, " You can't be of a negative age !\n");
    return;
  }
  p->age = new_age;
  if (p->age)
  {
    sprintf(oldstack, " Your age is now set to %d years old.\n", p->age);
    stack = end_string(oldstack);
    tell_player(p, oldstack);
  }
  else
    tell_player(p, " You have turned off your age so no-one can see it.\n");
  stack = oldstack;
}


void set_birthday(player * p, char *str)
{
  struct tm bday, *tm_time;
  time_t the_time;
  int t;

  if (!*str)
  {
    if (config_flags & cfUSUK)
      tell_player(p, " Format: birthday <month>/<day>(/<year>)\n");
    else
      tell_player(p, " Format: birthday <day>/<month>(/<year>)\n");
    return;
  }

  the_time = time(0);
  tm_time = localtime(&the_time);


  memset(&bday, 0, sizeof(struct tm));
  bday.tm_year = tm_time->tm_year;

  if (config_flags & cfUSUK)
  {
    bday.tm_mon = atoi(str);

    if (!bday.tm_mon)
    {
      tell_player(p, " Birthday cleared.\n");
      p->birthday = 0;
      return;
    }
    if (bday.tm_mon <= 0 || bday.tm_mon > 12)
    {
      tell_player(p, " Not a valid month.\n");
      return;
    }
    bday.tm_mon--;

    while (isdigit(*str))
      str++;
    str++;
    bday.tm_mday = atoi(str);
    if (bday.tm_mday < 0 || bday.tm_mday > 31)
    {
      tell_player(p, " Not a valid day of the month.\n");
      return;
    }
  }
  else
  {
    bday.tm_mday = atoi(str);

    if (!bday.tm_mday)
    {
      tell_player(p, " Birthday cleared.\n");
      p->birthday = 0;
      return;
    }
    if (bday.tm_mday < 0 || bday.tm_mday > 31)
    {
      tell_player(p, " Not a valid day of the month.\n");
      return;
    }
    while (isdigit(*str))
      str++;
    str++;
    bday.tm_mon = atoi(str);
    if (bday.tm_mon <= 0 || bday.tm_mon > 12)
    {
      tell_player(p, " Not a valid month.\n");
      return;
    }
    bday.tm_mon--;
  }
  while (isdigit(*str))
    str++;
  str++;
  while (strlen(str) > 2)
    str++;
  bday.tm_year = atoi(str);
  if (bday.tm_year == 0)
  {
    bday.tm_year = tm_time->tm_year;
    p->birthday = TIMELOCAL(&bday);
  }
  else
  {
    p->birthday = TIMELOCAL(&bday);
    t = time(0) - (p->birthday);
    if (t > 0)
      p->age = t / 31536000;
  }

  TELLPLAYER(p, " Your birthday is set to the %s.\n",
	     birthday_string(p->birthday));
}

/* recap someones name */


void recap(player * p, char *str)
{
  char *n;
  int found_lower;
  player *p2 = p;

  if (!*str)
  {
    tell_player(p, " Format: recap <name>\n");
    return;
  }
  if (strcasecmp(str, p->lower_name))
  {
    if (!(config_flags & cfSUSCANRECAP) || !(p->residency & SU))
    {
      tell_player(p, " You can only recap your own name.\n");
      return;
    }
    if (!(p2 = find_player_absolute_quiet(str)))
    {
      TELLPLAYER(p, " Noone by the name '%s' on atm.\n", str);
      return;
    }
    if (!check_privs(p->residency, p2->residency))
    {
      TELLPLAYER(p, " That would really make %s angry tho...\n", p2->name);
      TELLPLAYER(p2, " -=*> %s failed an attempt to recap you.\n", p->name);
      return;
    }
  }



  if (!(config_flags & cfCAPPEDNAMES))
  {
    found_lower = 0;
    n = str;
    while (*n)
    {
      if (*n >= 'a' && *n <= 'z')
	found_lower = 1;
      n++;
    }
    if (!found_lower)
    {
      n = str;
      n++;
      while (*n)
      {
	*n = *n - ('A' - 'a');
	n++;
      }
    }
  }

  strcpy(p2->name, str);
  TELLPLAYER(p, " Name changed to '%s'\n", p2->name);
  if (!(p == p2))
  {
    SW_BUT(p, " -=*> %s ReCaPPeD %s!\n", p->name, p2->name);
    LOGF("recap", "%s recapped %s ...", p->name, p2->name);
  }
}

#ifdef NULLcode
void recap(player * p, char *str)
{
  char *oldstack;
  char *n;
  int found_lower;
  player *p2 = p;

  oldstack = stack;
  if (!*str)
  {
    tell_player(p, " Format: recap <name>\n");
    return;
  }
  strcpy(stack, str);

  if (strcasecmp(stack, p->lower_name))
  {
    tell_player(p, " You can only recap your own name.\n");
    return;
  }
  if (!(config_flags & cfCAPPEDNAMES))
  {
    found_lower = 0;
    n = str;
    while (*n)
    {
      if (*n >= 'a' && *n <= 'z')
	found_lower = 1;
      n++;
    }
    if (!found_lower)
    {
      n = str;
      n++;
      while (*n)
      {
	*n = *n - ('A' - 'a');
	n++;
      }
    }
  }

  strcpy(p2->name, str);
  sprintf(stack, " Name changed to '%s'\n", p2->name);
  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
  if (!(p == p2))
  {
    sprintf(stack, " -=*> %s recapped %s!\n", p->name, p2->name);
    stack = end_string(stack);
    su_wall_but(p, stack);
    stack = oldstack;
  }
}
#endif /* NULLcode */


/* see the time */
/* The standard EW2 code is aweful for this command - so lets rewrite it */
void view_time(player * p, char *str)
{
  char *oldstack = stack, top[70];
  float logperhour = -1, logperday = -1;

  if ((time(0) - up_date) > ONE_HOUR)
    logperhour = (float) logins / (float) (((float) time(0) - (float) up_date) / (float) ONE_HOUR);

  if ((time(0) - up_date) > ONE_DAY)
    logperday = (float) logins / (float) (((float) time(0) - (float) up_date) / (float) ONE_DAY);


  sprintf(top, "%s time and statistics", get_config_msg("talker_name"));
  pstack_mid(top);

  if (p->jetlag && p->hometown[0])
    sprintf(stack, "\n In %s the time is %s.\n",
	    p->hometown, time_diff(p->jetlag));
  else if (p->jetlag)
    sprintf(stack, "\n Your local time is %s.\n", time_diff(p->jetlag));
  else
    sprintf(stack, "\n");
  stack = strchr(stack, 0);

  stack += sprintf(stack, " In %s where ",
		   get_config_msg("server_locate"));
  stack += sprintf(stack, "%s is located, it is %s.\n",
		   get_config_msg("talker_name"), sys_time());
  stack += sprintf(stack,
		   " The talker has been up for %s\n"
		   " That is from %s\n"
		   " Total number of logins in that time is %s.\n",
		   word_time(time(0) - up_date), convert_time(up_date), number2string(logins));

  if (logperhour != -1)
  {
    stack += sprintf(stack, " The average number of logins per hour is %.1f", logperhour);
    if (logperday != -1)
      stack += sprintf(stack, " (%.1f per day)\n", logperday);
    else
      stack += sprintf(stack, ".\n");
  }
  stack += sprintf(stack, " Most people on so far since the last reboot was %d.\n", max_ppl_on_so_far);

  if (p->residency & PSU)
  {
    if (backup_time != -1)
      sprintf(stack, " Last backup took place at %s.\n",
	      convert_time(backup_time));
    else
      sprintf(stack, " Last backup time is currently unknown.\n");
    stack = strchr(stack, 0);
#ifdef SEAMLESS_REBOOT
    stack += sprintf(stack, " The last seamless reboot happened %s ago.\n",
		     word_time(time(0) - reboot_date));
#endif
  }
  sprintf(stack, " Longest connection was set by %s with %s\n\n"
	  LINE, longest_player, word_time(longest_time));

  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
}

/* go quiet */

void go_quiet(player * p, char *str)
{
  earmuffs(p, str);
  blocktells(p, str);
}

/* the idle command */

void check_idle(player * p, char *str)
{
  player *scan, **list, **step;
  int i, n;
  char *oldstack, middle[80], namestring[40], *id;
  file *is_scan;
  int page, pages, count, not_idle = 0;

#ifdef INTERCOM
  if (str && *str && strchr(str, '@'))
  {
    do_intercom_idle(p, str);
    return;
  }
#endif

/* removed for 1.0.8 patch
#ifdef ALLOW_MULTIS
  if (isdigit(*str))
  {
    multi_idle(p, str);
    return;
  }
#endif
*/

  oldstack = stack;
  command_type |= SEE_ERROR;
  if (isalpha(*str) && !strstr(str, "everyone"))
  {
    align(stack);
    list = (player **) stack;
#ifdef ALLOW_MULTIS
    n = global_tag(p, str, 0);
#else
    n = global_tag(p, str);
#endif
    if (!n)
    {
      stack = oldstack;
      return;
    }
    id = stack;
    for (step = list, i = 0; i < n; i++, step++)
    {
      if (p->custom_flags & NOPREFIX)
      {
	strcpy(namestring, (*step)->name);
      }
      else if (*((*step)->pretitle))
      {
	sprintf(namestring, "%s %s", (*step)->pretitle, (*step)->name);
      }
      else
      {
	strcpy(namestring, (*step)->name);
      }
      if (!(*step)->idle)
      {
	sprintf(stack, "%s has just hit return.\n", namestring);
      }
      else
      {
	if ((*step)->idle_msg[0])
	{
	  sprintf(stack, "%s %s\n%s is %s idle\n", namestring,
		  (*step)->idle_msg, caps(gstring((*step))),
		  word_time((*step)->idle));
	}
	else
	{
	  sprintf(stack, "%s is %s idle.\n", namestring,
		  word_time((*step)->idle));
	}
      }
      stack = end_string(stack);
      tell_player(p, id);
      stack = id;
    }
    cleanup_tag(list, n);
    stack = oldstack;
    return;
  }
  if (strstr(str, "everyone"))
  {
    id = str;
    str = strchr(str, ' ');
    if (!str)
    {
      str = id;
      *str++ = '1';
      *str = 0;
    }
  }
  page = atoi(str);
  if (page <= 0)
    page = 1;
  page--;

  pages = (current_players - 1) / (TERM_LINES - 2);
  if (page > pages)
    page = pages;

  for (scan = flatlist_start; scan; scan = scan->flat_next)
  {
    if (scan->name[0] && scan->location && (scan->idle) < 300)
    {
      not_idle++;
    }
  }

  if (current_players == 1)
  {
    sprintf(middle, "There is only you on %s at the moment",
	    get_config_msg("talker_name"));
  }
  else if (not_idle == 1)
  {
    sprintf(middle, "There are %d people here, only one of whom "
	    "appears to be awake", current_players);
  }
  else
  {
    sprintf(middle, "There are %d people here, %d of which appear "
	    "to be awake", current_players, not_idle);
  }
  pstack_mid(middle);

  count = page * (TERM_LINES - 2);
  for (scan = flatlist_start; count; scan = scan->flat_next)
  {
    if (!scan)
    {
      tell_player(p, " Bad idle listing, abort.\n");
      log("error", "Bad idle list");
      stack = oldstack;
      return;
    }
    else if (scan->name[0])
    {
      count--;
    }
  }

  for (count = 0; (count < (TERM_LINES - 1) && scan); scan = scan->flat_next)
  {
    if (scan->name[0] && scan->location)
    {
      if (p->custom_flags & NOPREFIX)
      {
	strcpy(namestring, scan->name);
      }
      else if ((*scan->pretitle))
      {
	sprintf(namestring, "%s %s", scan->pretitle, scan->name);
      }
      else
      {
	sprintf(namestring, "%s", scan->name);
      }
    }
    else
      continue;
    if (scan->idle_msg[0])
    {
      if (emote_no_break(*scan->idle_msg))
	sprintf(stack, "%3d:%.2d - %s%s\n", scan->idle / 60, scan->idle % 60, namestring, scan->idle_msg);
      else
	sprintf(stack, "%3d:%.2d - %s %s\n", scan->idle / 60, scan->idle % 60, namestring, scan->idle_msg);
    }
    else
    {
      for (is_scan = idle_string_list; is_scan->where; is_scan++)
      {
	if (is_scan->length >= scan->idle)
	  break;
      }
      if (!is_scan->where)
	is_scan--;
      sprintf(stack, "%3d:%.2d - %s %s", scan->idle / 60, scan->idle % 60, namestring, is_scan->where);
    }
    stack = strchr(stack, 0);
    count++;
  }
  sprintf(middle, "Page %d of %d", page + 1, pages + 1);
  pstack_mid(middle);

  *stack++ = 0;
  tell_player(p, oldstack);
  stack = oldstack;
}


/* set things */


void set_idle_msg(player * p, char *str)
{

  if (*str)
  {
    strncpy(p->idle_msg, str, MAX_TITLE - 3);
    if (p->custom_flags & NOPREFIX)
    {
      if (emote_no_break(*p->idle_msg))
	TELLPLAYER(p, " Idle message set to ....\n%s%s\nuntil you type a"
		   " new command.\n", p->name, p->idle_msg);
      else
	TELLPLAYER(p, " Idle message set to ....\n%s %s\nuntil you type a"
		   " new command.\n", p->name, p->idle_msg);
    }
    else
    {
      if (emote_no_break(*p->idle_msg))
	TELLPLAYER(p, " Idle message set to ....\n%s%s\nuntil you type a"
		   " new command.\n", full_name(p), p->idle_msg);
      else
	TELLPLAYER(p, " Idle message set to ....\n%s %s\nuntil you type a"
		   " new command.\n", full_name(p), p->idle_msg);
    }
  }
  else
    strcpy(stack, " Please set an idlemsg of a greater than zero length.\n");
}


void set_enter_msg(player * p, char *str)
{

  if (p->system_flags & NO_MSGS)
  {
    tell_player(p, " Sorry, but you have been prevented from changing this.\n");
    return;
  }
  if (!*str)
  {
    tell_player(p, " Format: entermsg <entermsg>\n");
    return;
  }
  strncpy(p->enter_msg, str, MAX_ENTER_MSG - 3);

  if (emote_no_break(*str))
    TELLPLAYER(p, " This is what people will see when you enter the "
	       "room.\n%s%s\n", p->name, p->enter_msg);
  else
    TELLPLAYER(p, " This is what people will see when you enter the "
	       "room.\n%s %s\n", p->name, p->enter_msg);
}

void set_title(player * p, char *str)
{

  if (p->system_flags & NO_MSGS)
  {
    tell_player(p, " Sorry, but you have been prevented from changing this.\n");
    return;
  }
  strncpy(p->title, str, MAX_TITLE - 3);
  if (emote_no_break(*str))
    TELLPLAYER(p, " You change your title so that now you are known as "
	       "...\n%s%s\n", p->name, p->title);
  else
    TELLPLAYER(p, " You change your title so that now you are known as "
	       "...\n%s %s\n", p->name, p->title);
}

void set_pretitle(player * p, char *str)
{
  char *oldstack, *scan;

  if (p->system_flags & NO_MSGS)
  {
    tell_player(p, " Sorry, but you have been prevented from changing this.\n");
    return;
  }
  oldstack = stack;
  if (strstr(str, "+[") != NULL)
  {
    tell_player(p, " You may not have \"+[\" in your prefix.\n");
    return;
  }
  for (scan = str; *scan; scan++)
  {
    switch (*scan)
    {
      case '^':
	tell_player(p, " You may not have colors in your prefix.\n");
	return;
	break;
      case '&':
	tell_player(p, " You may not have dynatext in your prefix.\n");
	return;
	break;
      case ' ':
	tell_player(p, " You may not have spaces in your prefix.\n");
	return;
	break;
      case ',':
	tell_player(p, " You may not have commas in your prefix.\n");
	return;
	break;
    }
  }

  if (find_saved_player(str) || find_player_absolute_quiet(str))
  {
    tell_player(p, " That is the name of a player, so you can't use that "
		"for a prefix.\n");
    return;
  }
  strncpy(p->pretitle, str, MAX_PRETITLE - 3);
  sprintf(stack, " You change your prefix so you become: %s %s\n",
	  p->pretitle, p->name);
  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
}

void set_description(player * p, char *str)
{
  char *oldstack;

  if (p->system_flags & NO_MSGS)
  {
    tell_player(p, " Sorry, but you have been prevented from changing this.\n");
    return;
  }
  oldstack = stack;
  strncpy(p->description, str, MAX_DESC - 3);
  sprintf(stack, " You change your description to read...\n%s\n",
	  p->description);
  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
}

void set_plan(player * p, char *str)
{
  char *oldstack;

  if (p->system_flags & NO_MSGS)
  {
    tell_player(p, " Sorry, but you have been prevented from changing this.\n");
    return;
  }
  oldstack = stack;
  strncpy(p->plan, str, MAX_PLAN - 3);
  sprintf(stack, " You set your plan to read ...\n%s\n", p->plan);
  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
}

void set_prompt(player * p, char *str)
{
  char *dt;

  if ((dt = strchr(str, '&')) &&
      (*(dt + 1) == 'r' || *(dt + 1) == 'a' || *(dt + 1) == 'c'))
  {
    tell_player(p, " You really dont want that dynatext in your prompt do ya?\n");
    return;
  }

  strncpy(p->prompt, str, MAX_PROMPT - 3);
  if (!p->prompt[0])
    tell_player(p, " You turn off your prompt.\n");
  else
    TELLPLAYER(p, " You change your prompt to %s\n", p->prompt);
}

void set_converse_prompt(player * p, char *str)
{
  char *oldstack;

  oldstack = stack;
  strncpy(p->converse_prompt, str, MAX_PROMPT - 3);
  sprintf(stack, " You change your converse prompt to %s\n",
	  p->converse_prompt);
  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
}

void set_term_width(player * p, char *str)
{
  char *oldstack;
  int n;

  oldstack = stack;
  if (!strcasecmp("off", str))
  {
    tell_player(p, " Linewrap turned off.\n");
    p->term_width = 0;
    return;
  }
  n = atoi(str);
  if (!n)
  {
    tell_player(p, " Format: linewrap off/<terminal_width>\n");
    return;
  }
  if (n <= ((p->word_wrap) << 1))
  {
    tell_player(p, " Can't set terminal width that small compared to "
		"word wrap.\n");
    return;
  }
  if (n < 10)
  {
    tell_player(p, " Nah, you haven't got a terminal so small !!\n");
    return;
  }
  p->term_width = n;
  sprintf(stack, " Linewrap set on, with terminal width %d.\n", p->term_width);
  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
}


void set_word_wrap(player * p, char *str)
{
  char *oldstack;
  int n;

  oldstack = stack;
  if (!strcasecmp("off", str))
  {
    tell_player(p, " Wordwrap turned off.\n");
    p->word_wrap = 0;
    return;
  }
  n = atoi(str);
  if (!n)
  {
    tell_player(p, " Format: wordwrap off/<max_word_size>\n");
    return;
  }
  if (n >= ((p->term_width) >> 1))
  {
    tell_player(p, " Can't set max word length that big compared to term "
		"width.\n");
    return;
  }
  p->word_wrap = n;
  sprintf(stack, " Wordwrap set on, with max word size set to %d.\n",
	  p->word_wrap);
  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
}

/* Quit with message */

void byebye(player * p, char *str)
{
  if (p->ttt_opponent)
    ttt_abort(p, 0);

  tell_player(p, quit_msg.where);
  quit(p, 0);
}

/* quit from the program */

void quit(player * p, char *str)
{
  p->flags |= CHUCKOUT;

  if (p->ttt_opponent)
    ttt_abort(p, 0);

  disconnect_channels(p);
#ifdef ALLOW_MULTIS
  remove_from_multis(p);
#endif

  if (!str)
    p->flags |= PANIC;
  tell_player(p, "^N\n");

  /* Clean up lists */

  if (p->residency == NON_RESIDENT)
    check_list_newbie(p->lower_name);
  else
    check_list_resident(p);

  clear_gag_logoff(p);

  purge_gaglist(p, 0);

  if ((p->logged_in == 1) && (sess_name[0] != '\0'))
  {
    if (!strcasecmp(p->name, sess_name))
    {
      session_reset = 0;
    }
  }
}

/* command to change gender */

void gender(player * p, char *str)
{
  *str = tolower(*str);
  switch (*str)
  {
    case 'm':
      p->gender = MALE;
      tell_player(p, " Gender set to Male.\n");
      break;
    case 'f':
      p->gender = FEMALE;
      tell_player(p, " Gender set to Female.\n");
      break;
    case 'n':
      p->gender = OTHER;
      tell_player(p, " Gender set to well, erm, something.\n");
      break;
    default:
      tell_player(p, " No gender set, Format : gender m/f/n\n");
      break;
  }
}



/* save command */

void do_save(player * p, char *str)
{

  if (p->residency == NON_RESIDENT)
  {
    log("error", "Tried to save a non-resi, (chris)");
    return;
  }
  save_player(p);
  /* Moved here so you don't keep getting character saved messages 
     only when you actually DO type "save". Oh yes, the saving is still
     going on! Just it doesn't tell you. --Silver */
  tell_player(p, " -=*> Character Saved\n");

}

/* show email */

void check_email(player * p, char *str)
{
  char *oldstack;

  oldstack = stack;
  if (p->residency == NON_RESIDENT)
  {
    TELLPLAYER(p, " You are non resident and so cannot set an email address.\n"
	       " Please ask a %s to make you resident.\n", get_config_msg("staff_name"));
    return;
  }
  if (p->email[0] == -1)
    tell_player(p, " You have declared that you have no email address.\n");
  else
  {
    sprintf(stack, " Your email address is set to :\n%s\n", p->email);
    stack = end_string(oldstack);
    tell_player(p, oldstack);
  }
  if (p->custom_flags & FRIEND_EMAIL)
    tell_player(p, " Only your friends can see your email address.\n");
  else if (p->custom_flags & PRIVATE_EMAIL)
    tell_player(p, " Your email is private.\n");
  else
    tell_player(p, " Your email is public for all to read.\n");
  stack = oldstack;
}

/* change whether an email is public or private */

void public_com(player * p, char *str)
{
  tell_player(p, "This command is no longer used -- Use TOGGLE instead.\n");
}

/* password changing routines */

char *do_crypt(char *entered, player * p)
{
  char key[9];

  strncpy(key, entered, 8);
  return crypt(key, p->lower_name);
}


void got_password2(player * p, char *str)
{
  password_mode_off(p);
  p->input_to_fn = 0;
  p->flags |= PROMPT;
  if (strcmp(p->password_cpy, str))
  {
    tell_player(p, "\n But that doesn't match !!!\n"
		" Password not changed ...\n");
  }
  else
  {
    strcpy(p->password, do_crypt(str, p));
    tell_player(p, "\n Password has now been changed.\n");
    if (p->email[0] != 2)
      p->residency &= ~NO_SYNC;
    save_player(p);
  }
}

void got_password1(player * p, char *str)
{
  if (strlen(str) > (MAX_PASSWORD - 3))
  {
    do_prompt(p, "\n Password too long, please try again.\n"
	      " Please enter a shorter password: ");
    p->input_to_fn = got_password1;
  }
  else
  {
    strcpy(p->password_cpy, str);
    do_prompt(p, "\n Enter password again to verify: ");
    p->input_to_fn = got_password2;
  }
}

void validate_password(player * p, char *str)
{
  if (!check_password(p->password, str, p))
  {
    tell_player(p, "\n Hey ! thats the wrong password !!\n");
    password_mode_off(p);
    p->input_to_fn = 0;
    p->flags |= PROMPT;
  }
  else
  {
    do_prompt(p, "\n Now enter a new password: ");
    p->input_to_fn = got_password1;
  }
}

void change_password(player * p, char *str)
{
  if (p->residency == NON_RESIDENT)
  {
    TELLPLAYER(p, " You may only set a password once resident.\n"
	       " To become a resident, please ask a %s.\n",
	       get_config_msg("staff_name"));
    return;
  }
  password_mode_on(p);
  p->flags &= ~PROMPT;
  if (p->password[0] && p->password[0] != -1)
  {
    do_prompt(p, " Please enter your current password: ");
    p->input_to_fn = validate_password;
  }
  else
  {
    do_prompt(p, " You have no password.\n"
	      " Please enter a password: ");
    p->input_to_fn = got_password1;
  }
}



/* show wrap info */

void check_wrap(player * p, char *str)
{
  char *oldstack;

  oldstack = stack;
  if (p->term_width)
  {
    sprintf(stack, " Line wrap on, with terminal width set to %d "
	    "characters.\n",
	    p->term_width);
    stack = strchr(stack, 0);
    if (p->word_wrap)
      sprintf(stack, " Word wrap is on, with biggest word size "
	      "set to %d characters.\n",
	      p->word_wrap);
    else
      strcpy(stack, " Word wrap is off.\n");
    stack = end_string(stack);
    tell_player(p, oldstack);
  }
  else
    tell_player(p, " Line wrap and word wrap turned off.\n");
  stack = oldstack;
}

/* Toggle the ignoreprefix flag () */

void ignoreprefix(player * p, char *str)
{
  if (!strcasecmp("off", str))
    p->custom_flags &= ~NOPREFIX;
  else if (!strcasecmp("on", str))
    p->custom_flags |= NOPREFIX;
  else
    p->custom_flags ^= NOPREFIX;

  if (p->custom_flags & NOPREFIX)
    tell_player(p, " You are now ignoring prefixes.\n");
  else
    tell_player(p, " You are now seeing prefixes.\n");
}


/* Toggle the ignore emote flag () */
void ignoreemoteprefix(player * p, char *str)
{
  if (!strcasecmp("off", str))
    p->custom_flags &= ~NOEPREFIX;
  else if (!strcasecmp("on", str))
    p->custom_flags |= NOEPREFIX;
  else
    p->custom_flags ^= NOEPREFIX;

  if (p->custom_flags & NOEPREFIX)
    tell_player(p, " You are now ignoring prefixes specifically on emotes\n");
  else
    tell_player(p, " You are now seeing prefixes specifically on emotes again.\n");
}


void set_time_delay(player * p, char *str)
{
  int diff;
  char *oldstack;

  oldstack = stack;
  if (!*str)
  {
    if (p->jetlag)
      sprintf(stack, " Your time difference is currently set at %d hours.\n",
	      p->jetlag);
    else
      sprintf(stack, " Your time difference is not currently set.\n");
    stack = end_string(stack);
    tell_player(p, oldstack);
    stack = oldstack;
    return;
  }
  diff = atoi(str);
  if (!diff)
  {
    tell_player(p, " Time difference reset to 0 hours.\n");
    p->jetlag = 0;
    return;
  }

  if (diff < -23 || diff > 23)
  {
    tell_player(p, " That's a bit silly, isn't it?\n");
    return;
  }
  p->jetlag = diff;

  oldstack = stack;
  if (p->jetlag == 1)
    strcpy(stack, " Time Difference set to 1 hour.\n");
  else
    sprintf(stack, " Time Difference set to %d hours.\n", p->jetlag);
  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
}

void set_ignore_msg(player * p, char *str)
{
  if (!*str)
  {
    tell_player(p, " You reset your ignore message.\n");
    strcpy(p->ignore_msg, "");
    return;
  }
  strncpy(p->ignore_msg, str, MAX_IGNOREMSG - 3);
  TELLPLAYER(p, " You set your ignore message to ... %s\n", p->ignore_msg);
}

void set_logonmsg(player * p, char *str)
{
  if (p->system_flags & NO_MSGS)
  {
    tell_player(p, " Sorry, but you have been prevented from changing this.\n");
    return;
  }

  if (!*str)
    strncpy(p->logonmsg, get_config_msg("def_login"), MAX_ENTER_MSG - 3);
  else
    strncpy(p->logonmsg, str, MAX_ENTER_MSG - 3);

  if (emote_no_break(*str))
    TELLPLAYER(p, " You set your logonmsg to ...\n%s%s\n", p->name, p->logonmsg);
  else
    TELLPLAYER(p, " You set your logonmsg to ...\n%s %s\n", p->name, p->logonmsg);
}


void set_logoffmsg(player * p, char *str)
{
  if (p->system_flags & NO_MSGS)
  {
    tell_player(p, " Sorry, but you have been prevented from changing this.\n");
    return;
  }

  if (!*str)
    strncpy(p->logoffmsg, get_config_msg("def_logout"), MAX_ENTER_MSG - 3);
  else
    strncpy(p->logoffmsg, str, MAX_ENTER_MSG - 3);

  if (emote_no_break(*str))
    TELLPLAYER(p, " You set your logoffmsg to ...\n%s%s\n", p->name, p->logoffmsg);
  else
    TELLPLAYER(p, " You set your logoffmsg to ...\n%s %s\n", p->name, p->logoffmsg);
}


void set_blockmsg(player * p, char *str)
{
  strncpy(p->blockmsg, str, MAX_IGNOREMSG - 3);
  if (emote_no_break(*str))
    TELLPLAYER(p, " You set your blockmsg to ...\n{%s%s}\n", p->name, p->blockmsg);
  else
    TELLPLAYER(p, " You set your blockmsg to ...\n{%s %s}\n", p->name, p->blockmsg);
}


void set_exitmsg(player * p, char *str)
{
  if (p->system_flags & NO_MSGS)
  {
    tell_player(p, " Sorry, but you have been prevented from changing this.\n");
    return;
  }
  strncpy(p->exitmsg, str, MAX_ENTER_MSG - 3);
  if (emote_no_break(*str))
    TELLPLAYER(p, " You set your exitmsg to ...\n%s%s\n", p->name, p->exitmsg);
  else
    TELLPLAYER(p, " You set your exitmsg to ...\n%s %s\n", p->name, p->exitmsg);
}


void set_irl_name(player * p, char *str)
{
  if (p->system_flags & NO_MSGS)
  {
    tell_player(p, " Sorry, but you have been prevented from changing this.\n");
    return;
  }
  strncpy(p->irl_name, str, MAX_NAME - 3);
  TELLPLAYER(p, " You set your irl name to be: %s\n", p->irl_name);
}


/* decided to make this URL instead.. bear with and watch the 
   old 'alt_email' references until I fix it :P   -traP */

void set_alt_email(player * p, char *str)
{
  if (p->system_flags & NO_MSGS)
  {
    tell_player(p, " Sorry, but you have been prevented from changing this.\n");
    return;
  }
  strncpy(p->alt_email, str, MAX_EMAIL - 3);
  TELLPLAYER(p, " You set your URL to ...\n%s\n", p->alt_email);
}

void set_hometown(player * p, char *str)
{
  char *oldstack;

  if (p->system_flags & NO_MSGS)
  {
    tell_player(p, " Sorry, but you have been prevented from changing this.\n");
    return;
  }
  oldstack = stack;
  strncpy(p->hometown, str, MAX_SPODCLASS - 3);
  sprintf(stack, " You set your hometown to ...\n%s\n", p->hometown);
  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
}

void set_icq(player * p, char *str)
{
  if (p->system_flags & NO_MSGS)
  {
    tell_player(p, " Sorry, but you have been prevented from changing this.\n");
    return;
  }

  if (!*str)
  {
    tell_player(p, " Format: icq <number>\n"
		"   (use 0 to blank)\n");
    return;
  }

  p->icq = atoi(str);

  if (p->icq)
    TELLPLAYER(p, " You set your ICQ number to %d.\n", p->icq);
  else
    tell_player(p, " You blank your ICQ number.\n");
}


void set_spod_class(player * p, char *str)
{
  char *oldstack;

  if (p->system_flags & NO_MSGS)
  {
    tell_player(p, " Sorry, but you have been prevented from changing this.\n");
    return;
  }

  if (strstr(str, "^") || strstr(str, "&"))
  {
    tell_player(p, " You may not have colour or dynatext in your spod class.\n");
    return;
  }

  oldstack = stack;
  strncpy(p->spod_class, str, MAX_SPODCLASS - 3);
  sprintf(stack, " You set your spod_class to '%s'\n", p->spod_class);
  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
}



void set_favorites(player * p, char *str)
{
  char *oldstack, firster[80] = "", *nexter, *arg_two;
  int choice;

  if (p->system_flags & NO_MSGS)
  {
    tell_player(p, " Sorry, but you have been prevented from changing this.\n");
    return;
  }
  if (!*str)
  {
    tell_player(p, " Type 'help favorite' for how to use this.\n");
    return;
  }
  arg_two = next_space(str);
  if (*arg_two)
    *arg_two++ = 0;

  if (!strcasecmp(str, "list"))
  {
    oldstack = stack;

    sprintf(stack, " Your favorites are set to...\n");
    stack = strchr(stack, 0);
    strcpy(firster, "");
    if (*p->favorite1)
    {
      strcpy(firster, p->favorite1);
      nexter = next_space(firster);
      *nexter++ = 0;
      sprintf(stack, " 1. %-12.12s : %s\n", firster, nexter);
    }
    else
      strcpy(stack, "1. Not set.\n");
    stack = strchr(stack, 0);
    strcpy(firster, "");

    if (*p->favorite2)
    {
      strcpy(firster, p->favorite2);
      nexter = next_space(firster);
      *nexter++ = 0;
      sprintf(stack, " 2. %-12.12s : %s\n", firster, nexter);
    }
    else
      strcpy(stack, " 2. Not set.\n");
    stack = strchr(stack, 0);
    strcpy(firster, "");
    if (*p->favorite3)
    {
      strcpy(firster, p->favorite3);
      nexter = next_space(firster);
      *nexter++ = 0;
      sprintf(stack, " 3. %-12.12s : %s\n", firster, nexter);
    }
    else
      strcpy(stack, " 3. Not set.\n");
    stack = strchr(stack, 0);

    stack = end_string(stack);
    tell_player(p, oldstack);
    stack = oldstack;
    return;
  }
  if (*arg_two && !strcasecmp(arg_two, "blank"))
  {
    /* arg 1 should be a number */

    choice = atoi(str);
    if ((!*arg_two) || choice < 1 || choice > 3)
    {
      tell_player(p, " Format: favorite <#> blank  (# = 1 2 or 3)\n");
      return;
    }
    switch (choice)
    {
      case 1:
	p->favorite1[0] = 0;
	break;
      case 2:
	p->favorite2[0] = 0;
	break;
      case 3:
	p->favorite3[0] = 0;
	break;
      default:
	tell_player(p, " Erk,,, got a bug here...\n");
	log("error", "Error in set_favorite");
	return;
    }
    tell_player(p, " OK, blanked.\n");
    return;
  }
  /* ok... last variation... */
  /* arg 1 is a number, arg 2 is what to set it too... */
  choice = atoi(str);
  if (choice < 1 || choice > 3)
  {
    tell_player(p, " Format: favorite <#> <set to> (# = 1 2 or 3)\n");
    return;
  }


  oldstack = stack;
  switch (choice)
  {
    case 1:
      strncpy(p->favorite1, arg_two, MAX_SPODCLASS - 3);
      strcpy(firster, p->favorite1);
      nexter = next_space(firster);
      *nexter++ = 0;
      if (strstr(firster, "^"))
      {
	tell_player(p, " Sorry, but you can't have colours in your favourite title.\n");
	p->favorite1[0] = 0;
	return;
      }

      sprintf(stack, " You set your favorite %.12s to: %s\n", firster, nexter);
      break;
    case 2:
      strncpy(p->favorite2, arg_two, MAX_SPODCLASS - 3);
      strcpy(firster, p->favorite2);
      nexter = next_space(firster);
      *nexter++ = 0;
      if (strstr(firster, "^"))
      {
	tell_player(p, " Sorry, but you can't have colours in your favourite title.\n");
	p->favorite1[0] = 0;
	return;
      }

      sprintf(stack, " You set your favorite %.12s to: %s\n", firster, nexter);
      break;
    case 3:
      strncpy(p->favorite3, arg_two, MAX_SPODCLASS - 3);
      strcpy(firster, p->favorite3);
      nexter = next_space(firster);
      *nexter++ = 0;
      if (strstr(firster, "^"))
      {
	tell_player(p, " Sorry, but you can't have colours in your favourite title.\n");
	p->favorite1[0] = 0;
	return;
      }
      sprintf(stack, " You set your favorite %.12s to: %s\n", firster, nexter);
      break;
    default:
      log("error", "Error in setting a favorite...");
      return;
  }

  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
}


/* Screen locking routines... spoon of password, basically... 
   (Erm, not really since it didn't use the persons password at all ...
   ... however here is one that does -- Silver) */

void unlock_screen(player * p, char *str)
{

  if (!check_password(p->password, str, p))
  {
    do_prompt(p, " Please enter CORRECT password to unlock screen: ");
    p->input_to_fn = unlock_screen;
  }
  else
  {
    password_mode_off(p);
    p->input_to_fn = 0;
    p->flags |= PROMPT;
    tell_player(p, " Screen is now unlocked.\n");
  }

}

/* The all new screenlock command! Sort of --Silver */

void set_screenlock(player * p, char *str)
{

  password_mode_on(p);
  p->flags &= ~PROMPT;

  do_prompt(p, "\n Screen is now locked - No one may type any commands"
	    " without unlocking it with your password.\n"
	    " Please enter your password to unlock screen: ");
  p->input_to_fn = unlock_screen;
}

void toggle_email_site_flags(player * p, char *str)
{

  char *opt;
  int em = 0, si = 0;
  int pri = 0, pub = 0, fri = 0;

  opt = next_space(str);
  *opt++ = 0;

  if (!*str || !*opt)
  {
    tell_player(p, " Format: toggle <email/site/both> <public/private/friend>\n");
    return;
  }
  if (!strcasecmp(str, "email"))
    em = 1;
  else if (!strcasecmp(str, "site"))
    si = 1;
  else if (!strcasecmp(str, "both"))
  {
    em = 1;
    si = 1;
  }
  else
  {
    tell_player(p, " Format: toggle <email/site/both> <public/private/friend>\n");
    return;
  }

  if (!strcasecmp(opt, "public"))
    pub = 1;
  else if (!strcasecmp(opt, "private"))
    pri = 1;
  else if (!strcasecmp(opt, "friend"))
    fri = 1;
  else
  {
    tell_player(p, " Format: toggle <email/site/both> <public/private/friend>\n");
    return;
  }

  if (em)
  {

    if (fri)
    {
      tell_player(p, " Setting email to friend viewable only.\n");
      p->custom_flags |= PRIVATE_EMAIL;
      p->custom_flags |= FRIEND_EMAIL;
    }
    else if (pub)
    {
      tell_player(p, " Setting email as public for all to see.\n");
      p->custom_flags &= ~PRIVATE_EMAIL;
      p->custom_flags &= ~FRIEND_EMAIL;
    }
    else
    {
      tell_player(p, " Setting email as private. Only you and the admin will be able to see it.\n");
      p->custom_flags |= PRIVATE_EMAIL;
      p->custom_flags &= ~FRIEND_EMAIL;
    }
  }
  if (si)
  {
    if (fri)
    {
      tell_player(p, " Setting site to friend viewable only.\n");
      p->custom_flags &= ~PUBLIC_SITE;
      p->custom_flags |= FRIEND_SITE;
    }
    else if (pub)
    {
      tell_player(p, " Setting site as public for all to see.\n");
      p->custom_flags |= PUBLIC_SITE;
      p->custom_flags &= ~FRIEND_SITE;
    }
    else
    {
      tell_player(p, " Setting site as private. Only you and the admin will be able to see it.\n");
      p->custom_flags &= ~PUBLIC_SITE;
      p->custom_flags &= ~FRIEND_SITE;
    }
  }
}

void newsetpw0(player * p, char *str)
{
  p->flags &= ~PROMPT;
  password_mode_on(p);
  {
    do_prompt(p, " I am now going to ask you to set a password on your character.\n"
	" Pick something not easy to guess, but easy for you to remember.\n"
	      " \n Please enter a new password: ");
    p->input_to_fn = newsetpw1;
  }
}

void begin_ressie(player * p, char *str)
{

  p->flags &= ~PROMPT;

  tell_player(p, " We will start with your EMAIL address. If you have more than one please use the one that you check the most regulary.\n\n");

  do_prompt(p, "Enter your email address now: ");
  p->input_to_fn = begin_ressie_doemail;
}

void newsetpw2(player * p, char *str)
{
  password_mode_off(p);
  p->input_to_fn = 0;
  p->flags |= PROMPT;
  if (strcmp(p->password_cpy, str))
  {
    tell_player(p, "\n But that doesn't match !!!\n"
		" Password not changed ...\n");
    p->flags &= ~PROMPT;
    password_mode_on(p);
    do_prompt(p, " Please enter a new password: ");
    p->input_to_fn = newsetpw1;
  }
  else
  {
    strcpy(p->password, do_crypt(str, p));
    tell_player(p, "\n Password has now been changed.\n");
    if (p->email[0] != 2)
      p->residency &= ~NO_SYNC;
    save_player(p);
    strncpy(p->saved->email, p->email, MAX_EMAIL - 3);
  }
}

void newsetpw1(player * p, char *str)
{
  if (strlen(str) > (MAX_PASSWORD - 3))
  {
    do_prompt(p, "\n Password too long, please try again.\n"
	      " Please enter a shorter password: ");
    p->input_to_fn = newsetpw1;
  }
  else if (*str)
  {
    strcpy(p->password_cpy, str);
    do_prompt(p, "\n Enter password again to verify: ");
    p->input_to_fn = newsetpw2;
  }
  else
  {
    do_prompt(p, "\n\n Passwords of zero length aren't very secure. Please try again.\n");
    do_prompt(p, " Please enter a new password: ");
    p->input_to_fn = newsetpw1;
  }
}

void set_made_from(player * p, char *str)
{
  char *oldstack;

  if (p->system_flags & NO_MSGS)
  {
    tell_player(p, " Sorry, but you have been prevented from changing this.\n");
    return;
  }
  oldstack = stack;
  strncpy(p->ingredients, str, MAX_SPODCLASS - 3);
  sprintf(stack, " You state that you are made from...\n%s\n", p->ingredients);
  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
}

/* Mailing list stuff */

void calc_mailinglist(player * p, char *str)
{
  saved_player *scanlist, **hlist;
  int counter, charcounter, accept = 0, reject = 0;
  char *oldstack;

  tell_player(p, "\n Scanning for emails ...\n");
  unlink("reports/emails.rpt");
  oldstack = stack;
  *stack = ' ';
  for (charcounter = 0; charcounter < 26; charcounter++)
  {
    hlist = saved_hash[charcounter];
    for (counter = 0; counter < HASH_SIZE; counter++, hlist++)
      for (scanlist = *hlist; scanlist; scanlist = scanlist->next)
      {
	switch (scanlist->residency)
	{
	  case BANISHED:
	  case BANISHD:
	  case SYSTEM_ROOM:
	    break;
	  default:
	    if (strlen(scanlist->email) > 0)
	    {
	      sprintf(stack, "%s\n", scanlist->email);
	      stack = strchr(stack, 0);
	      accept++;
	    }
	    else
	    {
	      TELLPLAYER(p, " %s has no email address set. Skipping.\n", scanlist->lower_name);
	      reject++;
	    }
	    break;
	}
      }
  }
  *stack++ = 0;
  sendtofile("emails", oldstack);
  stack = oldstack;

  TELLPLAYER(p, " Email report generated (%d listed and %d skipped)\n\n", accept, reject);
}

/* Turns clock on and off */

void toggle_view_clock(player * p, char *str)
{
  p->custom_flags ^= NO_CLOCK;

  if (p->custom_flags & NO_CLOCK)
    TELLPLAYER(p, " You are now ignoring the %s clock.\n",
	       get_config_msg("talker_name"));
  else
    TELLPLAYER(p, " You are now seeing the %s clock.\n",
	       get_config_msg("talker_name"));
}

void toggle_yes_dynatext(player * p, char *str)
{
  p->custom_flags ^= NO_DYNATEXT;

  if (p->custom_flags & NO_DYNATEXT)
    TELLPLAYER(p, " You are now ignoring all dynatext.\n");
  else
    TELLPLAYER(p, " You are now seeing all dynatext.\n");
}


/* Tonhe's nwho command - type "w" on the talker to see it */
/* Modified for PG+ rather heavily (removed colour and cleaned up
   presentation and odd bits of code *grin*) */

void nwho(player * p, char *str)
{
  char *oldstack;
  char middle[80];
  player *scan;
  int page, pages, count;

  oldstack = stack;
  page = atoi(str);
  if (page <= 0)
    page = 1;
  page--;
  pages = (current_players - 1) / (TERM_LINES - 2);
  if (page > pages)
    page = pages;

  sprintf(stack, "--- [Name] ----------- [Position] --------- [Idle] "
	  "------- [Location] -----");
  stack = strchr(stack, 0);
  strcpy(stack, "\n");
  stack = strchr(stack, 0);

  count = page * (TERM_LINES - 2);
  for (scan = flatlist_start; count; scan = scan->flat_next)
    if (!scan)
    {
      tell_player(p, " Bad who listing, abort.\n");
      log("error", "Bad who list!!");
      stack = oldstack;
      return;
    }
    else if (scan->name[0])
      count--;

  for (count = 0; (count < (TERM_LINES - 1) && scan); scan = scan->flat_next)
  {
    if (scan->location)
    {
      sprintf(stack, "    %-18s ", scan->name);
      stack = strchr(stack, 0);
#ifdef ROBOTS
      if (scan->residency & ROBOT_PRIV)
	strcpy(stack, "Robot             ");
      else
#endif
      if (((scan->residency & CODER) && (!(scan->flags & BLOCK_SU))) && (!(scan->flags & OFF_LSU)))
	sprintf(stack, "%-18.18s", get_config_msg("coder_name"));

      else if (((scan->residency & HCADMIN) && (!(scan->flags & BLOCK_SU))) && (!(scan->flags & OFF_LSU)))
	sprintf(stack, "%-18.18s", get_config_msg("hc_name"));

      else if (((scan->residency & ADMIN) && (!(scan->flags & BLOCK_SU))) && (!(scan->flags & OFF_LSU)))
	sprintf(stack, "%-18.18s", get_config_msg("admin_name"));

      else if (((scan->residency & LOWER_ADMIN) && (!(scan->flags & BLOCK_SU))) && (!(scan->flags & OFF_LSU)))
	sprintf(stack, "%-18.18s", get_config_msg("la_name"));

      else if (((scan->residency & ASU) && (!(scan->flags & BLOCK_SU))) && (!(scan->flags & OFF_LSU)))
	sprintf(stack, "%-18.18s", get_config_msg("asu_name"));

      else if (((scan->residency & SU) && (!(scan->flags & BLOCK_SU))) && (!(scan->flags & OFF_LSU)))
	sprintf(stack, "%-18.18s", get_config_msg("su_name"));

      else if (((scan->residency & PSU) && (!(scan->flags & BLOCK_SU))) && (!(scan->flags & OFF_LSU)))
	sprintf(stack, "%-18.18s", get_config_msg("psu_name"));

      else if (scan->residency)
	sprintf(stack, "Resident          ");
      else
	sprintf(stack, "Newbie            ");
      stack = strchr(stack, 0);

/* idle time */

      sprintf(stack, " %3d:%.2d          ", scan->idle / 60, scan->idle % 60);
      stack = strchr(stack, 0);

/* location */

      if ((!strcmp(scan->location->owner->lower_name, SYS_ROOM_OWNER) ||
	   !strcmp(scan->location->owner->lower_name, "system")))
      {
	if (!strcmp(scan->location->id, "room"))
	  sprintf(stack, " main      ");
	else
	  sprintf(stack, " %-10s", scan->location->id);
	stack = strchr(stack, 0);
      }
      else if ((scan->custom_flags & HIDING) && (!(p->saved_residency & ADMIN)))
      {
	strcpy(stack, " *Hiding* ");
	stack = strchr(stack, 0);
      }
      else if (!strcmp(scan->location->owner->lower_name,
		       scan->lower_name))
      {
	strcpy(stack, " home ");
	stack = strchr(stack, 0);
      }
      else
      {
	sprintf(stack, " %s's", scan->location->owner->lower_name);
	stack = strchr(stack, 0);
      }

/* end the line */
      strcpy(stack, "\n");
      stack = strchr(stack, 0);
    }
  }				/* end the loop */

  sprintf(middle, "Page %d of %d", page + 1, pages + 1);
  pstack_mid(middle);

  *stack++ = 0;
  tell_player(p, oldstack);
  stack = oldstack;

}

void timeprompt(player * p, char *str)
{
  if (!strcasecmp(str, "on"))
    p->system_flags |= TIMEPROMPT;
  else if (!strcasecmp(str, "off"))
    p->system_flags &= ~TIMEPROMPT;
  else
    p->system_flags ^= TIMEPROMPT;

  if (p->system_flags & TIMEPROMPT)
    tell_player(p, " Time prompting activated.\n");
  else
    tell_player(p, " Time prompting deactivated.\n");
}

/* Quit with message */

void quit_with_message(player * p, char *str)
{
  if (!*str)
  {
    tell_player(p, " Format: mquit <message>\n");
    return;
  }

  if (strlen(str) > MAX_MQUIT)
  {
    tell_player(p, " That message is a little too long. Please try to shorten it.\n");
    return;
  }

  strncpy(p->finger_message, str, MAX_MQUIT);
  byebye(p, 0);
}

void list_creators(player * p, char *str)
{
  char *oldstack = stack;
  player *scan;
  int count = 0;
  char temp[80];

  sprintf(temp, "Creators on %s", get_config_msg("talker_name"));

  pstack_mid(temp);
  for (scan = flatlist_start; scan; scan = scan->flat_next)
  {
    if (scan->location && *(scan->name) &&
	scan->residency & SPECIALK)
    {
      count++;
      stack += sprintf(stack, "   %-18s", scan->name);
      if (!(count % 3))
	stack += sprintf(stack, "\n");
    }
  }
  if (count % 3)
    stack += sprintf(stack, "\n");

  if (!count)
  {
    stack = oldstack;
    tell_player(p, " There aren't any Creators on atm.\n");
    return;
  }
  else if (count > 1)
    sprintf(temp, "There are %d creators connected", count);
  else
    sprintf(temp, "There is 1 creator connected");

  pstack_mid(temp);
  *stack++ = 0;

  tell_player(p, oldstack);
  stack = oldstack;

}


/* email command */

void change_email(player * p, char *str)
{

  if (p->residency == NON_RESIDENT)
  {
    TELLPLAYER(p, " You may only use the email command once resident.\n"
	       " Please ask a %s to grant you residency.\n",
	       get_config_msg("staff_name"));
    return;
  }
  if (!*str)
  {
    check_email(p, str);
    return;
  }
  if (!strcasecmp(str, "private"))
  {
    p->custom_flags |= PRIVATE_EMAIL;
    tell_player(p, " Your email is private, only the Admin will be able to see it.\n");
    return;
  }
  if (!strcasecmp(str, "public"))
  {
    p->custom_flags &= ~PRIVATE_EMAIL;
    tell_player(p, " Your email address is public, so everyone can see it.\n");
    return;
  }
  if (set_players_email(p, str) < 0)
  {
    tell_player(p, " Email Not changed ...\n");
    return;
  }
  strncpy(p->email, str, MAX_EMAIL - 3);

  p->residency &= ~NO_SYNC;
  save_player(p);
}

void begin_ressie_doemail(player * p, char *str)
{
  char *oldstack;

  oldstack = stack;
  if (set_players_email(p, str) < 0)
  {
    do_prompt(p, " Please enter your email address now: ");
    p->input_to_fn = begin_ressie_doemail;
    return;
  }
  p->input_to_fn = 0;
  p->flags |= PROMPT;
  newsetpw0(p, 0);
}


char *is_invalid_email(char *email)
{
  char *where = email;

  while (*where++)
  {
    switch (*where)
    {
      case '\'':
      case '!':
      case ' ':
      case ';':
      case '*':
      case '#':
      case '&':
      case '(':
      case ')':
      case '<':
      case '>':
      case '|':
      case '"':
	return where;
    }
  }
  return (char *) NULL;
}

int set_players_email(player * p, char *str)
{
  char *oldstack = stack, *inval, olde[MAX_EMAIL];
  struct stat sbuf;

  if (!*str)
  {
    tell_player(p, " Hello... you must have some kinda email address...\n");
    return -1;
  }

  if ((inval = is_invalid_email(str)))
  {
    TELLPLAYER(p, " Sorry, you may not have a - %c - in your email\n", *inval);
    return -1;
  }

  str[MAX_EMAIL] = '\0';

  if (!(inval = strchr(str, '@')))
  {
    tell_player(p, " You must have a '@' in your email address.\n");
    return -1;
  }
  if (!(strchr(inval, '.')))
  {
    tell_player(p, " You must have a '.' in your email address.\n");
    return -1;
  }

  /* ok, new str is kosher for email... lets set eeet */

  if (!p->email[0])
  {
    LOGF("help", "%s %s (new email set)", p->name, str);
    strcpy(olde, "<BLANK>");
  }
  else if (p->email[0] == ' ')
    strcpy(olde, "<VALIDATED>");
  else if (p->email[0] == 2)
    strcpy(olde, "<NOT YET SET>");
  else
    strncpy(olde, p->email, MAX_EMAIL - 1);

  strncpy(p->email, str, MAX_EMAIL - 1);
  if (p->saved)
    strncpy(p->saved->email, str, MAX_EMAIL - 1);

  TELLPLAYER(p, " Your email address is set to : %s\n", p->email);
  LOGF("emails", "%s email set to %s\n                      (was %s)",
       p->name, p->email, olde);
  AUWALL(" -=*> %s sets %s email to %s\n", p->name,
	 gstring_possessive(p), p->email);

  if (config_flags & cfWELCOMEMAIL && !(p->residency & (PSU | SU | ADC)))
  {
    if (stat("files/email_welcome.msg", &sbuf) < 0)
    {
      log("error", "Need a files/email_welcome.msg !!!");
      return 0;
    }
/*
   sprintf(stack,
   "mail -s 'Welcome message to %s@%s' %s < files/email_welcome.msg",
   p->name, p->inet_addr, p->email);
 */
    sprintf(stack, get_config_msg("emailer_call"), p->name,
	    p->inet_addr, p->email);


    stack = end_string(stack);
    system(oldstack);
    stack = oldstack;
    tell_player(p, " (which is being checked now)\n");
  }
  return 0;
}
