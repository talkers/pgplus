/*
 * Playground+ - session.c
 * Session code
 * ---------------------------------------------------------------------------
 */

#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <stdio.h>

#include "include/config.h"
#include "include/player.h"
#include "include/fix.h"
#include "include/proto.h"

void set_session(player * p, char *str)
{
  char *oldstack;
  time_t t;
  player *scan;
  int wait;

  oldstack = stack;

  if (p->custom_flags & YES_SESSION)
  {
    tell_player(p, " You are ignoring sessions - type \"muffle session\" to toggle.\n");
    return;
  }
  t = time(0);
  if (session_reset)
    wait = session_reset - (int) t;
  else
    wait = 0;
  if (wait < 0)
    wait = 0;
  if (strlen(session) == 0)
  {
    strncpy(session, get_session_msg("default_session"), MAX_SESSION - 3);
    strncpy(sess_name, get_session_msg("default_setter"), MAX_NAME - 1);
  }
  if (!*str)
  {
    sprintf(stack, " The session is currently '%s'\n", session);
    stack = strchr(stack, 0);
    sprintf(stack, "  It was set by %s, and can be reset in %s.\n",
	    sess_name, word_time(wait));
    stack = end_string(stack);
    tell_player(p, oldstack);
    stack = oldstack;
    return;
  }
  if (wait > 0 /* && p != p_sess */ )
  {
    sprintf(stack, " Session can be reset in %s\n", word_time(wait));
    stack = end_string(stack);
    tell_player(p, oldstack);
    stack = oldstack;
    return;
  }
  if (config_flags & cfNOSWEAR)
    str = filter_rude_words(str);


  if (strlen(str) > 55)
  {
    tell_player(p, " Too long a session name ...\n");
    stack = oldstack;
    return;
  }
  if (strstr(str, "^"))
  {
    tell_player(p, " Sorry, but you can't have colours in a session title.\n");
    return;
  }
  if (strstr(str, "&"))
  {
    tell_player(p, " Sorry, but you can't have dynatext in a session title.\n");
    return;
  }



  sprintf(session, "%s", str);
  sprintf(stack, " You reset the session message to be '%s^N'\n", str);
  stack = end_string(stack);
  tell_player(p, oldstack);

  /* reset comments */
  for (scan = flatlist_start; scan; scan = scan->flat_next)
    strncpy(scan->comment, "", MAX_COMMENT - 3);

  stack = oldstack;
  sprintf(stack, "%s sets the session to be '%s'\n", p->name,
	  str);
  stack = end_string(stack);

  command_type |= EVERYONE;

  for (scan = flatlist_start; scan; scan = scan->flat_next)
    if (scan != p && !(scan->custom_flags & YES_SESSION))
      tell_player(scan, oldstack);

  stack = oldstack;

  if (strcmp(sess_name, p->name) || wait <= 0)
    session_reset = t + (60 * atoi(get_session_msg("session_reset_time")));
  p_sess = p;
  strcpy(sess_name, p->name);

  sprintf(stack, "%s- %s", p->name, session);
  stack = end_string(stack);
  log("session", oldstack);
  stack = oldstack;
}



void reset_session(player * p, char *str)
{
  char *oldstack = stack;
  player *scan;

  if (config_flags & cfNOSWEAR)
    str = filter_rude_words(str);

  if (strlen(str) > 55)
  {
    tell_player(p, " Too long a session name ...\n");
    stack = oldstack;
    return;
  }

  if (*str)
  {
    if (strstr(str, "^"))
      sprintf(session, "%s^N", str);
    else
      sprintf(session, "%s", str);
    sprintf(stack, " You reset the session message to be '%s^N'\n", str);
    stack = end_string(stack);
    tell_player(p, oldstack);

    /* reset comments */
    for (scan = flatlist_start; scan; scan = scan->flat_next)
      strncpy(scan->comment, "", MAX_COMMENT - 3);

    stack = oldstack;
    sprintf(stack, "%s resets the session to be '%s'\n", p->name, str);
    stack = end_string(stack);

    command_type |= EVERYONE;

    for (scan = flatlist_start; scan; scan = scan->flat_next)
      if (scan != p && !(scan->custom_flags & YES_SESSION))
	tell_player(scan, oldstack);

    stack = oldstack;

    p_sess = p;
    strcpy(sess_name, p->name);

    sprintf(stack, "%s- %s", p->name, session);
    stack = end_string(stack);
    log("session", oldstack);
    stack = oldstack;
  }
  session_reset = 0;
  tell_player(p, " Session timer reset.\n");
}

void set_comment(player * p, char *str)
{
  char *oldstack;
  player *scan;

  oldstack = stack;

  if (!*str)
  {
    tell_player(p, " You reset your session comment.\n");
    strncpy(p->comment, "", MAX_COMMENT - 3);
    return;
  }
  if (p->custom_flags & YES_SESSION)
  {
    tell_player(p, " You are ignoring sessions - type \"muffle session\" to toggle.\n");
    return;
  }
  if (strstr(str, "^"))
  {
    tell_player(p, " Sorry, but you can't have colour in your session comment.\n");
    return;
  }
  if (strstr(str, "&"))
  {
    tell_player(p, " Sorry, but you can't have dynatext in your session comment.\n");
    return;
  }

  if (config_flags & cfNOSWEAR)
    str = filter_rude_words(str);

  strncpy(p->comment, str, MAX_COMMENT - 3);
  sprintf(stack, " You set your session comment to be '%s'\n", p->comment);
  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;

  /* Inform everyone else who is listening to session information */

  for (scan = flatlist_start; scan; scan = scan->flat_next)
    if (scan != p && !(scan->custom_flags & YES_SESSION))
      TELLPLAYER(scan, " -=*> %s sets %s session comment to be '%s'\n", p->name, gstring_possessive(p), p->comment);

}

/* new version of view_session, to try and show only those who have commented
   Hope this works... */

void comments(player * p, char *str)
{
  player *scan, *start;
  int pages = 1, page, line;
  char *oldstack, middle[80];

  oldstack = stack;

  if (strlen(str) < 1)
    page = 1;
  else
    page = atoi(str);

  if (p->custom_flags & YES_SESSION)
  {
    tell_player(p, " You are ignoring sessions - type \"muffle session\" to toggle.\n");
    return;
  }
  if (page < 1)
  {
    tell_player(p, " Usage : comments [<pagenumber>]\n");
    return;
  }
  scan = flatlist_start;
  start = NULL;

  line = 0;

  for (scan = flatlist_start; scan; scan = scan->flat_next)
  {
    if (pages <= page && line == 0)
      start = scan;

    if (scan->comment[0] != 0)
      line++;

    if (line > TERM_LINES - 2)
    {
      line = 0;
      pages++;
    }
  }

  if (page > pages)
    page = pages;

  if (strlen(session) < 1)
  {
    strncpy(session, get_session_msg("default_session"), MAX_SESSION - 3);
    strncpy(sess_name, get_session_msg("default_setter"), MAX_NAME - 1);
  }
  pstack_mid(session);

  line = 0;
  for (; start; start = start->flat_next)
  {
    if (line > TERM_LINES)
      break;

    if (start->comment[0] != 0)
    {
      if (!strcasecmp(sess_name, start->lower_name))
	sprintf(stack, "%-19s*", start->name);
      else
	sprintf(stack, "%-19s ", start->name);
      stack = strchr(stack, 0);

      /* Original code told people whether people who had set session
         comments had colour or other stuff on or not. Why? Its not as
         if anyone cares! --Silver */

      sprintf(stack, "- %s\n", start->comment);
      stack = strchr(stack, 0);
      line++;
    }
  }

  sprintf(middle, "Page %d of %d", page, pages);
  pstack_mid(middle);
  *stack++ = 0;
  tell_player(p, oldstack);
  stack = oldstack;
  return;
}



/* REPLYS */

/* save a list of who sent you the last list of names, for reply  */

void make_reply_list(player * p, player ** list, int matches)
{
  char *oldstack, *send, *mark, *scan;
  player **step;
  time_t t;
  int i, count, timeout;

  oldstack = stack;

  t = time(0);
  timeout = t + (2 * 60);

  if (matches < 2)
    return;

  sprintf(stack, "%s.,", p->lower_name);
  count = strlen(stack);
  stack = strchr(stack, 0);
  for (step = list, i = 0; i < matches; i++, step++)
    if (*step != p)
    {
      count += (strlen((*step)->lower_name) + 1);
      if (count < (MAX_REPLY - 2))
      {
	sprintf(stack, "%s.,", (*step)->lower_name);
	stack = strchr(stack, 0);
      }
      else
	log("reply", "Too longer reply string !!!");
    }
  stack = end_string(stack);

  /* should have string in oldstack */

  send = stack;
  for (step = list, i = 0; i < matches; i++, step++, mark = 0)
  {
    char buffer[50];

    sprintf(buffer, ".,%s.,", (*step)->lower_name);
    mark = strstr(oldstack, buffer);
    if (mark)
      mark += 2;
    if (!mark)
    {
      sprintf(buffer, "%s.,", (*step)->lower_name);
      mark = strstr(oldstack, buffer);
      if (mark != oldstack)
	mark = 0;
    }
    if (!mark)
    {
      log("reply", "Can't find player in reply string!!");
      return;
    }
    for (scan = oldstack; scan != mark;)
      *stack++ = *scan++;
    while (*scan != ',')
      scan++;
    scan++;
    while (*scan)
      *stack++ = *scan++;
    *stack = 0;
    strcpy((*step)->reply, send);
    (*step)->reply_time = timeout;
    stack = send;
  }
}

/* Reply command itself */

void reply(player * p, char *str)
{
  char *oldstack;

  oldstack = stack;

  if (!*str)
  {
    tell_player(p, " Format: reply <msg>\n");
    return;
  }
  if (!*(p->reply) || (p->reply_time < (int) time(0)))
  {
    tell_player(p, " You don't have anyone to reply to!\n");
    return;
  }
  sprintf(stack, "%s ", p->reply);
  stack = strchr(stack, 0);
  strcpy(stack, str);
  stack = end_string(stack);
  sys_flags |= REPLY_TAG;
  tell(p, oldstack);
  stack = oldstack;
  sys_flags &= ~REPLY_TAG;
}

/* And the emote reply command itself */

void ereply(player * p, char *str)
{
  char *oldstack;

  oldstack = stack;

  if (!*str)
  {
    tell_player(p, " Format: ereply <msg>\n");
    return;
  }
  if (!*(p->reply) || (p->reply_time < (int) time(0)))
  {
    tell_player(p, " You don't have anyone to reply to!\n");
    return;
  }
  sprintf(stack, "%s ", p->reply);
  stack = strchr(stack, 0);
  strcpy(stack, str);
  stack = end_string(stack);
  sys_flags |= REPLY_TAG;
  remote(p, oldstack);
  stack = oldstack;
  sys_flags &= ~REPLY_TAG;
}


void report_idea(player * p, char *str)
{
  if (!*str)
  {
    tell_player(p, " Format: idea <whatever you thought of>\n");
    return;
  }
  if (strlen(str) > 480)
  {
    tell_player(p, " Please make it a little smaller.\n");
    return;
  }
  TELLPLAYER(p, "%s%s\n%s ... logged as idea, thank you.\n", LINE, str, LINE);
  LOGF("idea", "%s - %s ", p->name, str);
  SW_BUT(p, " -=*> Idea log from %s: %s\n", p->name, str);
}

void report_error(player * p, char *str)
{
  char *spacer = (char *) NULL;	/* use NULL stead of "" */

  if (!*str)
  {
    tell_player(p, " Format: bug <whatever the bug is>\n");
    return;
  }
  if (strlen(str) > 480)
  {
    tell_player(p, " Please make it a little smaller.\n");
    return;
  }
  if ((spacer = strchr(str, ' ')))
    *spacer = '\0';

  if (find_player_global_quiet(str))
  {
    tell_player(p, " This isnt a social you gimp!\n");
    return;
  }
  if (spacer)
    *spacer = ' ';

  TELLPLAYER(p, "%s%s\n%s ... logged as a bug, thanks for your time\n",
	     LINE, str, LINE);
  LOGF("bug", "%s: %s", p->name, str);;
  SW_BUT(p, " -=*> Bug log from %s: %s\n", p->name, str);
}

void show_exits(player * p, char *str)
{
  if (p->custom_flags & SHOW_EXITS)
  {
    tell_player(p, " You won't see exits when you enter a room now.\n");
    p->custom_flags &= ~SHOW_EXITS;
  }
  else
  {
    tell_player(p, " When you enter a room you will now see the exits.\n");
    p->custom_flags |= SHOW_EXITS;
  }
}



/* And the think reply command itself */

void treply(player * p, char *str)
{
  char *oldstack;

  oldstack = stack;

  if (!*str)
  {
    tell_player(p, " Format: treply <msg>\n");
    return;
  }
  if (!*(p->reply) || (p->reply_time < (int) time(0)))
  {
    tell_player(p, " You don't have anyone to reply to!\n");
    return;
  }
  sprintf(stack, "%s ", p->reply);
  stack = strchr(stack, 0);
  strcpy(stack, str);
  stack = end_string(stack);
  sys_flags |= REPLY_TAG;
  remote_think(p, oldstack);
  stack = oldstack;
  sys_flags &= ~REPLY_TAG;
}


/* And the premote reply command itself */

void preply(player * p, char *str)
{
  char *oldstack;

  oldstack = stack;

  if (!*str)
  {
    tell_player(p, " Format: preply <msg>\n");
    return;
  }
  if (!*(p->reply) || (p->reply_time < (int) time(0)))
  {
    tell_player(p, " You don't have anyone to reply to!\n");
    return;
  }
  sprintf(stack, "%s ", p->reply);
  stack = strchr(stack, 0);
  strcpy(stack, str);
  stack = end_string(stack);
  sys_flags |= REPLY_TAG;
  premote(p, oldstack);
  stack = oldstack;
  sys_flags &= ~REPLY_TAG;
}


/* And the rsing reply command itself */

void sreply(player * p, char *str)
{
  char *oldstack;

  oldstack = stack;

  if (!*str)
  {
    tell_player(p, " Format: sreply <msg>\n");
    return;
  }
  if (!*(p->reply) || (p->reply_time < (int) time(0)))
  {
    tell_player(p, " You don't have anyone to reply to!\n");
    return;
  }
  sprintf(stack, "%s ", p->reply);
  stack = strchr(stack, 0);
  strcpy(stack, str);
  stack = end_string(stack);
  sys_flags |= REPLY_TAG;
  rsing(p, oldstack);
  stack = oldstack;
  sys_flags &= ~REPLY_TAG;
}



/* And the beep_tell reply command itself */

void yreply(player * p, char *str)
{
  char *oldstack;

  oldstack = stack;

  if (!*str)
  {
    tell_player(p, " Format: yreply <msg>\n");
    return;
  }
  if (!*(p->reply) || (p->reply_time < (int) time(0)))
  {
    tell_player(p, " You don't have anyone to reply to!\n");
    return;
  }
  sprintf(stack, "%s ", p->reply);
  stack = strchr(stack, 0);
  strcpy(stack, str);
  stack = end_string(stack);
  sys_flags |= REPLY_TAG;
  beep_tell(p, oldstack);
  stack = oldstack;
  sys_flags &= ~REPLY_TAG;
}


/* And the echo reply command itself */

void echoreply(player * p, char *str)
{
  char *oldstack;

  oldstack = stack;

  if (!*str)
  {
    tell_player(p, " Format: oreply <msg>\n");
    return;
  }
  if (!*(p->reply) || (p->reply_time < (int) time(0)))
  {
    tell_player(p, " You don't have anyone to reply to!\n");
    return;
  }
  sprintf(stack, "%s ", p->reply);
  stack = strchr(stack, 0);
  strcpy(stack, str);
  stack = end_string(stack);
  sys_flags |= REPLY_TAG;
  recho(p, oldstack);
  stack = oldstack;
  sys_flags &= ~REPLY_TAG;
}


void view_people_and_flags(player * p, char *str)
{
  char *oldstack, middle[80];
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

  strcpy(middle, "Listing all people and flags");

  pstack_mid(middle);

  count = page * (TERM_LINES - 2);
  for (scan = flatlist_start; count; scan = scan->flat_next)
    if (!scan)
    {
      tell_player(p, " Bad who listing, abort.\n");
      log("error", "Bad who list (session.c)");
      stack = oldstack;
      return;
    }
    else if (scan->name[0])
      count--;
  for (count = 0; (count < (TERM_LINES - 1) && scan); scan = scan->flat_next)
    if (scan->name[0] && scan->location)
    {
/*
   if (scan == p_sess)
 */
      sprintf(stack, "%-20s=", scan->name);

      stack = strchr(stack, 0);

      if (scan->misc_flags & SYSTEM_COLOR)
	strcpy(stack, "SysC ");
      else
	strcpy(stack, "     ");
      stack = strchr(stack, 0);

      if (scan->misc_flags & NOCOLOR)
	strcpy(stack, "    ");
      else
	strcpy(stack, "Col ");
      stack = strchr(stack, 0);

      if (scan->tag_flags & BLOCK_TELLS)
	strcpy(stack, "BkTl ");
      else
	strcpy(stack, "     ");
      stack = strchr(stack, 0);

      if (scan->tag_flags & BLOCK_SHOUT)
	strcpy(stack, "BkSht ");
      else
	strcpy(stack, "      ");
      stack = strchr(stack, 0);

      if (scan->tag_flags & BLOCK_FRIENDS)
	strcpy(stack, "BkFT ");
      else
	strcpy(stack, "     ");
      stack = strchr(stack, 0);

      if (scan->tag_flags & SINGBLOCK)
	strcpy(stack, "BkSg ");
      else
	strcpy(stack, "     ");
      stack = strchr(stack, 0);

      if (scan->tag_flags & NOBEEPS)
	strcpy(stack, "BkBp ");
      else
	strcpy(stack, "     ");
      stack = strchr(stack, 0);
      if (scan->tag_flags & BLOCKCHANS)
	strcpy(stack, "BkChAn ");
      else
	strcpy(stack, "       ");
      stack = strchr(stack, 0);
      if (scan->tag_flags & NO_FACCESS)
	strcpy(stack, "NoF ");
      else
	strcpy(stack, "    ");
      stack = strchr(stack, 0);
      if (scan->tag_flags & BLOCK_ROOM_DESC)
	strcpy(stack, "NoRDesc ");
      else
	strcpy(stack, "        ");
      stack = strchr(stack, 0);
      *stack++ = '\n';
      count++;
    }
  sprintf(middle, "Page %d of %d", page + 1, pages + 1);
  pstack_mid(middle);
  *stack++ = 0;
  tell_player(p, oldstack);
  stack = oldstack;
}




void view_player_url(player * p, char *str)
{

  player *scan, *start;
  int pages = 1, page, line;
  char *oldstack, middle[80];

  oldstack = stack;

  if (strlen(str) < 1)
    page = 1;
  else
    page = atoi(str);

  if (page < 1)
  {
    tell_player(p, " Usage : list_url [<pagenumber>]\n");
    return;
  }
  scan = flatlist_start;
  start = NULL;

  line = 0;

  for (scan = flatlist_start; scan; scan = scan->flat_next)
  {
    if (pages <= page && line == 0)
      start = scan;

    if (scan->alt_email[0] != 0)
      line++;

    if (line > TERM_LINES - 2)
    {
      line = 0;
      pages++;
    }
  }

  if (page > pages)
    page = pages;

  pstack_mid("Listing of all active player's URLs");

  line = 0;
  for (; start; start = start->flat_next)
  {
    if (line > TERM_LINES)
      break;

    if (start->alt_email[0] != 0)
    {
      sprintf(stack, "%-19s *", start->name);
      stack = strchr(stack, 0);

      strcpy(stack, start->alt_email);

      stack = strchr(stack, 0);
      *stack++ = '\n';

      line++;
    }
  }

  sprintf(middle, "Page %d of %d", page, pages);
  pstack_mid(middle);
  *stack++ = 0;
  tell_player(p, oldstack);
  stack = oldstack;
  return;
}

/* like comments bit shows everyone even if they haven't set a comment */
/* Added here because I removed it first time around - doh! */

void view_session(player * p, char *str)
{
  char *oldstack, middle[80];
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

  if (strlen(session) == 0)
  {
    strncpy(session, "Set Me Please", MAX_SESSION - 3);
    strcpy(sess_name, "No-One");
  }
  strcpy(middle, session);
  pstack_mid(middle);

  count = page * (TERM_LINES - 2);
  for (scan = flatlist_start; count; scan = scan->flat_next)
    if (!scan)
    {
      tell_player(p, " Bad who listing, abort.\n");
      log("error", "Bad who list (session.c)");
      stack = oldstack;
      return;
    }
    else if (scan->name[0])
      count--;
  for (count = 0; (count < (TERM_LINES - 1) && scan); scan = scan->flat_next)
    if (scan->name[0] && scan->location)
    {
      if (!strcasecmp(sess_name, scan->lower_name))
	sprintf(stack, "%-19s*- ", scan->name);
      else
	sprintf(stack, "%-19s - ", scan->name);

      stack = strchr(stack, 0);

      strcpy(stack, scan->comment);
      stack = strchr(stack, 0);
      *stack++ = '\n';
      count++;
    }
  sprintf(middle, "Page %d of %d", page + 1, pages + 1);
  pstack_mid(middle);
  *stack++ = 0;
  tell_player(p, oldstack);
  stack = oldstack;
}
