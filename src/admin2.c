/*
 * Playground+ - admin2.c
 * All non-su / admin routines and some simple staff routines
 * ---------------------------------------------------------------------------
 */

#include <string.h>
#include <memory.h>
#include <time.h>
#ifndef BSDISH
#include <malloc.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "include/config.h"
#include "include/player.h"
#include "include/proto.h"

#ifdef AUTOSHUTDOWN
extern int auto_sd;
#endif

extern char shutdown_reason[];

/* New vlog lib by phypor ... not only is it dynamic with scandir,
   but it allows for diffrent logs to be accessible by diffrent privs
 */

char snl[160];

int priv_for_log_str(char *str)
{
  if (!strncasecmp(str, "ad", 2))
    return S_IRUSR;
  if (!strcasecmp(str, "la") || !strcasecmp(str, "lower_admin"))
    return S_IRGRP | S_IRUSR;
  if (!strncasecmp(str, "su", 2))
    return S_IROTH | S_IRGRP | S_IRUSR;

  return -1;
}


int priv_for_log(player * p)
{
  if (p->residency & (ADMIN | HCADMIN))
    return S_IRUSR;
  if (p->residency & (LOWER_ADMIN))
    return S_IRGRP;
  return S_IROTH;
}

char *str_priv_log(int mode)
{
  if (mode & S_IROTH)
    return "su";
  if (mode & S_IRGRP)
    return "la";
  return "ad";
}

#ifdef REDHAT5
int valid_log(struct dirent *d)
#else /* REDHAT5 */
int valid_log(const struct dirent *d)
#endif				/* !REDHAT5 */
{
  char *dotter;

  if (!(dotter = strstr(d->d_name, ".log")) || strlen(dotter) != 4)
    return 0;
  return 1;
}


void vlog(player * p, char *str)
{
  struct dirent **de;
  struct stat sbuf;
  char *oldstack = stack, path[160];
  int dc = 0, hits = 0, i;
  file lf;

  if (!*str || *str == '?')
  {
    dc = scandir("logs", &de, valid_log, alphasort);
    if (!dc)
    {
      tell_player(p, " There are currently no log files ...\n");
      if (de)
	free(de);
      return;
    }
    pstack_mid("Accessible Logs");	/* might as well make it pretty */
    stack += sprintf(stack, "\n");
    for (i = 0; i < dc; i++)
    {
      sprintf(path, "logs/%s", de[i]->d_name);
      if (stat(path, &sbuf) < 0)	/* dunno how it could be tho */
	continue;

      if (!(sbuf.st_mode & priv_for_log(p)))
	continue;

      hits++;
      strcpy(stack, de[i]->d_name);
      stack = strchr(stack, 0);
      while (*stack != '.')
	*stack-- = 0;
      *stack++ = ',';
      *stack++ = ' ';

    }
    for (i = 0; i < dc; i++)
      FREE(de[i]);
    if (de)
      FREE(de);

    if (!hits)
    {
      stack = oldstack;
      tell_player(p, " There are currently no log files available to you.\n");
      return;
    }
    while (*stack != ',')
      *stack-- = '\0';
    stack += sprintf(stack, ".\n\n");
    if (hits == 1)
      sprintf(path, "There is one log available to you");
    else
      sprintf(path, "There are %d logs available to you", hits);
    pstack_mid(path);
    *stack++ = '\0';
    tell_player(p, oldstack);
    stack = oldstack;
    return;
  }
  if (*str == '.' && !(p->residency & ADMIN))
  {
    tell_player(p, " No such log ... bung hole\n");
    return;
  }

  sprintf(path, "logs/%s.log", str);
  if (stat(path, &sbuf) < 0)
  {
    TELLPLAYER(p, " No log '%s' found ...\n", str);
    return;
  }
  if (!(sbuf.st_mode & priv_for_log(p)))
  {
    TELLPLAYER(p, " No log '%s' available ...\n", str);		/* notice wording */
    return;
  }
  lf = load_file(path);
  if (!lf.where)
  {
    tell_player(p, " Ouch, dunno whut happened ...\n");
    return;
  }
  if (!*(lf.where))
  {
    tell_player(p, " The file is there, but its empty as a blondes skull.\n");
    FREE(lf.where);
    return;
  }
  pager(p, lf.where);
  FREE(lf.where);
}



void set_log_priv(player * p, char *str)
{
  int npriv, fd;
  char *nstr, path[160];
  struct stat sbuf;

  nstr = next_space(str);
  if (!*str || !*nstr)
  {
    tell_player(p, " Format : chaccess <log> <level>\n"
		"    <level> is either ad, la, or su\n");
    return;
  }
  *nstr++ = 0;

  if (*str == '.')
  {
    tell_player(p, " Now, now... go onto the shell to do that...\n");
    return;
  }

  npriv = priv_for_log_str(nstr);
  if (npriv < 0)
  {
    tell_player(p, " Not a valid priv level... try ad, la, or su\n");
    return;
  }
  sprintf(path, "logs/%s.log", str);
  if (stat(path, &sbuf) < 0)
  {
    if (strcmp(snl, path))
    {
      strncpy(snl, path, 159);
      tell_player(p, " No such log file, attempt this command again if"
		  " you wish to have the log created (and be blank)\n");
      return;
    }
    memset(snl, 0, 160);
    tell_player(p, " Ok, creating new, empty log.\n");
    fd = creat(path, S_IRUSR);	/* doesnt matter on mode */
    close(fd);
  }
  chmod(path, S_IWUSR | npriv);

  TELLPLAYER(p, " Priv access changed for log file %s to '%s'\n",
	     path, nstr);
  SW_BUT(p, " -=*> %s changes the access priv of logfile %s to '%s'\n",
	 p->name, str, nstr);
  LOGF("logs", "%s changes %s to %s", p->name, path, nstr);
}



void show_logs(player * p, char *str)
{
  struct dirent **de;
  struct stat sbuf;
  char *oldsmack = stack, path[160], tl[160], *pt;
  int dc = 0, hits = 0, i, compact = 0, t;

  t = time(0);

  if (*str && !strcasecmp(str, "new"))
    compact++;

  dc = scandir("logs", &de, valid_log, alphasort);
  if (!dc)
  {
    if (!compact)
      tell_player(p, " Log directory is empty ...\n");
    if (de)
      free(de);
    return;
  }
  if (compact)
    stack += sprintf(stack, " Log touches: ");
  else
    pstack_mid("Logs Listing");

  for (i = 0; i < dc; i++)
  {
    sprintf(path, "logs/%s", de[i]->d_name);
    if (stat(path, &sbuf) < 0)	/* dunno how it could be tho */
      continue;

    if (!(sbuf.st_mode & priv_for_log(p)))
      continue;

    if (compact && sbuf.st_mtime < p->saved->last_on)
      continue;

    hits++;
    if (compact)
    {
      strcpy(stack, de[i]->d_name);
      stack = strchr(stack, 0);
      while (*stack != '.')
	*stack-- = 0;
      *stack++ = ',';
      *stack++ = ' ';
    }
    else
    {
      memset(tl, 0, 160);
      strncpy(tl, de[i]->d_name, 159);
      pt = strchr(tl, 0);
      while (*pt-- != '.');
      *++pt = 0;
      stack += sprintf(stack, "  %-30s %-5s %s\n", tl,
		  str_priv_log(sbuf.st_mode), word_time(t - sbuf.st_mtime));
    }
  }
  for (i = 0; i < dc; i++)
    FREE(de[i]);
  if (de)
    FREE(de);

  if (!hits)
  {
    stack = oldsmack;
    if (!compact)
      tell_player(p, " No logs accessible ...\n");
    return;
  }
  if (compact)
  {
    while (*stack != ',')
      *stack-- = '\0';
    stack += sprintf(stack, ".\n");
    stack = end_string(stack);
    tell_player(p, oldsmack);
    stack = oldsmack;
    return;
  }
  if (hits == 1)
    sprintf(path, "There is one log available to you");
  else
    sprintf(path, "There are %d logs available to you", hits);
  pstack_mid(path);
  *stack++ = '\0';
  pager(p, oldsmack);
  stack = oldsmack;
}

/* warn someone */

void warn(player * p, char *str)
{
  char *oldstack, *msg, *pstring, *final;
  player **list, **step;
  int i, n, r = 0;


  oldstack = stack;
  align(stack);
  command_type = PERSONAL | SEE_ERROR | WARNING;

  if (p->tag_flags & BLOCK_TELLS)
  {
    tell_player(p, " You are currently BLOCKING TELLS. It might be an idea to"
		" unblock so they can reply, eh?\n");
  }
  msg = next_space(str);
  if (*msg)
    *msg++ = 0;
  if (!*msg)
  {
    tell_player(p, " Format: warn <player(s)> <message>\n");
    stack = oldstack;
    return;
  }
  CHECK_DUTY(p);

  /* no warns to groups */
  if (!strcasecmp(str, "everyone") || !strcasecmp(str, "friends")
      || !strcasecmp(str, "supers") || !strcasecmp(str, "sus")
      || strstr(str, "everyone"))
  {
    tell_player(p, " Now that would be a bit silly wouldn't it?\n");
    stack = oldstack;
    return;
  }
  if (!strcasecmp(str, "room"))
    r = 1;
  /* should you require warning, the consequences are somewhat severe */
  if (!strcasecmp(str, "me"))
  {
    tell_player(p, " Ummmmmmmmmmmmmmmmmmmmmm no. \n");
    stack = oldstack;
    return;
  }
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
  final = stack;
  if (r)
    sprintf(stack, " -=*> %s warns everyone in this room: %s\n\n", p->name, msg);
  else
    sprintf(stack, " -=*> %s warns you: %s\n\n", p->name, msg);
  stack = end_string(stack);
  for (step = list, i = 0; i < n; i++, step++)
  {
    if (*step != p)
    {
      command_type |= HIGHLIGHT;
      if ((*step)->residency & SU && (*step)->tag_flags & NOBEEPS)
	tell_player(*step, "\n");
      else
	tell_player(*step, "\007\n");
      tell_player(*step, final);
      (*step)->warn_count++;
      p->num_warned++;
      command_type &= ~HIGHLIGHT;
    }
  }
  stack = final;

  pstring = tag_string(p, list, n);
  final = stack;
  sprintf(stack, " -=*> %s warns %s: %s\n", p->name, pstring, msg);
  stack = end_string(stack);
  LOGF("warn", "%s warns %s: %s", p->name, pstring, msg);
  command_type = 0;
  su_wall(final);

  cleanup_tag(list, n);
  stack = oldstack;
}


/* trace someone and check against email */

void trace(player * p, char *str)
{
  char *oldstack;
  player *p2, dummy;


  oldstack = stack;
  if (!*str)
  {
    tell_player(p, " Format: trace <person>\n");
    return;
  }
  p2 = find_player_absolute_quiet(str);
  if (!p2)
  {
    sprintf(stack, " \'%s\' not logged on, checking saved files...\n",
	    str);
    stack = end_string(stack);
    tell_player(p, oldstack);
    stack = oldstack;
    strcpy(dummy.lower_name, str);
    lower_case(dummy.lower_name);
    dummy.fd = p->fd;
    if (!load_player(&dummy))
    {
      tell_player(p, " Not found.\n");
      return;
    }
    if (dummy.residency == BANISHD)
    {
      tell_player(p, " That is a banished name.\n");
      return;
    }
    if (dummy.email[0])
    {
      if (dummy.email[0] == -1)
      {
	sprintf(stack, " %s has declared no email address.\n", dummy.name);
	stack = strchr(stack, 0);
      }
      else if (p->residency & ADMIN)
      {
	sprintf(stack, " %s [%s]\n", dummy.name, dummy.email);
	if (dummy.custom_flags & PRIVATE_EMAIL)
	{
	  while (*stack != '\n')
	    stack++;
	  strcpy(stack, " (private)\n");
	}
	stack = strchr(stack, 0);
      }
    }
    sprintf(stack, " %s last connected from %s\n   and disconnected at ",
	    dummy.name, dummy.saved->last_host);
    stack = strchr(stack, 0);
    if (p->jetlag)
      sprintf(stack, "%s\n", convert_time(dummy.saved->last_on
					  + (p->jetlag * 3600)));
    else
      sprintf(stack, "%s\n", convert_time(dummy.saved->last_on));
    stack = end_string(stack);
    tell_player(p, oldstack);
    stack = oldstack;
    return;
  }
  if (p2->residency == NON_RESIDENT)
  {
    sprintf(stack, " %s is non resident.\n", p2->name);
    stack = strchr(stack, 0);
  }
  else if (p2->email[0])
  {
    if (p2->email[0] == -1)
    {
      sprintf(stack, " %s has declared no email address.\n", p2->name);
      stack = strchr(stack, 0);
    }
    else if (p->residency & ADMIN)
    {
      sprintf(stack, " %s [%s]\n", p2->name, p2->email);
      if (p2->custom_flags & PRIVATE_EMAIL)
      {
	while (*stack != '\n')
	  stack++;
	strcpy(stack, " (private)\n");
      }
      stack = strchr(stack, 0);
    }
  }
  else
  {
    sprintf(stack, " %s has not set an email address.\n", p2->name);
    stack = strchr(stack, 0);
  }
  sprintf(stack, " %s is connected from %s.\n", p2->name, get_address(p2, p));
  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
}

/* A grep command (Code provided by Retro - which was nice because I
   didn't really want to write this) */

void grep(player * p, char *str)
{
  char *oldstack, *scan, *scan2, *search;
  file logb;
  int count = 0;

  oldstack = stack;

  search = strchr(str, 0);
  search--;
  while (*search && *search != ' ')
    search--;
  if (*search)
    *search++ = 0;
  if (!*search || !*str)
  {
    tell_player(p, " Format: grep <string> <log file>\n");
    return;
  }
  sprintf(stack, "logs/%s.log", search);
  stack = end_string(stack);
  logb = load_file_verbose(oldstack, 0);
  stack = oldstack;

  if (logb.where)
  {
    if (*(logb.where))
    {
      scan = logb.where;
      while (*scan)
      {
	scan2 = scan;
	while (*scan && *scan != '\n')
	{
	  if (!strncasecmp(scan, str, strlen(str)))
	  {
	    while (*scan2 && *scan2 != '\n')
	      *stack++ = *scan2++;
	    *stack++ = '\n';

	    while (*scan && *scan != '\n')
	      scan++;
	    count++;
	    break;
	  }
	  scan++;
	}
	scan++;
      }
      if (count == 0)
      {
	tell_player(p, " Sorry, no matches found.\n");
	FREE(logb.where);
	stack = oldstack;
	return;
      }
      sprintf(stack, " %d match(es) found.\n", count);
      stack = strchr(stack, 0);
      *stack++ = 0;
      pager(p, oldstack);
    }
    else
      TELLPLAYER(p, " Couldn't find logfile 'logs/%s.log'\n", search);
    FREE(logb.where);
  }
  stack = oldstack;
}


/* list people who are from the same site */

void same_site(player * p, char *str)
{
  char *oldstack, *text;
  player *p2;

  oldstack = stack;
  if (isalpha(*str))
  {
    if (!strcasecmp(str, "me"))
    {
      p2 = p;
    }
    else
    {
      p2 = find_player_global(str);
    }
    if (!p2)
    {
      stack = oldstack;
      return;
    }
    str = stack;
    text = p2->num_addr;
    while (isdigit(*text))
      *stack++ = *text++;
    *stack++ = '.';
    text++;
    while (isdigit(*text))
      *stack++ = *text++;
    *stack++ = '.';
    *stack++ = '*';
    *stack++ = '.';
    *stack++ = '*';
    *stack++ = 0;
  }
  if (!isdigit(*str))
  {
    tell_player(p, " Format: site <inet_number> or site <person>\n");
    stack = oldstack;
    return;
  }
  text = stack;
  sprintf(stack, "People from %s:\n", str);
  stack = strchr(stack, 0);

  for (p2 = flatlist_start; p2; p2 = p2->flat_next)
  {
    if (match_banish(p2, str))
    {
      sprintf(stack, "(%s) %s : %s ", p2->num_addr, get_address(p2, p), p2->name);
      stack = strchr(stack, 0);
      if (p2->residency == NON_RESIDENT)
      {
	strcpy(stack, "non resident.\n");
	stack = strchr(stack, 0);
      }
      else if (p2->email[0])
      {
	if (p2->email[0] == -1)
	  strcpy(stack, "No email address.");
	else
	{
	  if (((p2->custom_flags & PRIVATE_EMAIL) &&
	       (p->residency & ADMIN))
	      || !(p2->custom_flags & PRIVATE_EMAIL))
	  {

	    sprintf(stack, "[%s]", p2->email);
	    stack = strchr(stack, 0);
	  }
	  if (p2->custom_flags & PRIVATE_EMAIL)
	  {
	    strcpy(stack, " (private)");
	    stack = strchr(stack, 0);
	  }
	}
	*stack++ = '\n';
      }
      else
      {
	strcpy(stack, "Email not set\n");
	stack = strchr(stack, 0);
      }
    }
  }
  *stack++ = 0;
  pager(p, text);
  stack = oldstack;
}

/* toggle whether the program is globally closed to newbies */

void close_to_newbies(player * p, char *str)
{
  CHECK_DUTY(p);
  if (*str)
  {
    if ((!strcasecmp("off", str) || !strcasecmp("close", str)) &&
	!(sys_flags & CLOSED_TO_NEWBIES))
    {
      sys_flags |= CLOSED_TO_NEWBIES;
      LOGF("newbies", "Newbies turned off by %s", p->name);
      SUWALL("\n -=*> %s closes the program to newbies.\n\n", p->name);
      return;
    }
    else if ((!strcasecmp("on", str) || !strcasecmp("open", str)) &&
	     sys_flags & CLOSED_TO_NEWBIES)
    {
      sys_flags &= ~CLOSED_TO_NEWBIES;
      LOGF("newbies", "Newbies allowed by %s", p->name);
      SUWALL("\n -=*> %s opens the program to newbies.\n\n", p->name);
      return;
    }
  }
  if (sys_flags & CLOSED_TO_NEWBIES)
    TELLPLAYER(p, " %s currently closed to newbies.\n", talker_name);
  else
    TELLPLAYER(p, " %s currently open to newbies.\n", talker_name);
}

#ifdef NULLcode
void close_to_newbies(player * p, char *str)
{
  int wall = 0;

  if (p->flags & BLOCK_SU)
  {
    tell_player(p, " You cant do THAT when off_duty.\n");
    return;
  }

  if (!*str)
    tell_player(p, " Format: newbies [on|off]\n");

  if ((!strcasecmp("on", str) || !strcasecmp("open", str))
      && sys_flags & CLOSED_TO_NEWBIES)
  {
    sys_flags &= ~CLOSED_TO_NEWBIES;

    /*log the open */
    LOGF("newbies", "Program opened to newbies by %s", p->name);
    wall = 1;
  }
  else if ((!strcasecmp("off", str) || !strcasecmp("close", str))
	   && !(sys_flags & CLOSED_TO_NEWBIES))
  {
    sys_flags |= CLOSED_TO_NEWBIES;
    sys_flags &= SCREENING_NEWBIES;

    /*log the close */
    LOGF("newbies", "Program closed to newbies by %s", p->name);
    wall = 1;
  }
  else
    wall = 0;

  if (sys_flags & CLOSED_TO_NEWBIES)
  {
    if (!wall)
      TELLPLAYER(p, " %s is closed to all newbies.\n",
		 get_config_msg("talker_name"));
    else
      SUWALL("\n -=*> %s closes %s to newbies.\n\n", p->name,
	     get_config_msg("talker_name"));
  }
  else
  {
    if (!wall)
      TELLPLAYER(p, " %s is open to all newbies.\n",
		 get_config_msg("talker_name"));
    else
      SUWALL("\n -=*> %s opens %s to newbies.\n\n", p->name,
	     get_config_msg("talker_name"));
  }
}
#endif /* NULLcode */

/* toggle whether the program is globally closed to ressies */

void close_to_ressies(player * p, char *str)
{
  int wall = 0;

  if (p->flags & BLOCK_SU)
  {
    tell_player(p, " You cant do THAT when off_duty.\n");
    return;
  }

  if (!*str)
    tell_player(p, " Format: ressies [on|off]\n");


  if ((!strcasecmp("on", str) || !strcasecmp("open", str))
      && sys_flags & CLOSED_TO_RESSIES)
  {
    sys_flags &= ~CLOSED_TO_RESSIES;

    /*log the open */
    LOGF("ressies", "Program opened to ressies by %s", p->name);
    wall = 1;
  }
  else if ((!strcasecmp("off", str) || !strcasecmp("close", str))
	   && !(sys_flags & CLOSED_TO_RESSIES))
  {
    sys_flags |= CLOSED_TO_RESSIES;

    /*log the close */
    LOGF("ressies", "Program closed to ressies by %s", p->name);
    wall = 1;
  }
  else
    wall = 0;

  if (sys_flags & CLOSED_TO_RESSIES)
  {
    if (!wall)
      TELLPLAYER(p, " %s is closed to all ressies.\n",
		 get_config_msg("talker_name"));
    else
      SUWALL("\n -=*> %s closes %s to ressies.\n\n", p->name,
	     get_config_msg("talker_name"));
  }
  else
  {
    if (!wall)
      TELLPLAYER(p, " %s is open to all ressies.\n",
		 get_config_msg("talker_name"));
    else
      SUWALL("\n -=*> %s opens %s to ressies.\n\n", p->name,
	     get_config_msg("talker_name"));
  }
}

/* Sync all player files */

void sync_all_by_user(player * p, char *str)
{
  tell_player(p, " Starting to sync ALL players...");
  sync_all();
  tell_player(p, " Completed\n\r");
}

/* command to list lots of info about a person */

void check_info(player * p, char *str)
{
  player dummy, *p2;
  char *oldstack;

  oldstack = stack;
  if (!*str)
  {
    tell_player(p, " Format: check info <player>\n");
    return;
  }
  memset(&dummy, 0, sizeof(player));

  p2 = find_player_absolute_quiet(str);
  if (p2)
    memcpy(&dummy, p2, sizeof(player));
  else
  {
    strcpy(dummy.lower_name, str);
    lower_case(dummy.lower_name);
    dummy.fd = p->fd;
    if (!load_player(&dummy))
    {
      tell_player(p, " No such person in saved files.\n");
      return;
    }
  }

  switch (dummy.residency)
  {
    case SYSTEM_ROOM:
      tell_player(p, " Standard rooms file\n");
      return;
    default:
      if (dummy.residency & BANISHD)
      {
	if (dummy.residency == BANISHD)
	  tell_player(p, "BANISHED (Name only).\n");
	else
	  tell_player(p, "BANISHED.\n");
      }

      sprintf(stack, "            " RES_BIT_HEAD "\n"
	      "Residency   %s\n", privs_bit_string(dummy.residency));
      break;
  }
  stack = strchr(stack, 0);

  sprintf(stack, "%s %s %s^N\n", dummy.pretitle, dummy.name, dummy.title);
  stack = strchr(stack, 0);
  sprintf(stack, "EMAIL: %s\n", dummy.email);
  stack = strchr(stack, 0);
  sprintf(stack, "SPOD_CLASS: %s^N\n", dummy.spod_class);
  stack = strchr(stack, 0);
  if (dummy.term)
  {
    stack += sprintf(stack, "Hitells turned set to %s",
		     terms[(p->term) - 1].name);
    if (dummy.misc_flags & NOCOLOR)
      stack += sprintf(stack, "  (Inline ON)");
    if (dummy.misc_flags & SYSTEM_COLOR)
      stack += sprintf(stack, "  (Syscolor ON)");
    strcpy(stack, "\n");
    stack = strchr(stack, 0);
  }
  if ((dummy.password[0]) <= 0)
  {
    strcpy(stack, "NO PASSWORD SET\n");
    stack = strchr(stack, 0);
  }
  sprintf(stack, "            !D$LdJmM--fnemMmLAIiDsb---------\n"
	  "SystemFlags %s\n", bit_string(dummy.system_flags));
  stack = strchr(stack, 0);
  sprintf(stack, "            >-!]+e#^^--!>*+dm$c=B#?pILB------\n"
	  "TagFlags    %s\n", bit_string(dummy.tag_flags));
  stack = strchr(stack, 0);
  sprintf(stack, "            hESes-----gmnpeSPrfxCq----------\n"
	  "CustomFlags %s\n", bit_string(dummy.custom_flags));
  stack = strchr(stack, 0);
  sprintf(stack, "            PG--------csCSg-----------------\n"
	  "MiscFlags   %s\n", bit_string(dummy.misc_flags));
  stack = strchr(stack, 0);
  sprintf(stack, "            PRNREPCPTLCEISDRUbSWAFSlC-------\n"
	  "flags       %s\n", bit_string(dummy.flags));
  stack = strchr(stack, 0);
  sprintf(stack, "Max: rooms %d, exits %d, autos %d, list %d, mails %d, "
	  "alias %d, items %d\n",
	  dummy.max_rooms, dummy.max_exits, dummy.max_autos, dummy.max_list,
	  dummy.max_mail, dummy.max_alias, dummy.max_items);
  stack = strchr(stack, 0);
  sprintf(stack, "Term: width %d, wrap %d\n",
	  dummy.term_width, dummy.word_wrap);
  stack = strchr(stack, 0);
  if (dummy.script)
  {
    sprintf(stack, "Scripting on for another %s.\n",
	    word_time(dummy.script));
    stack = strchr(stack, 0);
  }
  *stack++ = 0;
  tell_player(p, oldstack);
  stack = oldstack;
}


/* command to check IP addresses */

void view_ip(player * p, char *str)
{
  player *scan;
  char *oldstack, middle[80];
  int page, pages, count;

  oldstack = stack;
  if (isalpha(*str))
  {
    scan = find_player_global(str);
    stack = oldstack;
    if (!scan)
      return;
    sprintf(stack, "%s is logged in from %s.\n", scan->name,
	    get_address(scan, p));
    stack = end_string(stack);
    tell_player(p, oldstack);
    stack = oldstack;
    return;
  }
  page = atoi(str);
  if (page <= 0)
    page = 1;
  page--;

  pages = (current_players - 1) / (TERM_LINES - 2);
  if (page > pages)
    page = pages;

  if (current_players == 1)
    sprintf(middle, "There is only you on %s at the moment",
	    get_config_msg("talker_name"));
  else
    sprintf(middle, "There are %s people on %s",
	    number2string(current_players), get_config_msg("talker_name"));
  pstack_mid(middle);
  count = page * (TERM_LINES - 2);
  for (scan = flatlist_start; count; scan = scan->flat_next)
  {
    if (!scan)
    {
      tell_player(p, " Bad where listing, abort.\n");
      log("error", "Bad where list");
      stack = oldstack;
      return;
    }
    else if (scan->name[0])
      count--;
  }

  for (count = 0; (count < (TERM_LINES - 1) && scan); scan = scan->flat_next)
  {
#ifdef ROBOTS
    if (scan->name[0] && scan->location && !(scan->residency & ROBOT_PRIV))
#else
    if (scan->name[0] && scan->location)
#endif
    {
      if (scan->flags & SITE_LOG)
	*stack++ = '*';
      else
	*stack++ = ' ';
      sprintf(stack, "%s is logged in from %s.\n", scan->name,
	      get_address(scan, p));
      stack = strchr(stack, 0);
      count++;
    }
  }
  sprintf(middle, "Page %d of %d", page + 1, pages + 1);
  pstack_mid(middle);

  *stack++ = 0;
  tell_player(p, oldstack);
  stack = oldstack;
}


/* command to view email status about people on the prog */

void view_player_email(player * p, char *str)
{
  player *scan;
  char *oldstack, middle[80];
  int page, pages, count;

  oldstack = stack;
  page = atoi(str);
  if (page <= 0)
    page = 1;
  page--;

  pages = (current_players - 1) / (TERM_LINES - 2);
  if (page > pages)
    page = pages;

  if (current_players == 1)
    sprintf(middle, "There is only you on %s at the moment.",
	    get_config_msg("talker_name"));
  else
    sprintf(middle, "There are %s people on %s",
	    number2string(current_players), get_config_msg("talker_name"));
  pstack_mid(middle);

  count = page * (TERM_LINES - 2);
  for (scan = flatlist_start; count; scan = scan->flat_next)
  {
    if (!scan)
    {
      tell_player(p, " Bad where listing, abort.\n");
      log("error", "Bad where list");
      stack = oldstack;
      return;
    }
    else if (scan->name[0])
      count--;
  }

  for (count = 0; (count < (TERM_LINES - 1) && scan); scan = scan->flat_next)
  {
    if (scan->name[0] && scan->location)
    {
      if (scan->residency == NON_RESIDENT)
	sprintf(stack, "%s is non resident.\n", scan->name);
      else if (scan->email[0])
      {
	if (scan->email[0] == -1)
	  sprintf(stack, "%s has declared no email address.\n",
		  scan->name);
	else if (scan->email[0] == -2)
	{
	  sprintf(stack, "%s has not yet set an email address.\n",
		  scan->name);
	}
	else
	{
	  sprintf(stack, "%s [%s]\n", scan->name, scan->email);
	  if (scan->custom_flags & PRIVATE_EMAIL)
	  {
	    while (*stack != '\n')
	      stack++;
	    strcpy(stack, " (private)\n");
	  }
	}
      }
      else
	sprintf(stack, "%s has not set an email address.\n", scan->name);
      stack = strchr(stack, 0);
      count++;
    }
  }
  sprintf(middle, "Page %d of %d", page + 1, pages + 1);
  pstack_mid(middle);

  *stack++ = 0;
  tell_player(p, oldstack);
  stack = oldstack;
}

/* command to validate lack of email */

void validate_email(player * p, char *str)
{
  player *p2;
  char *oldstack;

  oldstack = stack;

  if (p->flags & BLOCK_SU)
  {
    tell_player(p, " You cant validate emails when off_duty.\n");
    return;
  }

  if (!*str)
  {
    tell_player(p, " Format: validate_email <player>\n");
    return;
  }

  p2 = find_player_global(str);
  if (!p2)
    return;
  p2->email[0] = ' ';
  p2->email[1] = 0;
  p2->saved->email[0] = ' ';
  p2->saved->email[1] = 0;
  tell_player(p, " Set player as having no email address.\n");

  sprintf(stack, "%s validated email for %s", p->name, p2->name);
  stack = end_string(stack);
  log("validate_email", oldstack);
  stack = oldstack;

}

/* a test fn to test things */

void test_fn(player * p, char *str)
{
  do_birthdays();
}

void remove_saved_lag(player * p, char *str)
{
  char *oldstack;
  player *p2, dummy;

  oldstack = stack;

  tell_player(p, " Checking saved files... ");
  strcpy(dummy.lower_name, str);
  lower_case(dummy.lower_name);
  dummy.fd = p->fd;
  if (!load_player(&dummy))
  {
    tell_player(p, " Not found.\n");
    return;
  }
  else
  {
    tell_player(p, "\n");
    p2 = &dummy;
    p2->location = (room *) - 1;
  }


  if (!(p2->system_flags & SAVE_LAGGED))
  {
    tell_player(p, " That player is not lagged.\n");
    return;
  }
  p2->system_flags ^= SAVE_LAGGED;

  sprintf(stack, " -=*> %s unlags %s.\n", p->name, p2->name);
  stack = end_string(stack);
  au_wall_but(p, oldstack);
  stack = oldstack;
  sprintf(stack, "%s unlags %s.", p->name, p2->name);
  stack = end_string(stack);
  log("lag", oldstack);
  stack = oldstack;
  save_player(&dummy);
}


/* give someone lag ... B-) */

void add_lag(player * p, char *str)
{
  char *size;
  int new_size, plag = 0;
  char *oldstack;
  player *p2;

  if (p->flags & BLOCK_SU)
  {
    tell_player(p, " Lagging isnt nice at the best of times, the least "
		"you can do is go on_duty before you torture the poor "
		"victim :P \n");
    return;
  }
  oldstack = stack;
  size = next_space(str);
  *size++ = 0;
  new_size = atoi(size);

  /*change minutes to seconds */
  new_size *= 60;

  /* can't check with new_size == 0 as we need that for unlagging */
  /* check for duff command syntax */
  if (strlen(size) == 0)
  {
    tell_player(p, " Format: lag <player> <time in minutes>\n");
    return;
  }
  /* find them and return if they're not on */
  p2 = find_player_global(str);
  if (!p2)
  {
    if (new_size == 0)
      remove_saved_lag(p, str);
    return;
  }
  /* thou shalt not lag those above you */
  if (!check_privs(p->residency, p2->residency))
  {
    tell_player(p, " You can't do that... Bad idea... !!\n");
    sprintf(oldstack, " -=*> %s tried to lag you.\n", p->name);
    stack = end_string(oldstack);
    tell_player(p2, oldstack);
    stack = oldstack;
    return;
  }
  /* check for silly or nasty amounts of lag */
  if (new_size < 0)
  {
    plag = 1;			/* yes, we can lag forever now :P */
    new_size = 1;		/* just for the hell of it */
  }
  if (new_size > 600 && !(p->residency & ADMIN))
  {
    tell_player(p, "That's kinda excessive, set to 10 minutes.\n");
    new_size = 600;
  }
  /* lag 'em */
  p2->lagged = new_size;
  if (plag)
    p2->system_flags |= SAVE_LAGGED;
  else
    p2->system_flags &= ~SAVE_LAGGED;
  /* report success */
  if (new_size == 0)
  {
    stack = oldstack;
    sprintf(stack, " -=*> %s unlags %s. (darn!)\n", p->name, p2->name);
    stack = end_string(stack);
    su_wall(oldstack);
    stack = oldstack;
    LOGF("lag", "%s unlags %s", p->name, p2->name);
  }
  else
  {
    stack = oldstack;
    if (plag)
      sprintf(oldstack, " -=*> %s lags %s like a bitch ass for...ever !!\n", p->name, p2->name);
    else
      sprintf(oldstack, " -=*> %s lags %s like a bitch ass for %d minutes.\n", p->name, p2->name, new_size / 60);
    stack = end_string(oldstack);
    su_wall(oldstack);
    stack = oldstack;
    if (plag)
      sprintf(oldstack, "%s lags %s for...ever -- muhahahaha", p->name,
	      p2->name);

    else
      sprintf(oldstack, "%s lags %s for %d minutes", p->name,
	      p2->name, new_size / 60);
    stack = end_string(oldstack);
    log("lag", oldstack);
    if (plag)
      log("forever", oldstack);
    stack = oldstack;
  }
}

/* manual command to sync files to disk */

void sync_files(player * p, char *str)
{
  if (!isalpha(*str))
  {
    tell_player(p, " Argument must be a letter.\n");
    return;
  }
  sync_to_file(tolower(*str), 1);
  tell_player(p, " Sync succesful.\n");
}

/* manual retrieve from disk */

void restore_files(player * p, char *str)
{

  if (!isalpha(*str))
  {
    tell_player(p, " Argument must be a letter.\n");
    return;
  }
  remove_entire_list(tolower(*str));
  hard_load_one_file(tolower(*str));
  tell_player(p, " Restore succesful.\n");
}


/* shut down the program */

void pulldown(player * p, char *str)
{
  char *oldstack, *reason, *i;

  if (p->flags & BLOCK_SU)
  {
    tell_player(p, " You need to be on_duty for that\n");
    return;
  }
  oldstack = stack;
  command_type &= ~HIGHLIGHT;

  if (!(p->residency & (LOWER_ADMIN | ADMIN)))
  {
    /* SUs can see a shutdown but not start one */
    if (*str)
    {
      /* lest they try... */
      tell_player(p, " Access denied :P\n");
      return;
    }
    if (shutdown_count > 1)
      /* if a shutdown is in progress */
    {
      /* contruct the message to tell them and send it to them */
      sprintf(stack, "\n %s, in %d seconds.\n", shutdown_reason, shutdown_count);
      stack = end_string(stack);
      tell_player(p, oldstack);
      /* clean up stack and exit */
      stack = oldstack;
      return;
    }
#ifdef AUTOSHUTDOWN
    else if (auto_sd == 1)
    {
      tell_player(p, " Auto-shutdown is active.\n");
      return;
    }
#endif
    else
    {
      /* tell them no joy */
      tell_player(p, " No shutdown in progress.\n");
      return;
    }
  }
  if (!*str)
  {
    if (shutdown_count > -1)
    {
      sprintf(stack, "\n %s, in %d seconds.\n  \'shutdown -1\' to abort.\n\n",
	      shutdown_reason, shutdown_count);
      stack = end_string(stack);
      tell_player(p, oldstack);
      stack = oldstack;
      return;
    }
    else
    {
      tell_player(p, " Format: shutdown <countdown> [<reason>]\n");
      return;
    }
  }
  reason = strchr(str, ' ');
  if (!reason)
  {
    sprintf(shutdown_reason, "%s is shutting the program down - it is "
	    "probably for a good reason too\n", p->name);
  }
  else
  {
    *reason++ = 0;
    sprintf(shutdown_reason, "%s is shutting the program down - %s",
	    p->name, reason);
  }
  if (!strcmp(str, "-1"))
  {
    shutdown_reason[0] = '\0';

    if (shutdown_count < 1)
    {
      tell_player(p, " There is no shutdown to abort! Fool!\n");
      return;
    }
    if (shutdown_count < 300)
    {
      raw_wall("\n\nShutdown aborted "
	       "(If you ever knew one was in progress...)\n\n");
    }
#ifdef AUTOSHUTDOWN
    else if (auto_sd == 1)
    {
      tell_player(p, " Auto-shutdown is active.\n");
      return;
    }
#endif
    else
    {
      tell_player(p, " Shutdown Aborted.\n");
    }
    shutdown_count = -1;
    return;
  }
  i = str;
  while (*i != 0)
  {
    if (!isdigit(*i))
    {
      tell_player(p, " Format: shutdown <countdown> [<reason>]\n");
      return;
    }
    i++;
  }
  shutdown_count = atoi(str);
  sprintf(stack, " -=*> Program set to shutdown in %d seconds...\n",
	  shutdown_count);
  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
  command_type &= ~HIGHLIGHT;
}

/* wall to everyone, non blockable */

void wall(player * p, char *str)
{
  char *oldstack = stack;

  CHECK_DUTY(p);

  if (!*str)
  {
    tell_player(p, " Format: wall <message>\n");
    return;
  }

  sprintf(stack, "  %s announces -=*> %s ^N<*=-\007\n", p->name, str);
  stack = end_string(stack);

  command_type |= HIGHLIGHT;
  raw_wall(oldstack);
  command_type &= ~HIGHLIGHT;
  stack = oldstack;
  LOGF("wall", "%s walls \"%s\"", p->name, str);
}

void emoted_wall(player * p, char *str)
{
  char *oldstack;

  if ((p->flags & BLOCK_SU) && !(p->residency & HCADMIN))
  {
    tell_player(p, "You cant use ewall when off_duty.\n");
    return;
  }

  oldstack = stack;
  if (!*str)
  {
    tell_player(p, " Format: ewall <emote>\n");
    return;
  }
  if (emote_no_break(*str))
    sprintf(oldstack, "  -=*> %s%s ^N<*=- \007\n", p->name, str);
  else
    sprintf(oldstack, "  -=*> %s %s ^N<*=- \007\n", p->name, str);

  stack = end_string(oldstack);
  command_type |= HIGHLIGHT;
  raw_wall(oldstack);
  command_type &= ~HIGHLIGHT;
  stack = oldstack;
  if (emote_no_break(*str))
    LOGF("wall", "%s%s", p->name, str);
  else
    LOGF("wall", "%s %s", p->name, str);
}

void thinkin_wall(player * p, char *str)
{
  char *oldstack;

  if ((p->flags & BLOCK_SU) && !(p->residency & HCADMIN))
  {
    tell_player(p, "You cant use twall when off_duty.\n");
    return;
  }

  oldstack = stack;
  if (!*str)
  {
    tell_player(p, " Format: twall <think>\n");
    return;
  }
  sprintf(oldstack, "  -=*> %s thinks . o O ( %s ^N) <*=- \007\n",
	  p->name, str);

  stack = end_string(oldstack);
  command_type |= HIGHLIGHT;
  raw_wall(oldstack);
  command_type &= ~HIGHLIGHT;
  stack = oldstack;
  LOGF("wall", "%s thinks . o O ( %s )", p->name, str);
}


void edtime(player * p, char *str)
{
  player *p2;
  char *oldstack;
  int temp_time;
  char *temp;

  oldstack = stack;
  temp = next_space(str);
  *temp++ = 0;

  if (!*str || !*temp)
  {
    tell_player(p, " Format: edtime <player> [-] time (hours)>\n");
    return;
  }
  if (p->flags & BLOCK_SU)
  {
    tell_player(p, " Go back on duty first beavis.\n");
    return;
  }
  /* p2 = find_player_absolute_quiet(str); */
  p2 = find_player_global(str);
  if ((!p2))
  {
    tell_player(p, " Only when the player is logged on, chief!\n");
    stack = oldstack;
    return;
  }
  if ((p2->residency == NON_RESIDENT))
  {
    tell_player(p, " That player is non-resident!\n");
    stack = oldstack;
    return;
  }
  if ((p2->residency == BANISHD || p2->residency == SYSTEM_ROOM))
  {
    tell_player(p, " That is a banished NAME, or System Room.\n");
    stack = oldstack;
    return;
  }
  if (p == p2)
  {
    TELLPLAYER(p, " Sorry. That would be cheating. That's immoral.\n");
    LOGF("edit", "%s tried to edit their own time (%d)", p->name, p->total_login);
    AUWALL(" -=*> %s tried to edit their own time...\n", p->name);
    p->total_login /= 4;
    return;
  }
  sprintf(oldstack, "\n -=*> %s has changed your total login time...\n",
	  p->name);
  stack = end_string(oldstack);
  tell_player(p2, oldstack);
  stack = oldstack;
  temp_time = atoi(temp);
  /* convert to hours */
  temp_time *= 3600;
  p2->total_login += temp_time;
  sprintf(stack, "%s changed the total login time of %s by %s hours.",
	  p->name, p2->name, temp);
  stack = end_string(stack);
  log("edtime", oldstack);
  save_player(p2);
  stack = oldstack;
  sprintf(stack, " -=*> %s changes the total login time of %s.\n", p->name,
	  p2->name);
  stack = end_string(stack);
  au_wall(oldstack);
  stack = oldstack;
}

/* List the Super Users who're on */


void lsu(player * p, char *str)
{
  int count = 0;
  char *oldstack, *prestack, middle[80];
  player *scan;
  int current_supers;

#ifdef INTERCOM
  if (str && *str == '@')
  {
    do_intercom_lsu(p, str);
    return;
  }
#endif

  current_supers = count_su();

  oldstack = stack;

  sprintf(middle, "%ss connected", get_config_msg("staff_name"));
  pstack_mid(middle);

  for (scan = flatlist_start; scan; scan = scan->flat_next)
  {
    prestack = stack;
    if (scan->location && scan->residency & PSU)
    {
      if (p->residency & PSU && *str != '-')
      {
	count++;
	*stack = ' ';
	stack++;
	sprintf(stack, " %-18s  ", scan->name);
	stack = strchr(stack, 0);
	if (scan->saved_residency & CODER)
	  sprintf(stack, "%-18.18s  ", get_config_msg("coder_name"));
	else if (scan->saved_residency & HCADMIN)
	  sprintf(stack, "%-18.18s  ", get_config_msg("hc_name"));
	else if (scan->saved_residency & ADMIN)
	  sprintf(stack, "%-18.18s  ", get_config_msg("admin_name"));
	else if (scan->saved_residency & LOWER_ADMIN)
	  sprintf(stack, "%-18.18s  ", get_config_msg("la_name"));
	else if (scan->saved_residency & ASU)
	  sprintf(stack, "%-18.18s  ", get_config_msg("asu_name"));
	else if (scan->saved_residency & SU)
	  sprintf(stack, "%-18.18s  ", get_config_msg("su_name"));
	else if (scan->saved_residency & PSU)
	  sprintf(stack, "%-18.18s  ", get_config_msg("psu_name"));
	stack = strchr(stack, 0);
	if (scan->flags & BLOCK_SU)
	{
	  strcpy(stack, "  Off Duty  ");
	  stack = strchr(stack, 0);
	}
	else if ((scan->flags & OFF_LSU) && !(scan->flags & BLOCK_SU))
	{
	  strcpy(stack, "  Invisible ");
	  stack = strchr(stack, 0);
	}
	else if ((!strcmp(scan->location->owner->lower_name, SYS_ROOM_OWNER) ||
		  !strcmp(scan->location->owner->lower_name, "system")))
	{
	  if (!strcasecmp(scan->location->id, "room"))
	    sprintf(stack, "  main      ");
	  else
	    sprintf(stack, "  %-10.10s", scan->location->id);
	}
	else
	{
	  strcpy(stack, "            ");
	}
	stack = strchr(stack, 0);
	sprintf(stack, " %3d:%.2d idle ", scan->idle / 60, scan->idle % 60);
	stack = strchr(stack, 0);
	if ((scan->tag_flags & (BLOCK_SHOUT | BLOCK_TELLS | BLOCKCHANS | SINGBLOCK)))
	{
	  strcpy(stack, " Blk ");
	  stack = strchr(stack, 0);
	  if ((scan->tag_flags & BLOCK_TELLS))
	  {
	    strcpy(stack, ">");
	    stack = strchr(stack, 0);
	  }
	  if ((scan->tag_flags & BLOCK_SHOUT))
	  {
	    strcpy(stack, "!");
	    stack = strchr(stack, 0);
	  }
	  if ((scan->tag_flags & BLOCKCHANS))
	  {
	    strcpy(stack, "%");
	    stack = strchr(stack, 0);
	  }
	  if ((scan->tag_flags & SINGBLOCK))
	  {
	    strcpy(stack, "$");
	    stack = strchr(stack, 0);
	  }
	}			/* end scan for blocks */
	*stack++ = '\n';
	/* ok here's what the ressies see (i hope) */
      }
      /* end if psu */
      /* This used to be really complicated but stupid since all the su's
         would think it was a bug if they weren't listed on LSU - so now they
         all are --Silver */
      else if (!(scan->flags & OFF_LSU) && !(scan->flags & BLOCK_SU) && scan->residency & (SU | ASU | LOWER_ADMIN | ADMIN | HCADMIN | CODER))
      {
	count++;
	*stack = ' ';
	stack++;
	sprintf(stack, " %-18s  ", scan->name);
	stack = strchr(stack, 0);

	if (scan->saved_residency & CODER)
	  sprintf(stack, "%-18.18s  ", get_config_msg("coder_name"));
	else if (scan->saved_residency & HCADMIN)
	  sprintf(stack, "%-18.18s  ", get_config_msg("hc_name"));
	else if (scan->saved_residency & ADMIN)
	  sprintf(stack, "%-18.18s  ", get_config_msg("admin_name"));
	else if (scan->saved_residency & LOWER_ADMIN)
	  sprintf(stack, "%-18.18s  ", get_config_msg("la_name"));
	else if (scan->saved_residency & ASU)
	  sprintf(stack, "%-18.18s  ", get_config_msg("asu_name"));
	else if (scan->saved_residency & SU)
	  sprintf(stack, "%-18.18s  ", get_config_msg("su_name"));
	stack = strchr(stack, 0);

	if ((!strcmp(scan->location->owner->lower_name, SYS_ROOM_OWNER) ||
	     !strcmp(scan->location->owner->lower_name, "system")))
	{
	  if (!strcasecmp(scan->location->id, "room"))
	    sprintf(stack, "  main        ");
	  else
	    sprintf(stack, "  %-12s", scan->location->id);
	}
	else
	{
	  strcpy(stack, "              ");
	}
	stack = strchr(stack, 0);
	sprintf(stack, " %3d:%.2d idle", scan->idle / 60, scan->idle % 60);
	stack = strchr(stack, 0);
	*stack++ = '\n';
      }				/* end if '-' or ressie */
    }				/* end scan one player -- ? */
  }				/* end for */
  if (count > 1)
    sprintf(middle, "There are %d %ss connected", count,
	    get_config_msg("staff_name"));
  else if (count == 1)
    sprintf(middle, "There is only 1 %s connected",
	    get_config_msg("staff_name"));
  pstack_mid(middle);
  *stack++ = 0;
  if (count)
    tell_player(p, oldstack);
  else
  {
    TELLPLAYER(p, " -=*> There are no %ss on at the moment. \n",
	       get_config_msg("staff_name"));

    TELLPLAYER(p, " -=*> If you want to get residency, send a mail to %s\n"
	  " -=*> If you have any problems use the \"emergency\" command.\n",
	       get_config_msg("talker_email"));
  }
  stack = oldstack;
}

/* List the Newbies that're on */

void lnew(player * p, char *str)
{
  char *oldstack, middle[80];
  int count = 0;
  player *scan;

  oldstack = stack;
  command_type = EVERYONE;
  pstack_mid("Newbies on");
  for (scan = flatlist_start; scan; scan = scan->flat_next)
  {
    if (scan->residency == NON_RESIDENT && scan->location)
    {
      count++;
      sprintf(stack, "%-20s ", scan->name);
      stack = strchr(stack, 0);
      sprintf(stack, "%-40s ", get_address(scan, p));
      stack = strchr(stack, 0);
      if (scan->assisted_by[0] != '\0')
      {
	sprintf(stack, "[%s]", scan->assisted_by);
	stack = strchr(stack, 0);
      }
      *stack++ = '\n';
    }
  }

  if (count > 1)
    sprintf(middle, "There are %d Newbies connected", count);
  else if (count == 1)
    sprintf(middle, "There is only 1 Newbie connected");

  pstack_mid(middle);
  *stack++ = 0;

  if (count == 0)
    tell_player(p, " No newbies on at the moment.\n");
  else
    tell_player(p, oldstack);
  stack = oldstack;
}


/* lets see if we cant spoon this for list gits */

void listgits(player * p, char *str)
{
  char *oldstack, bottom[50];
  int count = 0;
  player *scan;


  oldstack = stack;
  command_type = EVERYONE;
  pstack_mid("Gits on");
  ADDSTACK("\n");
  for (scan = flatlist_start; scan; scan = scan->flat_next)
  {
    if (scan->git_string[0] != '\0')
    {
      count++;
      sprintf(stack, "%-20s ", scan->name);
      stack = strchr(stack, 0);
      sprintf(stack, "%-40s ", get_address(scan, p));
      stack = strchr(stack, 0);
      sprintf(stack, "\n  [%s]", scan->git_by);
      stack = strchr(stack, 0);
      sprintf(stack, " : %s", scan->git_string);
      stack = strchr(stack, 0);
      *stack++ = '\n';
    }
  }

  ADDSTACK("\n");

  if (count != 1)
    sprintf(bottom, "There are %d gits on at the moment", count);
  else
    sprintf(bottom, "There is only 1 git on at the moment");

  pstack_mid(bottom);
  *stack++ = 0;

  if (count == 0)
    tell_player(p, " No dorkballs on at the moment.\n");
  else
    tell_player(p, oldstack);
  stack = oldstack;
}




/* And why not list tweedles? :P */

void listdumb(player * p, char *str)
{
  char *oldstack;
  int count = 0;
  player *scan;

  oldstack = stack;
  command_type = EVERYONE;
  pstack_mid("Tweedles on");
  for (scan = flatlist_start; scan; scan = scan->flat_next)
  {
    if (scan->flags & FROGGED)
    {
      count++;
      sprintf(stack, "%-20s ", scan->name);
      stack = strchr(stack, 0);
      sprintf(stack, "%-40s ", get_address(scan, p));
      stack = strchr(stack, 0);
      *stack++ = '\n';
    }
  }

  sprintf(stack, LINE);
  stack = end_string(stack);

  if (count == 0)
    tell_player(p, " No tweedles on at the moment.\n");
  else
    tell_player(p, oldstack);
  stack = oldstack;
}




/* help for superusers */

void super_help(player * p, char *str)
{
  char *oldstack;
  file h;


  oldstack = stack;
  if (!*str || (!strcasecmp(str, "admin") && !(p->residency & ADMIN)))
  {
    TELLPLAYER(p, " %s help files that you can read are:\n   "
	       "shelp basic, advanced, commands, res",
	       get_config_msg("staff_name"));
#ifdef INTERCOM
    tell_player(p, ", intercom");
#endif
    tell_player(p, "\n");
    return;
  }
  if (*str == '.')
  {
    tell_player(p, " Uh-uh, cant do that ...\n");
    return;
  }
  sprintf(stack, "doc/%s.doc", str);
  stack = end_string(stack);
  h = load_file_verbose(oldstack, 0);
  if (h.where)
  {
    if (*(h.where))
    {
      if (p->custom_flags & NO_PAGER)
	tell_player(p, h.where);
      else
	pager(p, h.where);
    }
    else
    {
      tell_player(p, " Couldn't find that help file ...\n");
    }
    FREE(h.where);
  }
  stack = oldstack;
}

/* assist command */

void assist_player(player * p, char *str)
{
  char *oldstack = stack;
  player *p2, *p3;

  CHECK_DUTY(p);

  if (!*str)
  {
    tell_player(p, " Format: assist <person>\n");
    return;
  }

  p2 = find_player_global(str);
  if (!p2)
    return;

  if (!p2->location)
  {
    tell_player(p, " Easy tiger. Wait till they've actually connected properly!\n");
    return;
  }

  if (p2 == p)
  {
    TELLPLAYER(p2, " -=*> The assist message reads ...\n%s",
	       get_admin_msg("assist_msg"));
    return;
  }

  if (p2->residency != NON_RESIDENT)
  {
    tell_player(p, " That person isn't a newbie though ...\n");
    return;
  }

  if (p2->flags & ASSISTED)
  {
    p3 = find_player_absolute_quiet(p2->assisted_by);
    if (p3)
    {
      if (p != p3)
      {
	sprintf(stack, " That person is already assisted by %s.\n",
		p2->assisted_by);
      }
      else
      {
	sprintf(stack, " That person has already been assisted by %s."
		" Oh! That's you that is! *smirk*\n",
		p2->assisted_by);
      }
      stack = end_string(stack);
      tell_player(p, oldstack);
      stack = oldstack;
      return;
    }
  }

  p2->flags |= ASSISTED;
  strcpy(p2->assisted_by, p->name);

  oldstack = stack;
  tell_player(p2, get_admin_msg("assist_msg"));

  SW_BUT(p, " -=*> %s assists %s.\n", p->name, p2->name);

  TELLPLAYER(p, " You assist %s.\n", p2->name);

  LOGF("assist", "%s assists %s", p->name, p2->name);
}


/* dibbs command */

void dibbs(player * p, char *str)
{
  char *oldstack;
  player *p2;

  if (p->flags & BLOCK_SU)
  {
    tell_player(p, " Go on_duty first.\n");
    return;
  }
  oldstack = stack;
  if (!*str)
  {
    tell_player(p, " Format: dibbs <person>\n");
    return;
  }

  p2 = find_player_global(str);
  if (!p2)
    return;

  if (p2 == p)
  {
    tell_player(p, " Idiot. Thats you!\n");
    return;
  }

  if (p2->residency != NON_RESIDENT)
  {
    tell_player(p, " That person is a resident.\n");
    return;
  }
  if (!strcasecmp(p2->assisted_by, p->name))
  {
    tell_player(p, "You've already called dibbs on em ....\n");
    return;
  }
  SW_BUT(p, " -=*> %s calls dibbs on %s.\n", p->name, p2->name);
  TELLPLAYER(p, " You call dibbs on %s.\n", p2->name);
  LOGF("assist", "%s calls dibbs on %s", p->name, p2->name);
  strcpy(p2->assisted_by, p->name);	/* Needed line!! --Silver */
}

/* Confirm if password and email are set on a resident */

void confirm_password(player * p, char *str)
{
  char *oldstack;
  player *p2;


  if (!*str)
  {
    tell_player(p, " Format: confirm <name>\n");
    return;
  }
  p2 = find_player_global(str);
  if (!p2)
    return;

  if (p2->residency == NON_RESIDENT)
  {
    tell_player(p, " That person is not a resident.\n");
    return;
  }
  oldstack = stack;

  p2->residency |= NO_SYNC;
  /* check email */
  if (p2->email[0] == 2)
  {
    strcpy(stack, " Email has not yet been set.");
  }
  else if (p2->email[0] == ' ')
  {
    strcpy(stack, " Email validated set.");
    p2->residency &= ~NO_SYNC;
  }
  else if (!strstr(p2->email, "@") || !strstr(p2->email, "."))
  {
    strcpy(stack, " Probably not a correct email.");
    p2->residency &= ~NO_SYNC;
  }
  else
  {
    strcpy(stack, " Email set.");
    p2->residency &= ~NO_SYNC;
  }
  stack = strchr(stack, 0);
  if (p2->email[0] != 2 && p2->email[0] != ' ')
  {
    if (p->residency & ADMIN || !(p2->custom_flags & PRIVATE_EMAIL))
    {
      sprintf(stack, " - %s", p2->email);
      stack = strchr(stack, 0);
      if (p2->custom_flags & PRIVATE_EMAIL)
      {
	strcpy(stack, " (private)\n");
      }
      else
      {
	strcpy(stack, "\n");
      }
    }
    else
    {
      strcpy(stack, "\n");
    }
  }
  else
  {
    strcpy(stack, "\n");
  }
  stack = strchr(stack, 0);

  /* password */
  if (p2->password[0] && p2->password[0] != -1)
  {
    strcpy(stack, " Password set.\n");
  }
  else
  {
    strcpy(stack, " Password NOT-set.\n");
    p2->residency |= NO_SYNC;
  }
  stack = strchr(stack, 0);

  if (p2->residency & NO_SYNC)
    sprintf(stack, " Character '%s' won't be saved.\n", p2->name);
  else
    sprintf(stack, " Character '%s' will be saved.\n", p2->name);
  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
}

/* unconverse, get idiots out of converse mode */

void unconverse(player * p, char *str)
{
  player *p2;
  saved_player *sp;
  char *oldstack;


  if (p->flags & BLOCK_SU)
  {
    tell_player(p, " Go on_duty first... Please..... Thanx.\n");
    return;
  }
  oldstack = stack;
  if (!*str)
  {
    tell_player(p, " Format: unconverse <player>\n");
    return;
  }
  lower_case(str);
  p2 = find_player_global_quiet(str);
  if (!p2)
  {
    tell_player(p, " Player not logged on, checking saved player files...\n");
    sp = find_saved_player(str);
    if (!sp)
    {
      tell_player(p, " Can't find saved player file.\n");
      return;
    }
    if (!(sp->residency & SU) && !(sp->residency & ADMIN))
    {
      if (!(sp->custom_flags & CONVERSE))
      {
	tell_player(p, " They aren't IN converse mode!!!\n");
	return;
      }
      sp->custom_flags &= ~CONVERSE;
      set_update(*str);
      sprintf(stack, " You take \'%s' out of converse mode.\n",
	      sp->lower_name);
      stack = end_string(stack);
      tell_player(p, oldstack);
      stack = oldstack;
    }
    else
    {
      tell_player(p, " You can't do that to them!\n");
    }
    return;
  }
  if (!(p2->custom_flags & CONVERSE))
  {
    tell_player(p, " But they're not in converse mode!!!\n");
    return;
  }
  if (!(p2->residency & SU) && !(p2->residency & ADMIN))
  {
    p2->custom_flags &= ~CONVERSE;
    p2->mode &= ~CONV;
    sprintf(stack, " -=*> %s has taken you out of converse mode.\n",
	    p->name);
    stack = end_string(stack);
    tell_player(p2, oldstack);
    stack = oldstack;
    do_prompt(p2, p2->prompt);
    sprintf(stack, " You take %s out of converse mode.\n", p2->name);
    stack = end_string(stack);
    tell_player(p, oldstack);
    stack = oldstack;
  }
  else
  {
    tell_player(p, " You can't do that to them!\n");
    sprintf(stack, " -=*> %s tried to unconverse you!\n", p->name);
    stack = end_string(stack);
    tell_player(p2, oldstack);
    stack = oldstack;
  }
}

void unjail(player * p, char *str)
{
  char *oldstack;
  player *p2, dummy;


  oldstack = stack;

  if (p->flags & BLOCK_SU)
  {
    tell_player(p, " Go on_duty first.\n");
    return;
  }
  if (!*str)
  {
    tell_player(p, " Format: unjail <player>\n");
    return;
  }
  if (!strcasecmp(str, "me"))
    p2 = p;
  else
    p2 = find_player_global(str);
  if (!p2)
  {
    tell_player(p, " Checking saved files... ");
    strcpy(dummy.lower_name, str);
    lower_case(dummy.lower_name);
    dummy.fd = p->fd;
    if (!load_player(&dummy))
    {
      tell_player(p, " Not found.\n");
      return;
    }
    else
    {
      tell_player(p, "\n");
      p2 = &dummy;
      p2->location = (room *) - 1;
    }
  }
  if (p2 == p)
  {
    if (p->location == prison)
    {
      tell_player(p, " You struggle to open the door, but to no avail.\n");
      sprintf(stack, " -=*> %s tries to unjail %s. *grin*\n", p->name,
	      self_string(p));
      stack = end_string(stack);
      su_wall_but(p, oldstack);
      stack = oldstack;
    }
    else
    {
      tell_player(p, " But you're not in jail!\n");
    }
    return;
  }
  if (p2 == &dummy)
  {
    if (!(p2->system_flags & SAVEDJAIL))
    {
      tell_player(p, " Erm, how can I say this? They're not in jail...\n");
      return;
    }
  }
  else if (p2->jail_timeout == 0 || p2->location != prison)
  {
    tell_player(p, " Erm, how can I say this? They're not in jail...\n");
    return;
  }
  p2->jail_timeout = 0;
  p2->system_flags &= ~SAVEDJAIL;
  if (p2 != &dummy)
  {
    sprintf(stack, " -=*> %s releases you from prison.\n", p->name);
    stack = end_string(stack);
    tell_player(p2, oldstack);
    stack = oldstack;
    move_to(p2, sys_room_id(ENTRANCE_ROOM), 0);
  }
  sprintf(stack, " -=*> %s releases %s from jail.\n", p->name, p2->name);
  stack = end_string(stack);
  su_wall(oldstack);
  stack = oldstack;
  sprintf(stack, "%s releases %s from jail.", p->name, p2->name);
  stack = end_string(stack);
  log("jail", oldstack);
  stack = oldstack;
  if (p2 == &dummy)
  {
    save_player(&dummy);
  }
}

/* cut down version of lsu() to just return number of SUs on */
/* This shows only sus that RESSIES can see.. not the REAL amount on */
int count_su()
{
  int count = 0;
  player *scan;

  for (scan = flatlist_start; scan; scan = scan->flat_next)
    if (scan->residency & SU && scan->location &&
	!(scan->flags & (BLOCK_SU | OFF_LSU))
	&& scan->idle < 300)
      count++;

  return count;
}

/* needed for statistics */
int true_count_su()
{
  int count = 0;
  player *scan;

  for (scan = flatlist_start; scan; scan = scan->flat_next)
    if (scan->residency & SU && scan->location)
      count++;

  return count;
}

/*
   Now that we know:
   the number of SUs,
   the number of newbies,
   and the number of ppl on the prog (current_players),
   we can output some stats
 */

/* Cleaned up drastically -- Silver */

void player_stats(player * p, char *str)
{
  char *oldstack, top[70];
  player *scan;
  int new = 0, sus = 0;


  oldstack = stack;
  sprintf(top, "Current %s player stats", get_config_msg("talker_name"));
  pstack_mid(top);

  for (scan = flatlist_start; scan; scan = scan->flat_next)
  {
    if (scan->residency == NON_RESIDENT && scan->location)
      new++;
    if (scan->residency & PSU && scan->location)
      sus++;
  }

  sprintf(stack, "\n  Current players on : %4d\n"
	  "        (Newbies on) : %4d\n"
	  "         (Supers on) : %4d\n"
	  "       (Normal res.) : %4d\n\n"
	  LINE
	  "\n",
	  current_players,
	  new, sus, (current_players - (sus + new)));
  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
}

/* Go to the SUs study */

void go_comfy(player * p, char *str)
{

  command_type |= ADMIN_BARGE;
  if (p->location == comfy)
  {
    tell_player(p, " You're already in the study!\n");
    return;
  }
  if (p->no_move)
  {
    tell_player(p, " You seem to be stuck to the ground.\n");
    return;
  }
  move_to(p, sys_room_id("comfy"), 0);
}

/* Tell you what mode someone is in */

void mode(player * p, char *str)
{
  player *p2;
  char *oldstack;


  oldstack = stack;
  if (!*str)
  {
    tell_player(p, " Format: mode <player>\n");
    return;
  }
  p2 = find_player_global(str);
  if (!p2)
    return;

  if (p2->mode == NONE)
  {
    sprintf(stack, " %s is in no particular mode.\n", p2->name);
  }
  else if (p2->mode & PASSWORD)
  {
    sprintf(stack, " %s is in Password Mode.\n", p2->name);
  }
  else if (p2->mode & ROOMEDIT)
  {
    sprintf(stack, " %s is in Room Mode.\n", p2->name);
  }
  else if (p2->mode & MAILEDIT)
  {
    sprintf(stack, " %s is in Mail Mode.\n", p2->name);
  }
  else if (p2->mode & NEWSEDIT)
  {
    sprintf(stack, " %s is in News Mode.\n", p2->name);
  }
  else if (p2->mode & CONV)
  {
    sprintf(stack, " %s is in Converse Mode.\n", p2->name);
  }
  else
  {
    sprintf(stack, " Ermmm, %s doesn't appear to be in any mode at all.\n",
	    p2->name);
  }
  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
}
void abort_shutdown(player * p, char *str)
{
  pulldown(p, "-1");
  return;
}

void echoroomall(player * p, char *str)
{
  char *oldstack;
  oldstack = stack;
  if (strlen(str) < 1)
  {
    tell_player(p, "Usage: becho <message>\n");
    return;
  }
  sprintf(oldstack, "%s\n", str);
  stack = end_string(oldstack);
  raw_room_wall(p->location, oldstack);
  stack = oldstack;
  return;
}
void echoall(player * p, char *str)
{
  char *oldstack;
  oldstack = stack;
  if (strlen(str) < 1)
  {
    tell_player(p, "Usage: aecho <message>\n");
    return;
  }
  sprintf(oldstack, "%s\n", str);
  stack = end_string(oldstack);
  raw_wall(oldstack);
  stack = oldstack;
  return;
}


/* chlim command, combines all 5 chlims.  */
/* but now there are 6 !! */
/* soon to be 7 !! */

void change_player_limits(player * p, char *str)
{
  char *size;
  int new_size;
  char *oldstack;
  player *p2;
  player dummy;
  char *pl;


  oldstack = stack;
  pl = next_space(str);
  *pl++ = 0;
  size = next_space(pl);
  *size++ = 0;
  new_size = atoi(size);

  if (!*str || !pl || !size)
  {
    tell_player(p, " Format: chlim <area> <player> <new limit>\n"
	   " Valid areas are: mail, list, room, auto, exit, alias, item\n");
    stack = oldstack;
    return;
  }
  if (new_size < 0)
  {
    tell_player(p, " Now try a _positive_ limit...\n");
    stack = oldstack;
    return;
  }
  p2 = find_player_absolute_quiet(pl);
  if (!p2)
  {
    /* load them if they're not logged in */
    strcpy(dummy.lower_name, pl);
    lower_case(dummy.lower_name);
    dummy.fd = p->fd;
    /* if they don't exist, say so and return */
    if (!load_player(&dummy))
    {
      tell_player(p, " That player does not exist.\n");
      stack = oldstack;
      return;
    }
    p2 = &dummy;
  }
  /* ok lets get the valid areas */
  if (strcasecmp(str, "room") && strcasecmp(str, "list") &&
      strcasecmp(str, "alias") && strcasecmp(str, "item") &&
      (!(p->residency & ADMIN) || (strcasecmp(str, "exit") &&
		       strcasecmp(str, "auto") && strcasecmp(str, "mail"))))
  {

    tell_player(p, " You can't change THAT limit !! \n");
    stack = oldstack;
    return;
  }
  if (!check_privs(p->residency, p2->residency))
  {
    /* now now, no messing with your superiors */
    tell_player(p, " You can't do that !!\n");
    TELLPLAYER(p2, " -=*> %s tried to change your %s limit - but failed.\n", p->name, str);
    stack = oldstack;
    return;
  }
  /* otherwise change the limit */
  if (!strcasecmp(str, "list"))
    p2->max_list = new_size;
  else if (!strcasecmp(str, "room"))
    p2->max_rooms = new_size;
  else if (!strcasecmp(str, "alias"))
    p2->max_alias = new_size;
  else if (!strcasecmp(str, "mail"))
    p2->max_mail = new_size;
  else if (!strcasecmp(str, "item"))
    p2->max_items = new_size;
  else if (!strcasecmp(str, "exit"))
    p2->max_exits = new_size;
  else if (!strcasecmp(str, "auto"))
    p2->max_autos = new_size;

  /* and if they are logged in, tell them */
  if (p2 != &dummy)
  {
    sprintf(oldstack, " -=*> %s has changed your %s limit to %d.\n",
	    p->name, str, new_size);
    stack = end_string(oldstack);
    tell_player(p2, oldstack);
  }
  else
  {
    save_player(&dummy);
  }
  /* now log that change please? */
  stack = oldstack;
  if (p2 != &dummy)
    sprintf(stack, "%s changed %s's %s limit to %d",
	    p->name, p2->name, str, new_size);
  else
    sprintf(stack, "%s changed %s's %s limit to %d - (logged out)",
	    p->name, p2->name, str, new_size);
  stack = end_string(stack);
  log("chlim", oldstack);

  TELLPLAYER(p, "%s's %s limit changed to %d", p2->name, str, new_size);
  if (p2 == &dummy)
    tell_player(p, " (logged out)");
  tell_player(p, "\n");

  stack = oldstack;
}

void extend(player * p, char *str)
{
  char *oldstack;
  player *p2;

  oldstack = stack;

  if (!*str)
  {
    tell_player(p, " Format: extend <player>\n");
    stack = oldstack;
    return;
  }
  p2 = find_player_global(str);
  if (!p2)
    return;

  if (!check_privs(p->residency, p2->residency) && (p != p2))
  {
    /* now now, no messing with your superiors */
    tell_player(p, " You can't do that !!\n");
    sprintf(oldstack, " -=*> %s tried to extend your list limit.\n", p->name);
    stack = end_string(oldstack);
    tell_player(p2, oldstack);
    stack = oldstack;
    return;
  }
  if (!(p2->residency & LIST))
  {
    /* whats the point? */
    tell_player(p, " That person has no list to extend.\n");
    stack = oldstack;
    return;
  }
  /* otherwise change the limit */
  if (p2->max_list >= 50 && !(p->residency & ADMIN))
    tell_player(p, " Sorry, their list is too full, ask an admin to extend it.\n");
  else if (p2->max_list >= 45 && !(p->residency & ADMIN))
  {
    p2->max_list = 50;
    TELLPLAYER(p2, " -=*> %s has increased your list limit to 50.\n",
	       p->name);
    TELLPLAYER(p, " %s's list limit increased to 50.\n", p2->name);
    stack = oldstack;
    LOGF("chlim", "%s extended %s's list to 50", p->name, p2->name);
  }
  else
  {
    p2->max_list += 5;
    TELLPLAYER(p2, " -=*> %s has increased your list limit to %d.\n", p->name, p2->max_list);
    TELLPLAYER(p, " %s's list limit increased to %d.\n", p2->name, p2->max_list);
    stack = oldstack;
    LOGF("chlim", "%s extended %s's list to %d", p->name, p2->name, p2->max_list);
  }
  stack = oldstack;
}


void blank_something(player * p, char *str)
{
  char *oldstack;
  player *p2, dummy;
  char *person;


  if (p->flags & BLOCK_SU)
  {
    tell_player(p, " Go on_duty first.\n");
    return;
  }
  oldstack = stack;
  person = next_space(str);
  *person++ = 0;

  if (*person)
  {
    p2 = find_player_absolute_quiet(person);
    if (p2)
    {
      if (!check_privs(p->residency, p2->residency))
      {
	tell_player(p, " You can't do that to THAT person.\n");
	sprintf(stack, " -=*> %s tried to blank %s\'s %s!\n",
		p->name, p2->name, str);
	stack = end_string(stack);
	su_wall_but(p, oldstack);
	stack = oldstack;
	return;
      }
      if (!strcasecmp(str, "prefix"))
	p2->pretitle[0] = 0;
      else if (!strcasecmp(str, "desc"))
	p2->description[0] = 0;
      else if (!strcasecmp(str, "plan"))
	p2->plan[0] = 0;
      else if (!strcasecmp(str, "title"))
	p2->title[0] = 0;
      else if (!strcasecmp(str, "comment"))
	p2->comment[0] = 0;
      else if (!strcasecmp(str, "entermsg"))
	p2->enter_msg[0] = 0;
      else if (!strcasecmp(str, "url"))
	p2->alt_email[0] = 0;
      else if (!strcasecmp(str, "logonmsg"))
	p2->logonmsg[0] = 0;
      else if (!strcasecmp(str, "logoffmsg"))
	p2->logoffmsg[0] = 0;
      else if (!strcasecmp(str, "irl_name"))
	p2->irl_name[0] = 0;
      else if (!strcasecmp(str, "exitmsg"))
	p2->exitmsg[0] = 0;
      else if (!strcasecmp(str, "hometown"))
	p2->hometown[0] = 0;
      else if (!strcasecmp(str, "madefrom"))
	p2->ingredients[0] = 0;
      else if (!strcasecmp(str, "fav1"))
	p2->favorite1[0] = 0;
      else if (!strcasecmp(str, "fav2"))
	p2->favorite2[0] = 0;
      else if (!strcasecmp(str, "fav3"))
	p2->favorite3[0] = 0;
      else if (!strcasecmp(str, "icq"))
	p2->icq = 0;
      else if (!strcasecmp(str, "fmsg"))
	p2->finger_message[0] = 0;
      else if (!strcasecmp(str, "ttt") && p->residency & LOWER_ADMIN)
      {
	p2->ttt_draw = 0;
	p2->ttt_loose = 0;
	p2->ttt_win = 0;
      }
      else
      {
	TELLPLAYER(p, "Wanna tell me what a %s is exactly? =)\n", str);
	return;
      }

      sprintf(stack, " -=*> %s has blanked your %s.\n", p->name, str);
      stack = end_string(stack);
      tell_player(p2, oldstack);
      stack = oldstack;
      sprintf(stack, "%s blanked %s's %s.", p->name, p2->name, str);
      stack = end_string(stack);
      log("blanks", oldstack);
      stack = oldstack;
      sprintf(stack, " -=*> %s has blanked %s's %s.\n", p->name, p2->name, str);
      stack = end_string(stack);
      su_wall(oldstack);
      tell_player(p, " Blanked.\n");
      stack = oldstack;
      return;
    }
    strcpy(dummy.lower_name, person);
    dummy.fd = p->fd;
    if (load_player(&dummy))
    {
      if (!check_privs(p->residency, dummy.residency))
      {
	tell_player(p, " You can't do that to THAT person.\n");
	sprintf(stack, " -=*> %s tried to blank %s\'s %s!\n",
		p->name, dummy.name, str);
	stack = end_string(stack);
	su_wall_but(p, oldstack);
	stack = oldstack;
	return;
      }
      if (!strcasecmp(str, "prefix"))
	dummy.pretitle[0] = 0;
      else if (!strcasecmp(str, "desc"))
	dummy.description[0] = 0;
      else if (!strcasecmp(str, "plan"))
	dummy.plan[0] = 0;
      else if (!strcasecmp(str, "title"))
	dummy.title[0] = 0;
      else if (!strcasecmp(str, "entermsg"))
	dummy.enter_msg[0] = 0;
      else if (!strcasecmp(str, "url"))
	dummy.alt_email[0] = 0;
      else if (!strcasecmp(str, "logonmsg"))
	dummy.logonmsg[0] = 0;
      else if (!strcasecmp(str, "logoffmsg"))
	dummy.logoffmsg[0] = 0;
      else if (!strcasecmp(str, "irl_name"))
	dummy.irl_name[0] = 0;
      else if (!strcasecmp(str, "exitmsg"))
	dummy.exitmsg[0] = 0;
      else if (!strcasecmp(str, "hometown"))
	dummy.hometown[0] = 0;
      else if (!strcasecmp(str, "madefrom"))
	dummy.ingredients[0] = 0;
      else if (!strcasecmp(str, "fav1"))
	dummy.favorite1[0] = 0;
      else if (!strcasecmp(str, "fav2"))
	dummy.favorite2[0] = 0;
      else if (!strcasecmp(str, "fav3"))
	dummy.favorite3[0] = 0;
      else if (!strcasecmp(str, "icq"))
	dummy.icq = 0;
      else if (!strcasecmp(str, "fmsg"))
	dummy.finger_message[0] = 0;
      else if (!strcasecmp(str, "ttt") && p->residency & LOWER_ADMIN)
      {
	dummy.ttt_draw = 0;
	dummy.ttt_loose = 0;
	dummy.ttt_win = 0;
      }
      else
      {
	TELLPLAYER(p, "Wanna tell me what a %s is exactly? =)\n", str);
	return;
      }

      sprintf(stack, "%s blanked %s's %s.", p->name, dummy.name, str);
      stack = end_string(stack);
      log("blanks", oldstack);
      stack = oldstack;
      sprintf(stack, " -=*> %s has blanked %s's %s (logged out)\n", p->name, dummy.name, str);
      stack = end_string(stack);
      su_wall(oldstack);
      stack = oldstack;
      dummy.location = (room *) - 1;
      save_player(&dummy);
      tell_player(p, " Blanked in saved file.\n");
      return;
    }
    else
      tell_player(p, " Can't find that person on the program or in the "
		  "files.\n");
  }
  else
  {
    stack += sprintf(stack, " Format: blank <something> <player>\n"
	     " You can blank: title, plan, desc, comment, prefix, exitmsg, "
		  "url, entermsg, logonmsg, logoffmsg, hometown, irl_name, "
		     "madefrom, fav1, fav2, fav3, icq, fmsg");
    if (p->residency & LOWER_ADMIN)
      stack += sprintf(stack, ", ttt\n");
    else
      stack += sprintf(stack, "\n");
    stack = end_string(stack);
    tell_player(p, oldstack);
    stack = oldstack;
  }
}


/* nice little bump command - might have to make this more nasty... */
void bump_off(player * p, char *str)
{
  player *e;

  if (!*str)
  {
    tell_player(p, " Format: bump <person|newbies>\n");
    return;
  }
  if ((p->flags & BLOCK_SU) && !(p->residency & HCADMIN))
  {
    tell_player(p, " Its generally best to be on_duty to do that...\n");
    return;
  }

  if (!strcasecmp(str, "newbies"))
  {
    TELLPLAYER(p, " Flushing newbies off %s ...\n",
	       get_config_msg("talker_name"));
    for (e = flatlist_start; e; e = e->flat_next)
      if (e->residency == NON_RESIDENT)
	quit(e, 0);
    return;
  }

  e = find_player_global(str);
  if (e)
  {
    if (!check_privs(p->residency, e->residency))
    {
      tell_player(p, " Sorry, you can't...\n");
      tell_player(e, " Someone just tried to bump you...\n");
      LOGF("bump", "%s tried to bump %s off the program", p->name, e->name);
    }
    else
    {
      /* asty again gets moral - shouldnt have let me touch the code */
      AW_BUT(p, " -=*> %s bumps %s off %s!!\n", p->name, e->name,
	     get_config_msg("talker_name"));
      quit(e, 0);
      LOGF("bump", "%s bumped %s off the program", p->name, e->name);
    }
  }
}

/* leaving this in for the novelty value of it ..;) */
void superview(player * p, char *str)
{
  char *oldstack;
  file logb;

  oldstack = stack;

  if (*str)
  {
    tell_player(p, " Format: snews\n");
    return;
  }
  sprintf(stack, "logs/sunews.log");
  stack = end_string(stack);
  logb = load_file_verbose(oldstack, 0);
  if (logb.where)
  {
    if (*(logb.where))
      pager(p, logb.where);
    FREE(logb.where);
  }
  stack = oldstack;
}

void new(player * p, char *str)
{
  char *oldstack;
  file logb;

  oldstack = stack;
  sprintf(stack, "files/version.msg");
  stack = end_string(stack);
  logb = load_file_verbose(oldstack, 0);
  if (logb.where)
  {
    if (*(logb.where))
      pager(p, logb.where);
    FREE(logb.where);
  }
  stack = oldstack;
}

void adminview(player * p, char *str)
{
  char *oldstack;
  file logb;

  oldstack = stack;

  sprintf(stack, "logs/adnews.log");
  stack = end_string(stack);
  logb = load_file_verbose(oldstack, 0);
  if (*(logb.where))
  {
    pager(p, logb.where);
    FREE(logb.where);
  }
  else
    tell_player(p, " No admin news available at the moment.\n");
  stack = oldstack;
}

void adminpost(player * p, char *str)
{
  char *oldstack;
  if (!*str)
  {
    tell_player(p, " Format: apost <message>\n");
    return;
  }
  if (strlen(str) > 500)
  {
    tell_player(p, " spam the logs? Not likely...\n");
    return;
  }
  tell_player(p, " Administrator memo logged ...\n");
  oldstack = stack;
  sprintf(stack, "%s :- %s", p->name, str);
  stack = end_string(stack);
  log("adnews", oldstack);
  stack = oldstack;
}

void list_ministers(player * p, char *str)
{
  int count = 0;
  char *oldstack, *prestack, middle[80];
  player *scan;

  oldstack = stack;
  pstack_mid("Ministers on");
  for (scan = flatlist_start; scan; scan = scan->flat_next)
  {
    prestack = stack;
    /* hey asty -- you forgot to update this.. I think this is right now.. */
    if (scan->residency & MINISTER && scan->location)
    {
      {
	count++;
	*stack = ' ';
	stack++;
	sprintf(stack, "%-20s ", scan->name);
	stack = strchr(stack, 0);
	sprintf(stack, "   ");
	stack = strchr(stack, 0);
	if (count % 3 == 0)
	  *stack++ = '\n';
      }
    }
  }
  if (count % 3 != 0)
    *stack++ = '\n';
  if (count > 1)
    sprintf(middle, "There are %d Ministers connected", count);
  else if (count == 1)
    sprintf(middle, "There is only 1 Minister connected");
  pstack_mid(middle);

  *stack++ = 0;

  if (count)
    tell_player(p, oldstack);
  else
    tell_player(p, " There are no ministers on at the moment.\n");
  stack = oldstack;
}

/* list spods =P */

void list_spods(player * p, char *str)
{
  int count = 0;
  char *oldstack, *prestack;
  char middle[80];
  player *scan;

  oldstack = stack;
  pstack_mid("Spods on");

  for (scan = flatlist_start; scan; scan = scan->flat_next)
  {
    prestack = stack;
    if (scan->residency & SPOD && scan->location)
    {
      if (scan->residency & SPOD)
      {
	count++;
	*stack = ' ';
	stack++;
	sprintf(stack, "%-20s ", scan->name);
	stack = strchr(stack, 0);
	if (strlen(scan->spod_class) > 0)
	  sprintf(stack, " %-40.40s",
		  scan->spod_class);
	else
	  sprintf(stack, " %-40.40s", "");
	stack = strchr(stack, 0);
	if (scan->misc_flags & NO_SPOD_CHANNEL)
	  sprintf(stack, "off chan\n");
	else
	  sprintf(stack, "on chan\n");
	stack = strchr(stack, 0);
      }
    }
  }
  if (count > 1)
    sprintf(middle, "There are %d spods connected", count);
  else if (count == 1)
    sprintf(middle, "There is only 1 spod connected");

  pstack_mid(middle);
  *stack++ = 0;

  if (count)
    tell_player(p, oldstack);
  else
    tell_player(p, " There are no spods on at the moment.\n");
  stack = oldstack;
}

/* obviously this command has the potential for abuse,
   but like ALL staff stuff here, its not the 
   command that is inherantly bad, but how its used
   that makes it bad... there are of course redeeming
   qualities to it, blanking something there isnt access
   to via the code, helping a newbie with a command they
   just cant seem to grasp, etcetc..........
   like all staff commands, it relies soley on the
   judgement and conscience of the person who can acceess it
   ~phypor
 */
void spank(player * p, char *str)
{
  char *marked;
  player *p2;
  player *oc = current_player;
  int os = sys_flags, oct = command_type;

  if (!(config_flags & cfFORCEABLE))
  {
    tell_player(p, " This command is unimplimented.\n");
    return;
  }

  marked = next_space(str);
  if (!*marked)
  {
    tell_player(p, " Format: force <player> <command>\n");
    return;
  }
  *marked++ = 0;
  p2 = find_player_global(str);
  if (!p2)
    return;

  if (p2->misc_flags & IN_EDITOR ||
      p2->mode & (MAILEDIT | ROOMEDIT | NEWSEDIT))
  {
    TELLPLAYER(p, " It appears '%s' is editing something....\n"
	       " Have to wait for them to finish.\n", p2->name);
    return;
  }

  if (!check_privs(p->residency, p2->residency))
  {
    tell_player(p, " Hey!... don't think so...\n");
    TELLPLAYER(p2, " -=*> %s just tried to force you to '%s'\n",
	       p->name, marked);
    return;
  }
  AW_BUT(p, " -=*> %s forces %s to do '%s'\n", p->name, p2->name, marked);
  TELLPLAYER(p, " You force %s to do '%s'\n", p2->name, marked);
  strncpy(p2->ibuffer, marked, IBUFFER_LENGTH - 1);
  current_player = p2;

  if (!match_social(p2, marked))
    match_commands(p2, marked);

  /* restore all the stuffs */
  current_player = oc;
  sys_flags = os;
  command_type = oct;
  memset(p2->ibuffer, 0, IBUFFER_LENGTH);
}

void see_player_whois(player * p, char *str)
{
  char *oldstack;
  file h;

  oldstack = stack;
  if (!*str)
  {
    tell_player(p, " Format: whois <player>\n");
    return;
  }
  lower_case(str);
  if (*str == '.')
  {
    tell_player(p, " Uh-uh, cant do that ...\n");
    return;
  }
  sprintf(stack, "files/whois/%s.who", str);
  stack = end_string(stack);
  h = load_file_verbose(oldstack, 0);
  if (h.where)
  {
    if (*(h.where))
    {
      if (p->custom_flags & NO_PAGER)
	tell_player(p, h.where);
      else
	pager(p, h.where);
    }
    else
    {
      tell_player(p, " Couldn't find a whois for that player ...\n");
    }
    FREE(h.where);
  }
  stack = oldstack;
}


void wall_to_supers(player * p, char *str)
{
  char *oldstack;

  oldstack = stack;
  command_type = 0;

  if (!*str)
  {
    tell_player(p, " Format: suwall <message>\n");
    return;
  }
  if (p->flags & BLOCK_SU)
  {
    tell_player(p, " You can't do su walls when you're ignoring them.\n");
    return;
  }
  sprintf(stack, " -=*> %s\n", str);
  stack = end_string(stack);
  su_wall(oldstack);
  stack = oldstack;
}

/* your lsp is so cool I stole it for lsa */
void list_admins(player * p, char *str)
{
  int count = 0;
  char *oldstack, *prestack;
  player *scan;

  oldstack = stack;

  pstack_mid("Admins on");

  for (scan = flatlist_start; scan; scan = scan->flat_next)
  {
    prestack = stack;
    if (scan->residency & (LOWER_ADMIN | ADMIN | ADC))
    {
      count++;
      if ((count == 1) || (count == 2))
	sprintf(stack, " Name : ");
      else
	sprintf(stack, "        ");
      stack = strchr(stack, 0);
      /*         if (count%2 ==1)
         sprintf(stack, "%-15s ", scan->name);
         else */
      sprintf(stack, "%-15s ", scan->name);
      stack = strchr(stack, 0);
      if ((count == 1) || (count == 2))
	sprintf(stack, " Idle : ");
      else
	sprintf(stack, "        ");
      stack = strchr(stack, 0);
      if (scan->idle % 60 < 10)
	sprintf(stack, "%d:0%d  ", scan->idle / 60, scan->idle % 60);
      else
	sprintf(stack, "%d:%2d  ", scan->idle / 60, scan->idle % 60);
      stack = strchr(stack, 0);
      if (count % 2 == 0)
	*stack++ = '\n';
      else
	*stack++ = '|';
    }
  }
  if (count % 2 == 1)
    *stack++ = '\n';
  if (count >= 1)
    sprintf(stack, LINE);
  stack = end_string(stack);
  if (count)
    tell_player(p, oldstack);
  else
    tell_player(p, " *wibble* no admins on?!\n");
  stack = oldstack;
}

void reset_total_idle(player * p, char *str)
{
  player *p2, dummy;
  char *oldstack, *temp;

  oldstack = stack;
  temp = next_space(str);
  *temp++ = 0;

  if (!*str)
  {
    tell_player(p, " Format: reset_idle <player>\n");
    return;
  }
  if (p->flags & BLOCK_SU)
  {
    tell_player(p, " Go back on duty first beavis.\n");
    return;
  }
  /* p2 = find_player_absolute_quiet(str); */
  p2 = find_player_global(str);
  if (p2)
  {
    if ((p2->residency == NON_RESIDENT))
    {
      tell_player(p, " That player is non-resident!\n");
      stack = oldstack;
      return;
    }
    if ((p2->residency == BANISHD || p2->residency == SYSTEM_ROOM))
    {
      tell_player(p, " That is a banished NAME, or System Room.\n");
      stack = oldstack;
      return;
    }
    sprintf(oldstack, "\n -=*> %s has reset your idle time...\n",
	    p->name);
    stack = end_string(oldstack);
    tell_player(p2, oldstack);
    stack = oldstack;
    p2->total_idle_time = 0;
    sprintf(stack, "%s reset the idle time on %s.", p->name, p2->name);
    stack = end_string(stack);
    log("edtime", oldstack);
    save_player(p2);
    stack = oldstack;
    sprintf(stack, " -=*> %s resets %s's idle time.\n", p->name,
	    p2->name);
    stack = end_string(stack);
    au_wall(oldstack);
    stack = oldstack;
    return;
  }
  /* else, load the dummy */
  strcpy(dummy.lower_name, str);
  dummy.fd = p->fd;
  if (load_player(&dummy))
  {
    if ((dummy.residency == BANISHD || dummy.residency == SYSTEM_ROOM))
    {
      tell_player(p, " That is a banished NAME, or System Room.\n");
      stack = oldstack;
      return;
    }
    dummy.total_idle_time = 0;
    sprintf(stack, "%s reset the idle time on %s.", p->name, dummy.name);
    stack = end_string(stack);
    log("edtime", oldstack);
    dummy.location = (room *) - 1;
    save_player(&dummy);
    stack = oldstack;
    sprintf(stack, " -=*> %s resets %s's idle time.\n", p->name,
	    dummy.name);
    stack = end_string(stack);
    au_wall(oldstack);
    stack = oldstack;
    return;
  }
}

/* use this to reset the disclaimer bit for email residency */
void disclaim(player * p, char *str)
{

  player *p2;

  if (!*str)
  {
    tell_player(p, " Format: disclaim <player>\n");
    return;
  }
  p2 = find_player_global(str);
  if (!p2)
    return;

  p2->system_flags &= ~AGREED_DISCLAIMER;
}

void make_git(player * p, char *str)
{

  char *gitstr;
  player *git;

  if (!*str)
  {
    tell_player(p, " Format: git <player> <gitstring>\n");
    return;
  }
  gitstr = next_space(str);
  if (*gitstr)
    *gitstr++ = 0;
  else
  {
    tell_player(p, " Format: git <player> <gitstring>\n");
    return;
  }
  git = find_player_global(str);
  if (!git)
    return;

  if (!(git->residency))
  {
    tell_player(p, " You can't git non-residents!\n");
    return;
  }

  /* Missing from PG and source of much amusement on UberWorld */

  if (git == p)
  {
    tell_player(p, " You can't git yourself!\n");
    return;
  }
  if (!check_privs(p->residency, git->residency))
  {
    tell_player(p, " No way, Jose!\n");
    return;
  }
  /* else, dooooooooooooooooooooooo eeeeeeeeeeeeeeeeeeeet */

  strncpy(git->git_string, gitstr, MAX_DESC - 3);
  strncpy(git->git_by, p->name, MAX_NAME - 3);
  git->residency |= GIT;
  tell_player(p, " All done =) \n");

  SUWALL(" -=*> %s declares %s to be a git -- %s\n",
	 p->name, git->name, gitstr);
  LOGF("git", "%s gits %s - %s", p->name, git->name, gitstr);
}

void clear_git(player * p, char *str)
{

  player *p2;

  if (!*str)
  {
    tell_player(p, " Format: ungit <player>\n");
    return;
  }
  p2 = find_player_global(str);

  if (!p2)
    return;

  if (!check_privs(p->residency, p2->residency))
  {
    tell_player(p, " No way, Jose! \n");
    return;
  }
  p2->git_string[0] = 0;
  p2->git_by[0] = 0;
  p2->residency &= ~GIT;

  tell_player(p, " All done.\n");
  SUWALL(" -=*> %s ungits %s.\n", p->name, p2->name);
  LOGF("git", "%s ungits %s.", p->name, p2->name);
}


/* A "lesser warn" -- idea from kw, but code is spoon of old warn command */

void lesser_warn(player * p, char *str)
{
  char *oldstack, *msg, *pstring, *final;
  player **list, **step;
  int i, n, r = 0;


  oldstack = stack;
  align(stack);
  command_type = PERSONAL | SEE_ERROR | WARNING;

  if (p->tag_flags & BLOCK_TELLS)
  {
    tell_player(p, " You are currently BLOCKING TELLS. It might be an idea to"
		" unblock so they can reply, eh?\n");
  }
  msg = next_space(str);
  if (*msg)
    *msg++ = 0;
  if (!*msg)
  {
    tell_player(p, " Format: ask <player(s)> <message>\n");
    stack = oldstack;
    return;
  }
  if (p->flags & BLOCK_SU)
  {
    tell_player(p, " You cannot ask when off_duty\n");
    stack = oldstack;
    return;
  }
  /* no warns to groups */
  if (!strcasecmp(str, "everyone") || !strcasecmp(str, "friends")
      || !strcasecmp(str, "supers") || !strcasecmp(str, "sus")
      || strstr(str, "everyone"))
  {
    tell_player(p, " Now that would be a bit silly wouldn't it?\n");
    stack = oldstack;
    return;
  }
  if (!strcasecmp(str, "room"))
    r = 1;
  /* should you require warning, the consequences are somewhat (less) severe */
  if (!strcasecmp(str, "me"))
  {
    tell_player(p, " Ummmmmmmmmmmmmmmmmmmmmm no. \n");
    stack = oldstack;
    return;
  }
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
  final = stack;
  if (r)
    sprintf(stack, " %s cautions everyone in the room '%s'\n", p->name, msg);
  else
    sprintf(stack, " %s cautions you '%s'\n", p->name, msg);
  stack = end_string(stack);
  for (step = list, i = 0; i < n; i++, step++)
  {
    if (*step != p)
    {
      command_type |= HIGHLIGHT;
      tell_player(*step, final);
      (*step)->warn_count++;
      p->num_warned++;
      command_type &= ~HIGHLIGHT;
    }
  }
  stack = final;

  pstring = tag_string(p, list, n);
  final = stack;
  sprintf(stack, " -=*> %s cautions %s '%s'", p->name, pstring, msg);
  stack = end_string(stack);
  LOGF("ask", "%s cautions %s with `%s`", p->name, pstring, msg);
  strcat(final, "\n");
  stack = end_string(final);
  command_type = 0;
  su_wall(final);

  cleanup_tag(list, n);
  stack = oldstack;
}


void edfirst(player * p, char *str)
{
  player *p2;
  char *oldstack;
  int temp_time;
  char *temp;

  oldstack = stack;
  temp = next_space(str);
  *temp++ = 0;

  if (!*str || !*temp)
  {
    tell_player(p, " Format: edfirst <player> [-] time (days)>\n");
    return;
  }
  if (p->flags & BLOCK_SU)
  {
    tell_player(p, " Go back on duty first beavis.\n");
    return;
  }
  /* p2 = find_player_absolute_quiet(str); */
  p2 = find_player_global(str);
  if ((!p2))
  {
    tell_player(p, " Only when the player is logged on, chief!\n");
    stack = oldstack;
    return;
  }
  if ((p2->residency == NON_RESIDENT))
  {
    tell_player(p, " That player is non-resident!\n");
    stack = oldstack;
    return;
  }
  if ((p2->residency == BANISHD || p2->residency == SYSTEM_ROOM))
  {
    tell_player(p, " That is a banished NAME, or System Room.\n");
    stack = oldstack;
    return;
  }
  sprintf(oldstack, "\n -=*> %s has changed your date of first login...\n",
	  p->name);
  stack = end_string(oldstack);
  tell_player(p2, oldstack);
  stack = oldstack;
  temp_time = atoi(temp);
  /* convert to hours */
  temp_time *= 3600;
  /* and to days */
  temp_time *= 24;
  p2->first_login_date += temp_time;
  sprintf(stack, "%s changed the initial login date of %s by %s days.",
	  p->name, p2->name, temp);
  stack = end_string(stack);
  log("edtime", oldstack);
  save_player(p2);
  stack = oldstack;
  sprintf(stack, " -=*> %s changes the initial login date of %s.\n", p->name,
	  p2->name);
  stack = end_string(stack);
  au_wall(oldstack);
  stack = oldstack;
}



void edidle(player * p, char *str)
{
  player *p2;
  char *oldstack;
  int temp_time;
  char *temp;

  oldstack = stack;
  temp = next_space(str);
  *temp++ = 0;

  if (!*str || !*temp)
  {
    tell_player(p, " Format: edidle <player> [-] time (hours)>\n");
    return;
  }
  if (p->flags & BLOCK_SU)
  {
    tell_player(p, " Go back on duty first beavis.\n");
    return;
  }
  /* p2 = find_player_absolute_quiet(str); */
  p2 = find_player_global(str);
  if ((!p2))
  {
    tell_player(p, " Only when the player is logged on, chief!\n");
    stack = oldstack;
    return;
  }
  if ((p2->residency == NON_RESIDENT))
  {
    tell_player(p, " That player is non-resident!\n");
    stack = oldstack;
    return;
  }
  if ((p2->residency == BANISHD || p2->residency == SYSTEM_ROOM))
  {
    tell_player(p, " That is a banished NAME, or System Room.\n");
    stack = oldstack;
    return;
  }
  sprintf(oldstack, "\n -=*> %s has changed your total login time...\n",
	  p->name);
  stack = end_string(oldstack);
  tell_player(p2, oldstack);
  stack = oldstack;
  temp_time = atoi(temp);
  /* convert to hours */
  temp_time *= 3600;
  p2->total_idle_time += temp_time;
  sprintf(stack, "%s changed the idle time of %s by %s hours.",
	  p->name, p2->name, temp);
  stack = end_string(stack);
  log("edtime", oldstack);
  save_player(p2);
  stack = oldstack;
  sprintf(stack, " -=*> %s changes the truespod login time of %s.\n", p->name,
	  p2->name);
  stack = end_string(stack);
  au_wall(oldstack);
  stack = oldstack;
}


void marry_edit(player * p, char *str)
{

  char *newmn;
  player *p2;

  if (!*str)
  {
    tell_player(p, " Format: medit <player> <new spouse>\n");
    return;
  }
  newmn = next_space(str);
  if (*newmn)
    *newmn++ = 0;
  else
  {
    tell_player(p, " Format: medit <player> <new spouse>\n");
    return;
  }
  p2 = find_player_global(str);
  if (!p2)
    return;

  /* Check the name is a valid one */
  if (!check_legal_entry(p, newmn, 1))
    return;

  /* Set the capitalisation correctly */
  sprintf(p2->married_to, check_legal_entry(p, newmn, 0));

  p2->system_flags |= MARRIED;
  tell_player(p, " All done =) \n");
  TELLPLAYER(p2, " -=*> %s has married you to %s.\n", p->name, p2->married_to);
  SW_BUT(p, " -=*> %s medits %s's spouses name to %s\n", p->name, p2->name, p2->married_to);
  LOGF("marry", "%s medits %s's spouses name to %s", p->name, p2->name, p2->married_to);
}

void list_builders(player * p, char *str)
{
  int count = 0;
  char middle[80];
  char *oldstack, *prestack;
  player *scan;

  oldstack = stack;
  pstack_mid("Builders on");

  for (scan = flatlist_start; scan; scan = scan->flat_next)
  {
    prestack = stack;
    /* hey asty -- you forgot to update this.. I think this is right now.. */
    if (scan->residency & BUILDER && scan->location)
    {
      {
	count++;
	*stack = ' ';
	stack++;
	sprintf(stack, "%-20s ", scan->name);
	stack = strchr(stack, 0);
	sprintf(stack, "   ");
	stack = strchr(stack, 0);
	if (count % 3 == 0)
	  *stack++ = '\n';
      }
    }
  }
  if (count % 3 != 0)
    *stack++ = '\n';
  if (count > 1)
    sprintf(middle, "There are %d Builders connected", count);
  else if (count == 1)
    sprintf(middle, "There is only 1 Builder connected");
  pstack_mid(middle);
  *stack++ = 0;

  if (count)
    tell_player(p, oldstack);
  else
    tell_player(p, " There are no builders on at the moment.\n");
  stack = oldstack;
}


#ifdef REDHAT5
int valid_emergency(struct dirent *d)
#else /* REDHAT5 */
int valid_emergency(const struct dirent *d)
#endif				/* !REDHAT5 */
{
  char *dotter;

  if (!(dotter = strstr(d->d_name, ".emergency")) || strlen(dotter) != 10)
    return 0;
  return 1;
}


#ifdef REDHAT5
int valid_script(struct dirent *d)
#else /* REDHAT5 */
int valid_script(const struct dirent *d)
#endif				/* !REDHAT5 */
{
  char *dotter;

  if (!(dotter = strstr(d->d_name, ".script")) || strlen(dotter) != 7)
    return 0;
  return 1;
}

void vscript(player * p, char *str)
{
  struct dirent **de;
  struct stat sbuf;
  char *oldstack = stack, path[160], *fr;
  int dc = 0, i, vemerg = 0;
  file lf;

  fr = first_char(p);
  if (strstr(fr, "vemerg"))
    vemerg++;

  if (!*str || *str == '?' || !strcasecmp(str, "list"))
  {
    if (vemerg)
      dc = scandir("logs/emergency", &de, valid_emergency, alphasort);
    else
      dc = scandir("logs/scripts", &de, valid_script, alphasort);

    if (!dc)
    {
      tell_player(p, " There are currently no files in there ...\n");
      if (de)
	free(de);
      return;
    }
    if (vemerg)
      pstack_mid("Emergency Scripts");
    else
      pstack_mid("Extended Scripting Files");
    stack += sprintf(stack, "\n");

    for (i = 0; i < dc; i++)
    {
      strcpy(stack, de[i]->d_name);
      stack = strchr(stack, 0);
      while (*stack != '.')
	*stack-- = 0;
      *stack++ = ',';
      *stack++ = ' ';

    }
    for (i = 0; i < dc; i++)
      FREE(de[i]);
    if (de)
      FREE(de);

    while (*stack != ',')
      *stack-- = '\0';
    stack += sprintf(stack, ".\n\n");
    if (dc == 1)
      sprintf(path, "There is one file here to view");
    else
      sprintf(path, "There are %d files here to view", dc);
    pstack_mid(path);
    *stack++ = '\0';
    tell_player(p, oldstack);
    stack = oldstack;
    return;
  }

  if (vemerg)
    sprintf(path, "logs/emergency/%s.emergency", str);
  else
    sprintf(path, "logs/scripts/%s.script", str);

  if (stat(path, &sbuf) < 0)
  {
    if (vemerg)
      TELLPLAYER(p, " No emergency file '%s' found ...\n", str);
    else
      TELLPLAYER(p, " No extended script file '%s' found ...\n", str);
    return;
  }
  lf = load_file(path);
  if (!lf.where)
  {
    tell_player(p, " Ouch, dunno whut happened ...\n");
    return;
  }
  if (!*(lf.where))
  {
    tell_player(p, " The file is plum empty.\n");
    FREE(lf.where);
    return;
  }

  if (vemerg)
    sprintf(path, "Emergency script for %s", str);
  else
    sprintf(path, "Extended script file '%s'", str);

  pstack_mid(path);
  stack += sprintf(stack, "%s\n", lf.where);
  sprintf(path, "End Of Script");
  pstack_mid(path);
  *stack++ = 0;
  pager(p, oldstack);
  FREE(lf.where);
  stack = oldstack;
}

void ammend_to_log(player * p, char *str)
{
  char *oldstack = stack, *arg = next_space(str);
  struct stat sbuf;

  if (!*arg)
  {
    tell_player(p, " Format : ammend <log> <your comments>\n");
    return;
  }
  *arg++ = 0;
  if (*str == '.' || strchr(str, '/'))
  {
    tell_player(p, " You just caint do dat.\n");
    return;
  }
  sprintf(stack, "logs/%s.log", str);
  stack = end_string(stack);
  if (stat(oldstack, &sbuf) < 0)
  {
    tell_player(p, " No such log for ammendment of any comments.\n");
    stack = oldstack;
    return;
  }
  stack = oldstack;
  LOGF(str, "Ammendment by %s : %s", p->name, arg);
  SW_BUT(p, " -=*> %s ammends to the '%s' log ...\n"
	 " -=*> %s\n", p->name, str, arg);
  TELLPLAYER(p, " Ammendment to %s log made.\n", str);
}
