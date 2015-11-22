/*
 * Playground+ - socials.c
 * kRad Soshuls, dynamic, created onthefly special krad d00d (c) phypor 1998
 * ---------------------------------------------------------------------------
 *
 * You may extract it and use it in a running talker
 * if you place credit for it either in help credits
 *     kRad Soshuls by phypor
 * or in the version command output
 *    -=> kRad Soshuls v1.1 (by phypor)
 *
 * 'This code' refers to the actual src, the look and feel
 * of the output, and any derivied works from it
 *
 * This code is licenced for distribution in PG+ ONLY
 * You may not distribute this code in any form other than
 * within the code release PG+, you may only distribute
 * it in a successor of PG+ with express consent in writing
 * from phypor (j. bradley christian) <phypor@benland.muc.edu>
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "include/config.h"
#include "include/player.h"
#include "include/proto.h"
#include "include/socials.h"

#define MAX_POSSIBLE_SOCIAL_EXPANSION	1024
#define MAX_SOCIAL_MEMBER_LENGTH	320


   /* interns */
int add_social_main_list(generic_social *);
int unlink_social(generic_social *);
int social_room(player *, char *, int, char *);
void do_any_social(player *, generic_social *, char *);
void do_simple_social(player *, simple_social *, char *);
void do_compound_social(player *, compound_social *, char *);
void do_private_social(player *, private_social *, char *);
void show_commands_for_social(player *);


   /* varibles */
generic_social *social_top;
player *targ;

void malloc_error(player * p, char *str)
{
  if (p)
    tell_player(p, " Errs, sorry a lowlevel error just occured.\n");

  LOGF("error", "malloc error trying to %s ...", str);
}

int issocia(char c)
{
  if (isalnum(c))
    return 1;
  if (c == '_' || c == '-' || c == '~')
    return 1;
  return 0;
}

void free_social(generic_social * soc)
{
  if (soc)
  {
    if (soc->format)
      FREE(soc->format);
    if (soc->room_msg)
      FREE(soc->room_msg);
    if (soc->used_room_msg)
      FREE(soc->used_room_msg);
    if (soc->direct_msg)
      FREE(soc->direct_msg);
    if (soc->used_direct_msg)
      FREE(soc->used_direct_msg);
    FREE(soc);
  }
}

void destroy_all_socials(void)
{
  generic_social *scan = social_top, *current;

  while (scan)
  {
    current = scan->next;
    free_social(scan);
    scan = current;
  }

  social_top = (generic_social *) NULL;

}


   /*  io stuffs */
int sync_social(generic_social * soc)
{
  char *oldstack = stack;
  int fd, wrote;

  sprintf(stack, "files/socials/%s", soc->command);
  stack = end_string(stack);
#ifdef BSDISH
  fd = open(oldstack, (O_TRUNC | O_CREAT | O_WRONLY),
	    (S_IRUSR | S_IWUSR));
#else
  fd = open(oldstack, (O_TRUNC | O_CREAT | O_WRONLY | O_SYNC),
	    (S_IRUSR | S_IWUSR));
#endif /* BSDISH */
  if (fd < 0)
  {
    LOGF("error", "Failed to sync social %s to file %s, [%s].\n",
	 soc->command, oldstack, strerror(errno));
    stack = oldstack;
    return -1;
  }
  stack = oldstack;

  stack += sprintf(stack, "%s\n%d\n%d\n", soc->creator,
		   soc->date, soc->flags);

  if (soc->format)
    stack += sprintf(stack, "%s\n", soc->format);
  else
    stack += sprintf(stack, "\n");

  if (soc->room_msg)
    stack += sprintf(stack, "%s\n", soc->room_msg);
  else
    stack += sprintf(stack, "\n");

  if (soc->used_room_msg)
    stack += sprintf(stack, "%s\n", soc->used_room_msg);
  else
    stack += sprintf(stack, "\n");

  if (soc->direct_msg)
    stack += sprintf(stack, "%s\n", soc->direct_msg);
  else
    stack += sprintf(stack, "\n");

  if (soc->used_direct_msg)
    stack += sprintf(stack, "%s\n", soc->used_direct_msg);
  else
    stack += sprintf(stack, "\n");

  stack = end_string(stack);
  wrote = write(fd, oldstack, strlen(oldstack));
  stack = oldstack;
  close(fd);

  return wrote;
}




void sync_all_socials(void)
{
  generic_social *scan;
  int sunc = 0;

  for (scan = social_top; scan; scan = scan->next)
  {
    if (sync_social(scan) < 0)
    {
      if (current_player)
	TELLPLAYER(current_player, "Shat! failed to sync '%s'",
		   scan->command);
    }
    else
      sunc++;

  }
  if (current_player)
    TELLPLAYER(current_player, "Sunc %d socials.\n", sunc);
}

void sync_socials_command(player * p, char *str)
{
  tell_player(p, " Syncing kRad soshuls ... ");
  sync_all_socials();
  tell_player(p, " Done.\n");
}


int load_social(char *name)
{
  int w = 0;
  FILE *f;
  char *oldshack = stack, sname[MAX_NAME];
  generic_social *soc;


  strncpy(sname, name, MAX_NAME - 1);
  lower_case(sname);

  sprintf(stack, "files/socials/%s", sname);
  stack = end_string(stack);

  f = fopen(oldshack, "r");

  if (!f)
  {
    LOGF("error", "Failed to load social %s [%s]", oldshack,
	 strerror(errno));
    stack = oldshack;
    return -1;
  }
  soc = (generic_social *) MALLOC(sizeof(generic_social));
  if (!soc)
  {
    fclose(f);
    malloc_error(current_player, "load_social");
    stack = oldshack;
    return -1;
  }
  memset(soc, 0, sizeof(generic_social));
  strncpy(soc->command, name, MAX_NAME - 1);

  stack = oldshack;
  memset(stack, 0, MAX_SOCIAL_MEMBER_LENGTH + 1);
  while (fgets(stack, MAX_SOCIAL_MEMBER_LENGTH, f))
  {
    if (stack[strlen(stack) - 1] == '\n')
      stack[strlen(stack) - 1] = '\0';

    if (!*stack)
    {
      w++;
      continue;
    }

    switch (w++)
    {
      case 0:
	strncpy(soc->creator, stack, MAX_NAME - 1);
	break;
      case 1:
	soc->date = atoi(stack);
	break;
      case 2:
	soc->flags = atoi(stack);
	break;
      case 3:
	soc->format = (char *) MALLOC(strlen(stack) + 1);
	if (!soc->format)	/* fuggit, we should handle_error if we cant
				   get any malloc'ing done ... */
	{
	  free_social(soc);
	  fclose(f);
	  malloc_error(current_player, "load_social");
	}
	memset(soc->format, 0, strlen(stack) + 1);
	strcpy(soc->format, stack);
	break;
      case 4:
	soc->room_msg = (char *) MALLOC(strlen(stack) + 1);
	if (!soc->room_msg)
	{
	  free_social(soc);
	  fclose(f);
	  malloc_error(current_player, "load_social");
	}
	memset(soc->room_msg, 0, strlen(stack) + 1);
	strcpy(soc->room_msg, stack);
	break;
      case 5:
	soc->used_room_msg = (char *) MALLOC(strlen(stack) + 1);
	if (!soc->used_room_msg)
	{
	  free_social(soc);
	  fclose(f);
	  malloc_error(current_player, "load_social");
	}
	memset(soc->used_room_msg, 0, strlen(stack) + 1);
	strcpy(soc->used_room_msg, stack);
	break;
      case 6:
	soc->direct_msg = (char *) MALLOC(strlen(stack) + 1);
	if (!soc->direct_msg)
	{
	  free_social(soc);
	  fclose(f);
	  malloc_error(current_player, "load_social");
	}
	memset(soc->direct_msg, 0, strlen(stack) + 1);
	strcpy(soc->direct_msg, stack);
	break;
      case 7:
	soc->used_direct_msg = (char *) MALLOC(strlen(stack) + 1);
	if (!soc->used_direct_msg)
	{
	  free_social(soc);
	  fclose(f);
	  malloc_error(current_player, "load_social");
	}
	memset(soc->used_direct_msg, 0, strlen(stack) + 1);
	strcpy(soc->used_direct_msg, stack);
	break;
    }
    memset(stack, 0, MAX_SOCIAL_MEMBER_LENGTH + 1);
  }
  fclose(f);
  stack = oldshack;
  add_social_main_list(soc);
  return 0;

}

#ifdef REDHAT5
int valid_social(struct dirent *d)
#else /* REDHAT5 */
int valid_social(const struct dirent *d)
#endif
{
  if (d->d_name[0] == '.')
    return 0;
  return 1;
}


void init_socials(void)
{
  struct dirent **de;
  int dc = 0;

  dc = scandir("files/socials", &de, valid_social, alphasort);
  if (!dc)
  {
    if (de)
      free(de);
    return;
  }
  destroy_all_socials();

  for (dc--; dc >= 0; dc--)
  {
    load_social(de[dc]->d_name);
    FREE(de[dc]);
  }
  if (de)
    FREE(de);
}

void load_socials_command(player * p, char *str)
{
  tell_player(p, " kRad Soshuls loading ... ");
  init_socials();
  tell_player(p, " Done\n");
}





int list_socials(player * p)
{
  generic_social *scan;
  int i = 0, len = 2, num = 0;

  stack += sprintf(stack, "\n\n  ");

  for (i = 0; SimpleSocials[i].command[0]; i++)
  {
    stack += sprintf(stack, "%s ", SimpleSocials[i].command);
    num++;
  }

  for (i = 0; CompoundSocials[i].command[0]; i++)
  {
    stack += sprintf(stack, "%s ", CompoundSocials[i].command);
    num++;
  }

  for (i = 0; PrivateSocials[i].command[0]; i++)
  {
    stack += sprintf(stack, "%s ", PrivateSocials[i].command);
    num++;
  }

  stack += sprintf(stack, "\n  ");

  for (scan = social_top; scan; scan = scan->next)
  {
    if (len + strlen(scan->command) > 70)
    {
      stack += sprintf(stack, "\n  ");
      len = 2;
    }
    stack += sprintf(stack, "%s ", scan->command);
    num++;
    len += (strlen(scan->command) + 1);
  }
  stack += sprintf(stack, "\n");
  return num;
}

/* uses the stack, set to oldstack before calling
   reset after used
 */
char *expand_social(player * p, char *str, char *arg)
{
  char *ret = stack;
  int siz = MAX_POSSIBLE_SOCIAL_EXPANSION;

  if (!str || !*str)
  {
    return "-- broken --";
  }
  strcpy(ret, str);

  strncpy(ret, replace(ret, "%hisher", gstring_possessive(p)), siz);
  strncpy(ret, replace(ret, "%himher", get_gender_string(p)), siz);
  strncpy(ret, replace(ret, "%heshe", gstring(p)), siz);
  strncpy(ret, replace(ret, "%name", p->name), siz);
  strncpy(ret, single_replace(ret, "%fullname", full_name(p)), siz);

  strncpy(ret, replace(ret, "%t_hisher", gstring_possessive(targ)), siz);
  strncpy(ret, replace(ret, "%t_himher", get_gender_string(targ)), siz);
  strncpy(ret, replace(ret, "%t_heshe", gstring(targ)), siz);
  strncpy(ret, replace(ret, "%t_name", targ->name), siz);
  strncpy(ret, single_replace(ret, "%t_fullname", full_name(targ)), siz);

  strncpy(ret, single_replace(ret, "%str", arg), siz);

  stack = end_string(stack);
  return ret;
}


int social_room(player * p, char *str, int try_room, char *msg)
{

  player *p2;
  char *oldstack = stack, *that;

  p2 = find_player_global(str);
  if (!p2)
    return 0;

  if (p == p2)
  {
    TELLPLAYER(p, " You spoon -- thats YOU!\n");
    return 0;
  }

  if (try_room && p2->location && p->location
      && p2->location == p->location)	/* same room */
  {
    that = stack;
    sprintf(stack, "%s | %s", msg, p2->name);
    stack = end_string(stack);

    extract_pipe_local(that);
    if (sys_flags & FAILED_COMMAND)
    {
      sys_flags &= ~FAILED_COMMAND;
      return 0;
    }
    command_type |= ROOM;
    send_to_room(p, that, 0, 1);
    cleanup_tag(pipe_list, pipe_matches);
    sys_flags &= ~PIPE;
  }
  else
  {
    sprintf(stack, "%s %s", p2->name, msg);
    stack = end_string(stack);
    remote_cmd(p, oldstack, 0);
  }
  stack = oldstack;
  return 1;
}



int match_social(player * p, char *str)
{
  int i = 0, old_sys = sys_flags, old_ct = command_type;
  char cmd[80], *strptr;
  generic_social *scan;

  if (!*str || social_index < 0)
    return 0;

  memset(cmd, 0, 80);
  for (i = 0, strptr = str; (*strptr && *strptr != ' ' && i < 78);
       cmd[i++] = *strptr++);

  if (*strptr)
    strptr++;

  targ = find_player_global_quiet(strptr);
  if (!targ)
    targ = p;

  for (scan = social_top; scan; scan = scan->next)
  {
    if (!strcasecmp(cmd, scan->command))
    {
      if (p->social_index)
      {
	tell_player(p, " You don't want to be overly social do you?\n");
	return 1;
      }
      do_any_social(p, scan, strptr);
      sys_flags = old_sys;
      command_type = old_ct;
      return 1;
    }
  }

  for (i = 0; SimpleSocials[i].command[0]; i++)
  {
    if (!strcasecmp(cmd, SimpleSocials[i].command))
    {
      if (p->social_index)
      {
	tell_player(p, " You don't want to be overly social do you?\n");
	return 1;
      }
      do_simple_social(p, &SimpleSocials[i], strptr);
      sys_flags = old_sys;
      command_type = old_ct;
      p->social_index = social_index;
      return 1;
    }
  }

  for (i = 0; CompoundSocials[i].command[0]; i++)
  {
    if (!strcasecmp(cmd, CompoundSocials[i].command))
    {
      if (p->social_index)
      {
	tell_player(p, " You don't want to be overly social do you?\n");
	return 1;
      }
      do_compound_social(p, &CompoundSocials[i], strptr);
      sys_flags = old_sys;
      command_type = old_ct;
      p->social_index = social_index;
      return 1;
    }
  }

  for (i = 0; PrivateSocials[i].command[0]; i++)
  {
    if (!strcasecmp(cmd, PrivateSocials[i].command))
    {
      if (p->social_index)
      {
	tell_player(p, " You don't want to be overly social do you?\n");
	return 1;
      }
      do_private_social(p, &PrivateSocials[i], strptr);
      sys_flags = old_sys;
      command_type = old_ct;
      p->social_index = social_index;
      return 1;
    }
  }
  return 0;
}

/* it may appear odd why this is done in seperate peices..
   but its not relaly....the expansion of random args is done
   first so that the expansions will match up throughout the
   social....for instance...if you have 
   blahs at you {erkingly|loudly|happily}
   in the room_msg and 
   You blah at %t_name {erkingly|loudly|happily}
   in the used_room_msg
   we want the random phrase thats used to be the same
   in both of them...
 */

void do_any_social(player * p, generic_social * soc, char *str)
{
  char *oldstack = stack;
  char *msg, *membuf;
  char *xpns[6], *ptr = "", *cntr;
  int i, r, rn;

  rn = rand();
  membuf = stack + (20 * 1024);	/* to expand the random args, we use 
				   a chunk of memory 20k into the stack,
				   as other stuff in this function or called
				   by it requires the stack as well
				 */
  for (i = 0; i < 5; i++)
  {
    switch (i)
    {
      case 0:
	ptr = soc->format;
	break;
      case 1:
	ptr = soc->room_msg;
	break;
      case 2:
	ptr = soc->used_room_msg;
	break;
      case 3:
	ptr = soc->direct_msg;
	break;
      case 4:
	ptr = soc->used_direct_msg;
	break;
    }
    xpns[i] = membuf;
    if (!ptr || !*ptr)
    {
      *membuf++ = 0;
      continue;
    }
    if (!(cntr = strchr(ptr, '{')) || !strchr(cntr, '}'))
    {
      xpns[i] = ptr;
      continue;
    }

    while (*ptr)
    {
      if (*ptr != '{')
      {
	*membuf++ = *ptr++;
	continue;
      }
      r = 1;
      for (cntr = ptr + 1; (*cntr && *cntr != '}'); cntr++)
	if (*cntr == '|')
	  r++;

      r = rn % r;
      cntr = ptr + 1;
      while (r && *cntr)
      {
	if (*cntr == '|')
	  r--;
	cntr++;
      }
      while (*cntr && *cntr != '|' && *cntr != '}')
	*membuf++ = *cntr++;

      if (*cntr && *cntr == '|')
	while (*cntr && *cntr != '}')
	  cntr++;

      ptr = cntr + 1;
    }
    *membuf++ = 0;
  }

  if ((soc->flags & stSIMPLE && soc->format && !*str) ||
      (soc->flags & stPRIVATE && !*str))
  {
    msg = expand_social(p, xpns[0], str);
    TELLPLAYER(p, " %s\n", msg);
    stack = oldstack;
    return;
  }

  if (soc->flags & stSIMPLE || (soc->flags & stCOMPLEX && !*str))
  {
    msg = expand_social(p, xpns[1], str);
    command_type |= ROOM;
    send_to_room(p, msg, 0, 1);
    stack = oldstack;
    command_type &= ~ROOM;

    msg = expand_social(p, xpns[2], str);
    TELLPLAYER(p, " %s\n", msg);
    stack = oldstack;
    p->social_index = social_index;
    return;
  }
  if (soc->flags & stCOMPLEX)	/* complex and *str */
  {
    msg = expand_social(p, xpns[3], str);
    if (social_room(p, str, 1, msg))
    {
      stack = oldstack;
      msg = expand_social(p, xpns[4], str);
      TELLPLAYER(p, " %s\n", msg);
    }
    stack = oldstack;
    p->social_index = social_index;
    return;
  }
  if (*str)
  {
    msg = expand_social(p, xpns[3], str);
    if (social_room(p, str, 0, msg))
    {
      stack = oldstack;
      msg = expand_social(p, xpns[4], str);
      TELLPLAYER(p, " %s\n", msg);
    }
    stack = oldstack;
    p->social_index = social_index;
    return;
  }
}
void do_simple_social(player * p, simple_social * soc, char *str)
{
  char *oldstack = stack;
  char *msg = expand_social(p, soc->outmsg, str);

  send_to_room(p, msg, 0, 1);

  tell_player(p, soc->workmsg);

  stack = oldstack;
}

void do_compound_social(player * p, compound_social * soc, char *str)
{
  if (!*str)
  {
    send_to_room(p, soc->nostr_outmsg, 0, 1);
    tell_player(p, soc->nostr_workmsg);
    return;
  }
  if (social_room(p, str, 1, soc->str_outmsg))
  {
    if (strstr(soc->str_workmsg, "%s"))
      TELLPLAYER(p, soc->str_workmsg, str);
    else
      tell_player(p, soc->str_workmsg);
  }
}

void do_private_social(player * p, private_social * soc, char *str)
{
  if (!soc)
  {
    tell_player(p, " How you get here?\n");
    LOGF("error", "%s to do_private_social but no soc\n", p->name);
    return;
  }
  if (!*str)
  {
    tell_player(p, soc->format);
    return;
  }
  if (social_room(p, str, 0, soc->outmsg))
  {
    if (strstr(soc->workmsg, "%s"))
      TELLPLAYER(p, soc->workmsg, str);
    else
      tell_player(p, soc->workmsg);
  }
}


/* new stuff */

int add_social_main_list(generic_social * soc)
{
  if (social_top)
    social_top->prev = soc;
  soc->next = social_top;
  social_top = soc;

  return 0;
}

int unlink_social(generic_social * soc)
{
  generic_social *scan;

  if (soc == social_top)
  {
    social_top = social_top->next;
    soc->next = (generic_social *) NULL;
    return 0;
  }

  for (scan = social_top; scan; scan = scan->next)
    if (scan == soc)
    {
      if (scan->next)
	scan->next->prev = scan->prev;
      if (scan->prev)
	scan->prev->next = scan->next;
      scan->next = (generic_social *) NULL;
      scan->prev = (generic_social *) NULL;
      return 0;
    }
  return -1;
}

int get_social_type(char *str)
{
  int i;

  for (i = 0; SocialTypes[i].where[0]; i++)
    if (!strcasecmp(str, SocialTypes[i].where))
      return SocialTypes[i].length;
  return -1;
}
char *soc_type_str(generic_social * soc)
{
  if (soc->flags & stSIMPLE)
    return "simple";
  else if (soc->flags & stCOMPLEX)
    return "complex";
  return "private";
}


generic_social *find_social(char *str)
{
  generic_social *scan;

  if (!*str || !social_top)
    return (generic_social *) NULL;

  for (scan = social_top; scan; scan = scan->next)
    if (!strcasecmp(scan->command, str))
      return scan;

  return (generic_social *) NULL;
}

int is_command(char *str)
{
  struct command *scan = coms[0];

  if (isalpha(*str))
    scan = coms[((int) (tolower(*str)) - (int) 'a' + 1)];

  while (scan && scan->text && scan->text[0])
  {
    if (!strcasecmp(str, scan->text))
      return 1;
    scan++;
  }
  return 0;
}


int get_attribute_type(generic_social * soc, char *str)
{

  if (!strcasecmp(str, "room_msg"))
  {
    if (soc->flags & stCOMPLEX || soc->flags & stSIMPLE)
      return atROOM_MSG;
    else
      return -1;
  }

  if (!strcasecmp(str, "used_room_msg"))
  {
    if (soc->flags & stCOMPLEX || soc->flags & stSIMPLE)
      return atUSED_ROOM_MSG;
    else
      return -1;
  }


  if (!strcasecmp(str, "direct_msg"))
  {
    if (soc->flags & stCOMPLEX || soc->flags & stPRIVATE)
      return atDIRECT_MSG;
    else
      return -1;
  }

  if (!strcasecmp(str, "used_direct_msg"))
  {
    if (soc->flags & stCOMPLEX || soc->flags & stPRIVATE)
      return atUSED_DIRECT_MSG;
    else
      return -1;
  }

  if (!strcasecmp(str, "format"))
  {
    if (soc->flags & stPRIVATE || soc->flags & stSIMPLE)
      return atFORMAT;
    else
      return -1;
  }

  if (!strcasecmp(str, "done"))
    return atDONE;

  if (!strcasecmp(str, "abort"))
    return atABORT;

  return -1;
}


void delete_social(player * p, char *str)
{
  int tool = 0;
  generic_social *soc;

  if (social_index < 0)
  {
    tell_player(p, " No social commands are implemented at the moment.\n");
    return;
  }

  if (!*str)
  {
    tell_player(p, " Format : delete_social <name of social>\n");
    return;
  }

  soc = find_social(str);

  if (!soc)
  {
    TELLPLAYER(p, " No such social '%s' to be found.\n", str);
    return;
  }

  if (strcasecmp(soc->creator, p->name))
  {
    if (!(p->residency & ADMIN))
    {
      TELLPLAYER(p, " The social '%s' isnt yours to delete.\n",
		 soc->command);
      return;
    }
    tool++;
  }

  /* inform now as itll be gone soon */
  if (tool)
  {
    SW_BUT(p, " -=*> %s deletes the social '%s' created by %s.\n",
	   p->name, soc->command, soc->creator);
    LOGF("social", "%s deletes %s, created by %s", p->name, soc->command,
	 soc->creator);
  }
  else
  {
    SW_BUT(p, " -=*> %s deletes the social '%s'\n",
	   p->name, soc->command);
    LOGF("social", "%s deletes %s", p->name, soc->command);
  }

  /* remove it from disk if its there */
  sprintf(stack, "files/socials/%s", soc->command);
  unlink(stack);

  /* get it out of the array */
  unlink_social(soc);

  /* kill it */
  free_social(soc);

  /* let the person know */
  tell_player(p, " Social deleted !\n");
}

void abort_social(player * p)
{
  generic_social *soc = p->social;

  if (!soc)
    return;

  tell_player(p, " Social creation aborted.\n");
  free_social(soc);
  p->social = (generic_social *) NULL;
}


void keep_social(player * p)
{
  generic_social *soc = p->social;

  if (!soc)
    return;

  if (soc->flags & stSIMPLE || soc->flags & stCOMPLEX)
  {
    if (!soc->room_msg)
    {
      tell_player(p, " The social is incomplete, it lacks the room message.\n");
      return;
    }
    if (!soc->used_room_msg)
    {
      tell_player(p, " The social is incomplete, it lacks the used room message.\n");
      return;
    }
  }
  if (soc->flags & stPRIVATE || soc->flags & stCOMPLEX)
  {
    if (!soc->direct_msg)
    {
      tell_player(p, " The social is incomplete, it lacks the direct message.\n");
      return;
    }
    if (!soc->used_direct_msg)
    {
      tell_player(p, " The social is incomplete, it lacks the used direct message.\n");
      return;
    }
  }
  if (soc->flags & stPRIVATE && !soc->format)
  {
    tell_player(p, " The social is incomplete, it lacks the format message.\n");
    return;
  }

  if (add_social_main_list(soc) < 0)
  {
    tell_player(p, " Failed to add social to the main list.\n");
    return;
  }

  soc->flags &= ~stINC;
  soc->date = time(0);

  TELLPLAYER(p, " Social '%s' created and ready to be used.\n", soc->command);
  LOGF("social", "%s completes %s", p->name, soc->command);
  SUWALL(" -=*> %s adds a new %s social '%s'\n", p->name,
	 soc_type_str(soc), soc->command);

  sync_social(soc);
  p->social = (generic_social *) NULL;
  init_socials();		/* re init so that we alwyas have em in order */
}

void start_new_social(player * p, char *str)
{
  player *scan;
  char *ptr, *type;
  int stype;

  if (social_index < 0)
  {
    tell_player(p, " No social commands are implemented at the moment.\n");
    return;
  }

  type = next_space(str);
  if (!*type)
  {
    tell_player(p, " Format : create_social <command for it> <type>\n");
    return;
  }
  *type++ = '\0';

  if (!isalpha(*str))
  {
    tell_player(p, " Socials must begin with a letter.\n");
    return;
  }

  for (ptr = str; (*ptr && *ptr != ' '); ptr++)
    if (!issocia(*ptr))
    {
      tell_player(p, " Socials may contain only letters, numbers and a few "
		  "different chars like _ ~ - for the command.\n");
      return;
    }

  if (strlen(str) < 3)
  {
    tell_player(p, " Social names must be at least 3 chars in length.\n");
    return;
  }

  if (p->social)
  {
    tell_player(p, " You may only be working on one social atta time.\n");
    return;
  }

  if (is_command(str))
  {
    tell_player(p, " Thats already a talker command, dood!\n");
    return;
  }

  for (scan = flatlist_start; scan; scan = scan->flat_next)
    if (scan->social && !strcasecmp(str, scan->social->command))
    {
      tell_player(p, " Ouch, someone is working on a social by that name atm.\n");
      return;
    }

  if (find_social(str))
  {
    tell_player(p, " That social already exists ...\n");
    return;
  }

  stype = get_social_type(type);
  if (stype == -1)
  {
    TELLPLAYER(p, " Unknown social type '%s' ...\n"
	       " (try simple, complex, or private)\n", type);
    return;
  }

  p->social = (generic_social *) MALLOC(sizeof(generic_social));
  if (!p->social)
  {
    tell_player(p, " Ouch lowlever error!\n");
    log("error", "failed to malloc in creation of social");
    return;
  }
  memset(p->social, 0, sizeof(generic_social));

  strncpy(p->social->command, str, MAX_NAME - 1);
  strncpy(p->social->creator, p->name, MAX_NAME - 1);
  p->social->flags = stype | stINC;

  TELLPLAYER(p, " New social creation begun for '%s' type %s.\n",
	     p->social->command, type);

  LOGF("social", "%s starts work on a new %s social called '%s'",
       p->name, type, p->social->command);

  show_commands_for_social(p);
}

void show_commands_for_social(player * p)
{
  if (!p->social)
  {
    tell_player(p, " But your not even creating a social.\n");
    return;
  }
  if (p->social->flags & stSIMPLE)
  {
    tell_player(p,
		" Simple social commands and meanings ...\n"
		"   setsoc room_msg <string>\n"
		"      the string that is shown to the room when a player uses the social\n"
		"   setsoc used_room_msg <string>\n"
	 "      the string a player sees themself when they do the social\n"
		"   setsoc format <string>\n"
		"     if set, the social will require an argument, this will show in absents\n");
  }
  else if (p->social->flags & stCOMPLEX)
  {
    tell_player(p,
		" Complex social commands and meanings ...\n"
		"   setsoc room_msg <string>\n"
		"      the string that is shown to room when social is used without argument\n"
		"   setsoc used_room_msg <string>\n"
      "      the string a player sees themself when the room_msg is used.\n"
		"   setsoc direct_msg <string>\n"
	     "      the string remoted or piped, if an argument is given.\n"
		"   setsoc used_direct_msg <string>\n"
		"      the string a player sees themself when the direct_msg is used.\n");
  }
  else
  {
    tell_player(p,
		" Private social commands and meanings ...\n"
		"   setsoc format <string>\n"
     "      the string shown if no argument is given when social is used.\n"
		"   setsoc direct_msg <string>\n"
		"      the string remoted or piped to the target player\n"
		"   setsoc used_direct_msg <string>\n"
       "      the string the player doing the social sees when its done\n");
  }

  tell_player(p,
	      "   setsoc done\n"
  "      when you are finished working on this social and want to keep it.\n"
	      "   setsoc abort\n"
	    "      when you want to scrap the social without saving it.\n");

}




void set_social_attribute(player * p, char *str)
{
  char *data = next_space(str), *workin, *success, *tester;
  generic_social *soc;
  int dowha, sumwon = 0;

  if (social_index < 0)
  {
    tell_player(p, " No social commands are implemented at the moment.\n");
    return;
  }

  if (!(soc = p->social))
  {
    tell_player(p, " You must actually have a social in the works to do this.\n");
    return;
  }
  if (!*str)
  {
    tell_player(p, " Format : setsoc <command> [string]\n");
    show_commands_for_social(p);
    return;
  }
  *data++ = '\0';

  dowha = get_attribute_type(soc, str);
  switch (dowha)
  {
    case -1:
      tell_player(p, " That isnt a valid command for social creation.\n");
      show_commands_for_social(p);
      return;
    case atDONE:
      keep_social(p);
      return;
    case atABORT:
      abort_social(p);
      return;
  }
  if (!*data)
  {
    tell_player(p, " Format : setsoc <command> <string>\n");
    show_commands_for_social(p);
    return;
  }
  if (strlen(data) > MAX_SOCIAL_MEMBER_LENGTH)
  {
    TELLPLAYER(p,
	       " Too long, truncated. (parts of a social maybe no longer than %d chars)\n",
	       MAX_SOCIAL_MEMBER_LENGTH - 1);
    data[MAX_SOCIAL_MEMBER_LENGTH - 1] = '\0';
  }
  if ((tester = strchr(data, '{')) && !strchr(tester, '}'))
  {
    tell_player(p, " There's an unmatched '{' in there ... try again.\n");
    return;
  }

  workin = (char *) MALLOC(strlen(data) + 1);
  if (!workin)
  {
    malloc_error(p, "set_social_attribute");
    return;
  }
  memset(workin, 0, strlen(data) + 1);
  strcpy(workin, data);


  switch (dowha)
  {
    case atFORMAT:
      if (soc->format)
	FREE(soc->format);
      soc->format = workin;
      success = "Format";
      break;
    case atROOM_MSG:
      if (soc->room_msg)
	FREE(soc->room_msg);
      soc->room_msg = workin;
      success = "Room message";
      sumwon++;
      break;
    case atUSED_ROOM_MSG:
      if (soc->used_room_msg)
	FREE(soc->used_room_msg);
      soc->used_room_msg = workin;
      success = "Used room message";
      break;
    case atDIRECT_MSG:
      if (soc->direct_msg)
	FREE(soc->direct_msg);
      soc->direct_msg = workin;
      success = "Direct message";
      sumwon++;
      break;
    case atUSED_DIRECT_MSG:
      if (soc->used_direct_msg)
	FREE(soc->used_direct_msg);
      soc->used_direct_msg = workin;
      success = "Used direct message";
      break;
    default:
      FREE(workin);
      tell_player(p, " Ouch uncase error...herms...\n");
      LOGF("error", "set_social_attribute unused case, social [%s] player [%s]",
	   soc->command, p->name);
      return;
  }
  if (sumwon)
    TELLPLAYER(p, " %s for social '%s' is now ...\n Someone %s\n",
	       success, soc->command, workin);
  else
    TELLPLAYER(p, " %s for social '%s' is now ...\n %s\n",
	       success, soc->command, workin);
  return;
}




void examine_social(player * p, char *str)
{
  char *oldstack = stack, temp[70];
  generic_social *soc;
  player *scan;
  int hit = 0;

  if (social_index < 0)
  {
    tell_player(p, " No social commands are implemented at the moment.\n");
    return;
  }

  if (!*str)
  {
    if (!(p->residency & SU))
    {
      tell_player(p, " Format : xs <social>\n");
      return;
    }
    for (scan = flatlist_start; scan; scan = scan->flat_next)
      if (scan->social)
      {
	hit++;
	TELLPLAYER(p, " %-20s : %s (%s)\n", scan->name,
		   scan->social->command, soc_type_str(scan->social));
      }
    if (!hit)
      tell_player(p, " No-one working on any socials atm.\n");
    else
      TELLPLAYER(p, " %d social/s in the works atm.\n", hit);
    return;
  }

  soc = find_social(str);
  if (!soc)
  {
    if (!p->social)
    {
      if (!(p->residency & SU))
      {
	TELLPLAYER(p, " No such social '%s' in existance.\n", str);
	return;
      }
      for (scan = flatlist_start; scan; scan = scan->flat_next)
	if (scan->social && !strcasecmp(str, scan->social->command))
	{
	  soc = scan->social;
	  break;
	}
      if (!soc)
      {
	TELLPLAYER(p, " No such social '%s' in existance (or being created).\n", str);
	return;
      }
    }
    else
      soc = p->social;
  }

  sprintf(temp, "Details of the '%s' social", soc->command);
  pstack_mid(temp);
  stack += sprintf(stack, "   Type               : %s\n", soc_type_str(soc));
  stack += sprintf(stack, "   Created by         : %s\n", soc->creator);
  if (soc->flags & stINC)
    stack += sprintf(stack, "   Date of Creation   : Not finished.\n");
  else
    stack += sprintf(stack, "   Date of Creation   : %s\n", convert_time(soc->date));

  stack += sprintf(stack, LINE);

  if (soc->format)
    stack += sprintf(stack, "Format string ...\n %s\n", soc->format);
  if (soc->room_msg)
    stack += sprintf(stack, "Room message ...\n Someone %s\n", soc->room_msg);
  if (soc->used_room_msg)
    stack += sprintf(stack, "Used room message ...\n %s\n", soc->used_room_msg);
  if (soc->direct_msg)
    stack += sprintf(stack, "Direct message ...\n Someone %s\n", soc->direct_msg);
  if (soc->used_direct_msg)
    stack += sprintf(stack, "Used direct message ...\n %s\n", soc->used_direct_msg);

  stack += sprintf(stack, LINE);
  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
}

void socials_version(void)
{
  if (social_index >= 0)
    stack += sprintf(stack, " -=*> kRad Soshuls v1.1 (by phypor) enabled.\n");
}
