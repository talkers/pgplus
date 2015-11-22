/*
 * Playground+ - room.c
 * The room and movement code
 * -----------------------------------------------------------------------
 */

#include <stdlib.h>
#include <ctype.h>
#include <memory.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#include "include/config.h"
#include "include/player.h"
#include "include/fix.h"
#include "include/proto.h"

/* externs */

#ifdef INTERCOM
extern room *intercom_room;
#endif

/* interns */

void do_room_inform(player *, room *, int);
void do_str_inform(player *, char *);
void delete_room(player * p, char *str);

dfile *room_df;

/* change decompress room to get from disk */

int decompress_room(room * r)
{
  int length;
  char *oldstack, *tmp;
  oldstack = stack;
  if (!(r->flags & COMPRESSED))
    return 1;

/*    printf("DEcompress key = %d, id = %s\n",r->data_key,r->id); */
  length = dynamic_load(room_df, r->data_key, stack);
  if (length <= 0)
  {
    current_room = r;
    delete_room(current_player, 0);
    return 0;
  }
  tmp = stack;
  stack += length;

  tmp = get_string(stack, tmp);
  length = strlen(stack) + 1;
  r->text.length = length;
  if (r->text.where)
    FREE(r->text.where);
  r->text.where = (char *) MALLOC(length);
  strcpy(r->text.where, stack);

  tmp = get_string(stack, tmp);
  length = strlen(stack) + 1;
  if (length == 1)
  {
    r->exits.length = 0;
    r->exits.where = 0;
  }
  else
  {
    r->exits.length = length;
    if (r->exits.where)
      FREE(r->exits.where);
    r->exits.where = (char *) MALLOC(length);
    strcpy(r->exits.where, stack);
  }
  tmp = get_string(stack, tmp);
  length = strlen(stack) + 1;
  if (length == 1)
  {
    r->automessage.length = 0;
    r->automessage.where = 0;
  }
  else
  {
    r->automessage.length = length;
    if (r->automessage.where)
      FREE(r->automessage.where);
    r->automessage.where = (char *) MALLOC(length);
    strcpy(r->automessage.where, stack);
  }
  r->flags &= ~(COMPRESSED | ROOM_UPDATED);
  stack = oldstack;
  return 1;
}


/* and the compress room chugs to the disk */

void compress_room(room * r)
{
  int length;
  char *oldstack;
  oldstack = stack;

  if (r->flags & COMPRESSED)
    return;
/*
   if (r->owner && r->owner->residency == STANDARD_ROOMS)
   return;
 */

/*  printf("compress key = %d, id = %s\n",r->data_key,r->id); */
  if (r->flags & ROOM_UPDATED)
  {
    if (r->text.where)
      stack = store_string(stack, r->text.where);
    else
      stack = store_string(stack, "");
    if (r->exits.where)
      stack = store_string(stack, r->exits.where);
    else
      stack = store_string(stack, "");
    if (r->automessage.where)
      stack = store_string(stack, r->automessage.where);
    else
      stack = store_string(stack, "");
    length = (int) stack - (int) oldstack;
    r->data_key = dynamic_save(room_df, oldstack, length, r->data_key);
  }
  if (r->text.where)
    FREE(r->text.where);
  r->text.where = 0;
  r->text.length = 0;
  if (r->exits.where)
    FREE(r->exits.where);
  r->exits.where = 0;
  r->exits.length = 0;
  if (r->automessage.where)
    FREE(r->automessage.where);
  r->automessage.where = 0;
  r->automessage.length = 0;
  r->flags |= COMPRESSED;
  r->flags &= ~ROOM_UPDATED;
  stack = oldstack;
}


 /* decompress room text */

void old_decompress_room(room * r)
{
  int length;
  if (!(r->flags & COMPRESSED))
    return;
  (void) get_string(stack, r->text.where);
  length = strlen(stack) + 1;
  r->text.length = length;
  if (r->text.where)
    FREE(r->text.where);
  r->text.where = (char *) MALLOC(length);
  strcpy(r->text.where, stack);

  if (r->exits.where)
  {
    (void) get_string(stack, r->exits.where);
    length = strlen(stack) + 1;
    r->exits.length = length;
    FREE(r->exits.where);
    r->exits.where = (char *) MALLOC(length);
    strcpy(r->exits.where, stack);
  }
  if (r->automessage.where)
  {
    (void) get_string(stack, r->automessage.where);
    length = strlen(stack) + 1;
    r->automessage.length = length;
    FREE(r->automessage.where);
    r->automessage.where = (char *) MALLOC(length);
    strcpy(r->automessage.where, stack);
  }
  r->flags &= ~COMPRESSED;
}

/* creates a new standard room */

room *create_room(player * p)
{
  room *r, *scan;
  saved_player *sp;
  int number, home = 1, unique = 1;
  char id[MAX_ID], buf[1024];
  sp = p->saved;
  if (!sp)
  {
    tell_player(p, " You must save first before you can create a room.\n");
    return 0;
  }
  sprintf(id, "room.%d", unique);
  do
  {
    for (scan = sp->rooms, number = 1; scan; number++, scan = scan->next)
    {
      if (!strcmp(scan->id, id))
      {
	unique++;
	sprintf(id, "room.%d", unique);
	break;
      }
      if (scan->flags & HOME_ROOM)
	home = 0;
    }
  }
  while (scan);

  if (number > p->max_rooms)
  {
    tell_player(p, " Can't create room, reached max room limit.\n");
    return 0;
  }
  r = (room *) MALLOC(sizeof(room));
  memset(r, 0, sizeof(room));
  strcpy(r->id, id);

  sprintf(buf, get_rooms_msg("def_name"), p->name);
  strncpy(r->name, buf, MAX_ROOM_NAME - 1);

  sprintf(buf, get_rooms_msg("def_entermsg"), p->name);
  strncpy(r->enter_msg, buf, MAX_ENTER_MSG - 1);

  sprintf(stack, get_rooms_msg("def_desc"), p->name);
  number = strlen(stack) + 1;
  r->text.where = (char *) MALLOC(number);
  memcpy(r->text.where, stack, number);
  r->text.length = number;

  r->flags = (home * HOME_ROOM) | ROOM_UPDATED | EXITMSGS_OK;
  r->exits.length = 0;
  r->exits.where = 0;
  r->auto_base = 30;
  r->automessage.length = 0;
  r->automessage.where = 0;
  r->owner = sp;
  r->players_top = 0;
  r->next = sp->rooms;
  sp->rooms = r;
  r->flags |= ROOM_UPDATED;
  return r;
}

 /* destroy all the room data */

void free_room_data(saved_player * sp)
{
  room *roomlist, *next;
  if (!sp->rooms)
    return;
  roomlist = sp->rooms;
  while (roomlist)
  {
    next = roomlist->next;
    if (roomlist->text.where)
      FREE(roomlist->text.where);
    if (roomlist->exits.where)
      FREE(roomlist->exits.where);
    if (roomlist->automessage.where)
      FREE(roomlist->automessage.where);
    dynamic_free(room_df, roomlist->data_key);
    FREE(roomlist);
    roomlist = next;
  }
  sp->rooms = 0;
}

/* collect all the room data ready for saving */

void construct_room_save(saved_player * sp)
{
  room *roomlist;
  char *tmpstack;
  int length;

  for (roomlist = sp->rooms; roomlist; roomlist = roomlist->next)
  {
    if (!roomlist->players_top || (sys_flags & (SHUTDOWN | PANIC)))
      compress_room(roomlist);
    tmpstack = stack;
    stack += 4;
    stack = store_string(stack, roomlist->name);
    stack = store_string(stack, roomlist->id);
    stack = store_string(stack, roomlist->enter_msg);
    stack = store_int(stack, roomlist->auto_base);
    stack = store_int(stack, roomlist->flags);
    stack = store_int(stack, roomlist->data_key);
    length = (int) stack - (int) tmpstack;
    (void) store_int(tmpstack, length);
  }
  stack = store_int(stack, 0);
}

/* retrieve room data from a save file */

char *retrieve_room_data(saved_player * sp, char *where)
{
  int length;
  room *r, **last;
  free_room_data(sp);
  last = &sp->rooms;
  where = get_int(&length, where);
  for (; length; where = get_int(&length, where))
  {
    r = (room *) MALLOC(sizeof(room));
    memset(r, 0, sizeof(room));
    where = get_string(r->name, where);
    where = get_string(r->id, where);
    where = get_string(r->enter_msg, where);
    where = get_int(&r->auto_base, where);
    where = get_int(&r->flags, where);
    where = get_int(&r->data_key, where);
/*
   if (r->flags&OPEN)
   printf("[%s.%s]\t%s\n",sp->lower_name,r->id,r->name);
   else
   printf("[%s.%s]\t%s (CLOSED)\n",sp->lower_name,r->id,r->name);
 */
    r->flags |= COMPRESSED;
    *last = r;
    last = &r->next;
    r->owner = sp;
    r->players_top = 0;
    r->next = 0;
  }
  *last = 0;
  return where;
}

/* convert a room string into an actual room */

room *convert_room_verbose(player * p, char *str, int verbose)
{
  char *scan, *oldstack;
  saved_player *sp;
  room *r;

  oldstack = stack;
  scan = str;
  while (*scan && *scan != '.')
    *stack++ = *scan++;
  if (!*scan)
  {
    if (verbose)
      tell_player(p, " Room IDs should be of the form <owner-name>."
		  "<roomid>\n");
    stack = oldstack;
    return 0;
  }
  *stack++ = 0;
  scan++;
  if (!*oldstack)
    strcpy(oldstack, p->name);
  lower_case(oldstack);
  sp = find_saved_player(oldstack);
  if (!sp)
  {
    if (verbose)
    {
      sprintf(oldstack, " Can't find player for room '%s'.\n", str);
      stack = end_string(oldstack);
      tell_player(p, oldstack);
    }
    stack = oldstack;
    return 0;
  }
  strcpy(oldstack, scan);
  lower_case(oldstack);
  stack = end_string(oldstack);
  r = sp->rooms;
  while (r)
  {
    strcpy(stack, r->id);
    lower_case(stack);
    if (!strcmp(stack, oldstack))
    {
      stack = oldstack;
      current_room = r;
      if (!decompress_room(r))
	return 0;
      return r;
    }
    if (!sp)
    {
      if (verbose)
      {
	sprintf(oldstack, " Can't find player for room '%s'.\n", str);
	stack = end_string(oldstack);
	tell_player(p, oldstack);
      }
      stack = oldstack;
      return 0;
    }
    strcpy(oldstack, scan);
    lower_case(oldstack);
    stack = end_string(oldstack);
    r = sp->rooms;
    while (r)
    {
      strcpy(stack, r->id);
      lower_case(stack);
      if (!strcmp(stack, oldstack))
      {
	stack = oldstack;
	current_room = r;
	if (!decompress_room(r))
	  return 0;
	return r;
      }
      r = r->next;
    }
    if (verbose)
    {
      sprintf(oldstack, " Can't find room '%s'.\n", scan);
      stack = end_string(oldstack);
      tell_player(p, oldstack);
    }
    stack = oldstack;
    return 0;
  }
  return r;
}

room *convert_room(player * p, char *str)
{
  return convert_room_verbose(p, str, 1);
}


/* create a new room  the player command */

void create_new_room(player * p, char *str)
{
  room *r;
  char *oldstack;

  oldstack = stack;
  r = create_room(p);
  if (r)
  {
    sprintf(oldstack, " New room created with an id of %s.%s\n", p->name,
	    r->id);
    stack = end_string(oldstack);
    tell_player(p, oldstack);
  }
  stack = oldstack;
}


/* add an exit to a room */

void add_exit(player * p, char *str)
{
  room *r;
  char *oldstack, *scan, *test;
  int count = 1;

  oldstack = stack;
  if (!current_room)
  {
    tell_player(p, " Strange, you don't actually seem to be anywhere ?!\n");
    return;
  }
  r = current_room;
  if (!convert_room(p, str))
    return;
  if (r == current_room)
  {
    tell_player(p, " You can't make a link from a room to itself.\n");
    return;
  }
  sprintf(stack, "%s.%s\n", current_room->owner->lower_name, current_room->id);
  lower_case(stack);
  stack = end_string(stack);

  scan = r->exits.where;
  test = stack;
  if (scan)
  {
    for (; *scan; count++, stack = test)
    {
      while (*scan != '\n')
	*stack++ = *scan++;
      *stack++ = '\n';
      *stack++ = 0;
      scan++;
      if (!strcmp(test, oldstack))
      {
	tell_player(p, " There already exists an exit to that room.\n");
	stack = oldstack;
	return;
      }
    }
  }
  if (count >= 3 && strcmp(p->lower_name, p->location->owner->lower_name)
      && !(p->residency & ADMIN))
  {
    tell_player(p, " Sorry, there are already 3 exits to this room.\n");
    stack = oldstack;
    return;
  }
  if (count > p->max_exits)
  {
    tell_player(p, " Sorry, you have reached you maximum exit limit "
		"in this room.\n");
    stack = oldstack;
    return;
  }
  if (r->exits.where)
    strcat(oldstack, r->exits.where);
  r->exits.length = strlen(oldstack) + 1;
  if (r->exits.where)
    FREE(r->exits.where);
  r->exits.where = (char *) MALLOC(r->exits.length);
  strcpy(r->exits.where, oldstack);
  r->flags |= ROOM_UPDATED;
  tell_player(p, " New exit added.\n");
  stack = oldstack;
}


/* remove exit */

void remove_exit_verbose(player * p, char *str, int verbose)
{
  char *exits, *oldstack, *last;
  room *r;
  oldstack = stack;
  if (!current_room)
  {
    tell_player(p, " Strange, you don't seem to be anywhere.\n");
    return;
  }
  r = current_room;
  exits = r->exits.where;
  if (!exits)
  {
    tell_player(p, " This room has no exits to delete.\n");
    return;
  }
  lower_case(str);
  while (*exits)
  {
    last = stack;
    while (*exits != '\n')
      *stack++ = *exits++;
    *stack++ = 0;
    if (!strcmp(last, str))
      break;
    if (convert_room_verbose(p, last, verbose))
    {
      strcpy(stack, current_room->id);
      lower_case(stack);
      if (!strcmp(str, stack))
	break;
    }
    *(--stack) = '\n';
    stack++;
    exits++;
  }
  if (!*exits)
  {
    if (verbose)
      tell_player(p, " No such exit to remove.\n");
    else
    {
      stack = oldstack;
      sprintf(stack, "auto-remove exit- '%s':", str);
      stack = strchr(stack, 0);
      sprintf(stack, " room: %s.%s", p->location->owner->lower_name,
	      p->location->id);
      stack = end_string(stack);
      log("error", oldstack);
      stack = oldstack;
      *stack = 0;
    }
    stack = oldstack;
    return;
  }
  exits++;
  stack = last;
  while (*exits)
    *stack++ = *exits++;
  *stack++ = 0;
  r->exits.length = strlen(oldstack) + 1;
  if (r->exits.where)
    FREE(r->exits.where);
  if (r->exits.length == 1)
  {
    r->exits.where = 0;
    r->exits.length = 0;
  }
  else
  {
    r->exits.where = (char *) MALLOC(r->exits.length);
    strcpy(r->exits.where, oldstack);
  }
  r->flags |= ROOM_UPDATED;
  if (verbose)
    tell_player(p, " Exit removed.\n");
  stack = oldstack;
}

void remove_exit(player * p, char *str)
{
  remove_exit_verbose(p, str, 1);
}

/* quick check to view stuff about a room */

void check_room(player * p, char *str)
{
  char *oldstack;
  room *r;
  oldstack = stack;
  r = current_room;
  if (!r)
  {
    tell_player(p, " Strange ! You don't appear to be anywhere.\n");
    return;
  }
  if (emote_no_break(*r->enter_msg))
    sprintf(stack, " [ %s.%s ]\n %s\n Someone%s\n", r->owner->lower_name, r->id, r->name, r->enter_msg);
  else
    sprintf(stack, " [ %s.%s ]\n %s\n Someone %s\n", r->owner->lower_name, r->id, r->name, r->enter_msg);
  stack = strchr(stack, 0);
  if (r->flags & LOCKABLE)
  {
    strcpy(stack, " This room is lockable by everybody.\n");
    stack = strchr(stack, 0);
  }
  if (r->flags & LINKABLE)
  {
    strcpy(stack, " This room is linkable by everyone.\n");
    stack = strchr(stack, 0);
  }
  if (r->flags & KEYLOCKED)
  {
    strcpy(stack, " This room is locked to those with keys.\n");
    stack = strchr(stack, 0);
  }
  if (r->flags & LOCKED)
  {
    strcpy(stack, " This room is locked.\n");
    stack = strchr(stack, 0);
  }
  if (r->flags & OPEN)
  {
    strcpy(stack, " This room is open.\n");
    stack = strchr(stack, 0);
  }
  if (r->flags & EXITMSGS_OK)
  {
    strcpy(stack, " This room permits exitmsgs to be used.\n");
    stack = strchr(stack, 0);
  }
  if (r->flags & SOUNDPROOF)
  {
    strcpy(stack, " This room is soundproofed, so shouts cannot be heard.\n");
    stack = strchr(stack, 0);
  }
  if (r->flags & ISOLATED_ROOM)
  {
    strcpy(stack, " This room is isolated, so no tells can get through.\n");
    stack = strchr(stack, 0);
  }
  if (r->flags & ANTISING)
  {
    strcpy(stack, " This room is chambered in such a way that singing is useless.\n");
    stack = strchr(stack, 0);
  }
  *stack++ = 0;
  tell_player(p, oldstack);
  stack = oldstack;
}

/* check all the exits from a room */

void check_exits(player * p, char *str)
{
  char *exits, *scan, *oldstack, *text, *end, newid[80];
  int count = 0, fill = 0, temp = 0;
  room *r;
  oldstack = stack;
  if (!current_room)
  {
    tell_player(p, " Hmm, that room doesn't exist.\n");
    return;
  }
  if (!decompress_room(current_room))
  {
    tell_player(p, " Eeek! Where did that room go?\n");
    return;
  }
  exits = current_room->exits.where;
  if (!exits)
  {
    tell_player(p, " This room has no exits.\n");
    return;
  }
  while (*exits)
  {
    while (*exits && *exits != '\n')
      *stack++ = *exits++;
    if ((*exits) == '\n')
      exits++;
    *stack++ = 0;
    count++;
  }
  end = stack;
  text = stack;
  temp = count;
  for (scan = oldstack; count > 0; count--)
  {
    memset(newid, 0, 80);
    if (!strchr(scan, '.'))	/* just id, no owner */
      sprintf(newid, "%s.%s", current_room->owner->lower_name, scan);
    else
      strcpy(newid, scan);
    r = convert_room_verbose(p, newid, 0);
/*    r = convert_room_verbose(p, scan, 0); */
    if (!r)
    {
      /*
       * erase a dud link (I hope)  mark=stack; stack=end;
       * remove_exit_verbose(p,scan,0); temp--; stack=mark;
       * stack=strchr(stack,0);
       */
      if (!strcasecmp(current_room->owner->lower_name, p->lower_name))
	stack += sprintf(stack, " Found a dud link to '%s'.\n", scan);
      else
	temp--;
    }
    else
    {
      for (fill = strlen(r->id); fill < MAX_ID; fill++)
	*stack++ = ' ';
      sprintf(stack, "%s -", r->id);
      stack = strchr(stack, 0);
      if (!possible_move(p, r, 0))
      {
	if (r->flags & LOCKED)
	  *stack++ = 'L';
	else
	  *stack++ = ' ';
      }
      else
	*stack++ = '>';
      sprintf(stack, " %s\n", r->name);
      stack = strchr(stack, 0);
    }
    scan = end_string(scan);
  }
  if (temp == 1)
    strcpy(stack, "\n There is one exit to this place.\n\n");
  else if (temp == 0)
    strcpy(stack, "\n There are no exits from here\n\n");
  else
    sprintf(stack, "\n There are %s exits from here.\n\n",
	    number2string(temp));
  stack = strchr(stack, 0);

  *stack++ = 0;
  tell_player(p, text);
  stack = oldstack;
}

/* add an automessage to a room  (based around add exit) */

void add_auto(player * p, char *str)
{
  room *r;
  char *oldstack, *scan;
  int count = 0;

  oldstack = stack;
  if (!*str)
  {
    tell_player(p, " Format +auto <string>\n");
    return;
  }
  if (!current_room)
  {
    tell_player(p, " Strange, you don't actually seem to be anywhere ?!\n");
    return;
  }
  r = current_room;
  if (strlen(str) > MAX_AUTOMESSAGE)
  {
    tell_player(p, " Thats a bit long isnt it ? Try something shorter ..\n");
    return;
  }
  while (*str)
    *stack++ = *str++;
  *stack++ = '\n';
  if (r->automessage.where)
  {
    for (scan = r->automessage.where; *scan; count++)
    {
      while (*scan != '\n')
	*stack++ = *scan++;
      *stack++ = *scan++;
    }
  }
  *stack++ = 0;
  if (count >= p->max_autos)
  {
    tell_player(p, " Sorry but you have already reached your maximum "
		"number of automessages.\n");
    stack = oldstack;
    return;
  }
  r->automessage.length = strlen(oldstack) + 1;
  if (r->automessage.where)
    FREE(r->automessage.where);
  r->automessage.where = (char *) MALLOC(r->automessage.length);
  strcpy(r->automessage.where, oldstack);
  r->flags |= ROOM_UPDATED;
  tell_player(p, " New message added.\n");
  stack = oldstack;
}


/* remove auto message */

void remove_auto(player * p, char *str)
{
  char *oldstack, *scan;
  int count;
  room *r;
  oldstack = stack;
  r = current_room;
  if (!r)
  {
    tell_player(p, " Strange, you don't seem to be anywhere.\n");
    return;
  }
  count = atoi(str);
  if (!count)
  {
    tell_player(p, " Format: -auto <number>\n");
    return;
  }
  scan = r->automessage.where;
  if (!scan)
  {
    tell_player(p, " No automessages in this room to delete.\n");
    return;
  }
  for (scan = r->automessage.where; count > 1; count--)
    if (!*scan)
    {
      tell_player(p, " No automessage of that number.\n");
      stack = oldstack;
      return;
    }
    else
    {
      while (*scan != '\n')
	*stack++ = *scan++;
      *stack++ = *scan++;
    }

  while (*scan != '\n')
    scan++;
  scan++;

  while (*scan)
    *stack++ = *scan++;
  *stack++ = 0;

  r->automessage.length = strlen(oldstack) + 1;
  if (r->automessage.where)
    FREE(r->automessage.where);
  if (r->automessage.length == 1)
  {
    r->automessage.where = 0;
    r->automessage.length = 0;
  }
  else
  {
    r->automessage.where = (char *) MALLOC(r->automessage.length);
    strcpy(r->automessage.where, oldstack);
  }
  r->flags |= ROOM_UPDATED;
  tell_player(p, " Message removed.\n");
  stack = oldstack;
}

/* list automessages */

void check_autos(player * p, char *str)
{
  char *oldstack, *scan;
  int number = 1;

  oldstack = stack;

  scan = current_room->automessage.where;
  if (!scan)
  {
    tell_player(p, " This room has no automessages.\n");
    return;
  }
  if (current_room->flags & AUTO_MESSAGE)
    strcpy(stack, " Automessages are currently turned on.\n\n");
  else
    strcpy(stack, " Automessages are currently off ...\n");
  stack = strchr(stack, 0);
  sprintf(stack, " Minimum time for an automessage set to %d seconds\n\n",
	  current_room->auto_base);
  stack = strchr(stack, 0);
  while (*scan)
  {
    sprintf(stack, " [%d] ", number);
    stack = strchr(stack, 0);
    while (*scan != '\n')
      *stack++ = *scan++;
    *stack++ = *scan++;
    number++;
  }
  *stack++ = 0;
  tell_player(p, oldstack);
  stack = oldstack;
}

/* auto command */

void autos_com(player * p, char *str)
{
  if (!strcasecmp("on", str))
  {
    current_room->flags |= AUTO_MESSAGE;
    tell_player(p, " Auto messages turned on in this room.\n");
    current_room->auto_count = 10 + (rand() & 63);
    return;
  }
  if (!strcasecmp("off", str))
  {
    current_room->flags &= ~AUTO_MESSAGE;
    tell_player(p, " Auto messages turned off in this room.\n");
    return;
  }
  check_autos(p, str);
}


/* the check room command */

void check_rooms(player * p, char *str)
{
  /* NOTE: system players, those players to whom the system rooms
     belong, do NOT seem to have a complete player record available.
     There are several bodges in here to avoid printing nonsense
     values. */

  saved_player *sp;
  char *oldstack, *cpy;
  int count = 0;
  room *scan;
  player *in;
  player dummy;

  oldstack = stack;
  if (*str && p->residency & (SU | ADMIN))
  {
    strcpy(stack, str);
    lower_case(stack);
    sp = find_saved_player(stack);
  }
  else
    sp = p->saved;
  if (!sp)
  {
    tell_player(p, " You have no save information, and therefore, no "
		"rooms either.\n");
    return;
  }
  strcpy(dummy.lower_name, sp->lower_name);
  dummy.fd = p->fd;
  load_player(&dummy);
  /* bodge to avoid crap being displayed as the name if the name
     being checked is a system room name */
  if (dummy.residency & SYSTEM_ROOM)
    strcpy(dummy.name, dummy.lower_name);

  for (scan = sp->rooms; scan; scan = scan->next)
    count++;
  if (p->saved == sp)
    sprintf(stack, " You have created %d out of a maximum of %d rooms.\n",
	    count, p->max_rooms);
  else
    /* avoid printing crap as the room limit if a system player */
  if (dummy.residency & SYSTEM_ROOM)
    sprintf(stack, " %s has created %d rooms.\n",
	    dummy.name, count);
  else
    sprintf(stack, " %s has created %d rooms, from a maximum of %d.\n",
	    dummy.name, count, dummy.max_rooms);
  stack = strchr(stack, 0);
  if (count)
  {
    strcpy(stack, "\n [NO.] [ID]           hLBOAlE!>$X  [NAME]\n");
    stack = strchr(stack, 0);
    for (scan = sp->rooms; scan; scan = scan->next)
    {
      for (count = 0, in = scan->players_top; in; in = in->room_next)
	count++;
      sprintf(stack, " [%d]", count);
      for (count = 0; *stack; stack++)
	count++;
      for (; count < 6; count++)
	*stack++ = ' ';
      for (count = 0, cpy = scan->id; *cpy; count++)
	*stack++ = *cpy++;
      for (; count < MAX_ID; count++)
	*stack++ = ' ';
      *stack++ = ' ';
      if (scan->flags & HOME_ROOM)
	*stack++ = 'h';
      else
	*stack++ = '-';
      if (scan->flags & LOCKED)
	*stack++ = 'L';
      else
	*stack++ = '-';
      if (scan->flags & KEYLOCKED)
	*stack++ = 'B';
      else
	*stack++ = '-';
      if (scan->flags & OPEN)
	*stack++ = 'O';
      else
	*stack++ = '-';
      if (scan->flags & LOCKABLE)
	*stack++ = 'A';
      else
	*stack++ = '-';
      if (scan->flags & LINKABLE)
	*stack++ = 'l';
      else
	*stack++ = '-';
      if (scan->flags & CONFERENCE)
	*stack++ = 'E';
      else
	*stack++ = '-';
      if (scan->flags & SOUNDPROOF)
	*stack++ = '!';
      else
	*stack++ = '-';
      if (scan->flags & ISOLATED_ROOM)
	*stack++ = '>';
      else
	*stack++ = '-';
      if (scan->flags & ANTISING)
	*stack++ = '$';
      else
	*stack++ = '-';
      if (scan->flags & EXITMSGS_OK)
	*stack++ = 'X';
      else
	*stack++ = '-';
      *stack++ = ' ';
      for (cpy = scan->name; *cpy;)
	*stack++ = *cpy++;
      *stack++ = '\n';
    }
  }
  *stack++ = 0;
  tell_player(p, oldstack);
  stack = oldstack;
}

static char *keycommands[] =
{"commands", "exits", "check", "lock", "lockable",
 "linkable", "open", "entermsg", "+exit", "-exit", "info",
 "name", "look", "go", "trans", "?", "help", 0};

/* the room command */

void room_command(player * p, char *str)
{
  char *rol;
  list_ent *l;
  saved_player *sp;

  if (p->edit_info)
  {
    tell_player(p, " Can't do room commands whilst in editor.\n");
    return;
  }
  if ((*str == '/') && (p->input_to_fn == room_command))
  {
    match_commands(p, str + 1);
    if (!(p->flags & PANIC) && (p->input_to_fn == room_command))
    {
      do_prompt(p, " Room Mode >");
      p->mode |= ROOMEDIT;
    }
    return;
  }
  if (*str == '@')
  {
    rol = next_space(str);
    *rol++ = 0;
    if (!convert_room(p, str + 1))
      return;
  }
  else
  {
    rol = str;
    current_room = p->location;
  }
  if (!*rol)
  {
    if (p->input_to_fn == room_command)
    {
      tell_player(p, " Format: (room) <action>\n");
      if (!(p->flags & PANIC) && (p->input_to_fn == room_command))
      {
	do_prompt(p, " Room Mode >");
	p->mode |= ROOMEDIT;
      }
      return;
    }
    else
    {
      tell_player(p, " Entering room mode. Use 'end' to leave.\n"
		  " '/<command>' does normal commands.\n");
      p->flags &= ~PROMPT;
      p->input_to_fn = room_command;
    }
  }
  else
  {
    sp = p->saved;
    l = fle_from_save(current_room->owner, p->lower_name);
    if (!l && p->residency & SU)
      l = fle_from_save(current_room->owner, "sus");
    if (!l && p->residency == NON_RESIDENT)
      l = fle_from_save(current_room->owner, "newbies");
    if (!l)
      l = fle_from_save(current_room->owner, "everyone");

    if ((current_room->flags & LINKABLE) && (!strncmp("+exit", rol, 5)) &&
	(!l || !(l->flags & KEY)))
      sub_command(p, rol, room_list);
    else if ((current_room->owner != sp) && !(p->residency & ADMIN)
	     && !(l && l->flags & SHARE_ROOM)	/* spooooooooon */
	     && strcasecmp("end", rol) && strcasecmp("create", rol))
    {
      if (l && l->flags & KEY)
      {
	int scan;
	char *found_space;
	found_space = (char *) strchr(rol, ' ');
	if (found_space)
	  *found_space = 0;
	for (scan = 0; (scan != -1) && keycommands[scan]; scan++)
	  if (!strcasecmp(keycommands[scan], rol))
	  {
	    if (found_space)
	      *found_space = ' ';
	    sub_command(p, rol, keyroom_list);
	    scan = -2;
	  }
	if (scan != -1)
	{
	  if (found_space)
	    *found_space = ' ';
	  tell_player(p, " You cannot do room operations on a "
		      "room that isn't yours.\n");
	}
      }
      else
      {
	tell_player(p, " You cannot do room operations on a room "
		    "that isn't yours.\n");
      }
    }
    else
      sub_command(p, rol, room_list);
  }
  if (!(p->flags & PANIC) && (p->input_to_fn == room_command))
  {
    do_prompt(p, "Room Mode >");
    p->mode |= ROOMEDIT;
  }
}

/* view room commands */

void view_room_commands(player * p, char *str)
{
  view_sub_commands(p, room_list);
}

/* Key sub-commands */

void view_room_key_commands(player * p, char *str)
{
  view_sub_commands(p, keyroom_list);
}


/* exit room mode */

void exit_room_mode(player * p, char *str)
{
  if (p->input_to_fn != room_command)
    return;
  tell_player(p, " Leaving room mode.\n");
  p->input_to_fn = 0;
  p->flags |= PROMPT;
  p->mode &= ~ROOMEDIT;
}


/* change the name of a room */

void change_room_name(player * p, char *str)
{
  char *oldstack;
  oldstack = stack;
  if (!*str)
  {
    sprintf(stack, " Current name of this room is ...\n%s\n",
	    current_room->name);
    stack = end_string(stack);
    tell_player(p, oldstack);
    stack = oldstack;
    return;
  }
  if (!current_room)
    return;
  if ((strlen(str) + 1) >= MAX_ROOM_NAME)
  {
    tell_player(p, " Sorry that name is too long.\n");
    stack = oldstack;
    return;
  }
  strncpy(current_room->name, str, MAX_ROOM_NAME - 3);
  sprintf(stack, " The name of room %s.%s has been changed to ...\n"
	  "%s\n", p->name, current_room->id, current_room->name);
  stack = end_string(oldstack);
  tell_player(p, oldstack);
  stack = oldstack;
}

/* change the id of a room */

void change_room_id(player * p, char *str)
{
  char *oldstack, *check;
  room *scan;
  oldstack = stack;
  if (!*str)
  {
    sprintf(stack, " Current id for this room is %s.%s\n",
	    current_room->owner->lower_name, current_room->id);
    stack = end_string(stack);
    tell_player(p, oldstack);
    stack = oldstack;
    return;
  }
  if (!current_room)
    return;
  strcpy(stack, str);
  lower_case(stack);
  stack = end_string(stack);
  scan = current_room->owner->rooms;
  for (; scan; scan = scan->next)
  {
    strcpy(stack, scan->id);
    lower_case(stack);
    if (!strcmp(stack, oldstack))
    {
      tell_player(p, " A room with that ID already exists.\n");
      stack = oldstack;
      return;
    }
  }
  if ((strlen(str) + 1) >= MAX_ID)
  {
    tell_player(p, " Sorry that name is too long.\n");
    stack = oldstack;
    return;
  }
  for (check = str; *check; check++)
    if (!(isalpha(*check) || isdigit(*check)))
    {
      tell_player(p, " A room-id can contain alphanumeric characters"
		  " only.\n");
      stack = oldstack;
      return;
    }
  strncpy(current_room->id, str, MAX_ID - 3);
  sprintf(oldstack, " The id of the room is now %s.%s\n", p->name,
	  current_room->id);
  stack = end_string(oldstack);
  tell_player(p, oldstack);
  stack = oldstack;
}


/* change the entermsg of a room */

void change_room_entermsg(player * p, char *str)
{
  char *oldstack;
  oldstack = stack;
  if (!current_room)
    return;
  if (!*str)
  {
    if (emote_no_break(*current_room->enter_msg))
      sprintf(stack, " Current enter message for this room is ...\n Someone%s\n", current_room->enter_msg);
    else
      sprintf(stack, " Current enter message for this room is ...\n Someone %s\n", current_room->enter_msg);
    stack = end_string(stack);
    tell_player(p, oldstack);
    stack = oldstack;
    return;
  }
  strncpy(current_room->enter_msg, str, MAX_ENTER_MSG - 3);
  if (emote_no_break(*current_room->enter_msg))
    sprintf(oldstack, " The enter message for this room is now set to ...\n Someone%s\n", current_room->enter_msg);
  else
    sprintf(oldstack, " The enter message for this room is now set to ...\n Someone %s\n", current_room->enter_msg);
  stack = end_string(oldstack);
  tell_player(p, oldstack);
  if (current_room->flags & EXITMSGS_OK)
  {
    current_room->flags &= ~EXITMSGS_OK;
    tell_player(p, " (Exitmsgs automatically disabled by changing room entermsg)\n");
  }
  stack = oldstack;
}


/* unlink a player from the rooms */

void unlink_from_room(player * p, char *str)
{
  player *p2;
  player *scan, *previous;
  if (*str)
  {
    p2 = find_player_global(str);
    if (!p2)
      return;
    if (p2->residency > p->residency)
    {
      tell_player(p, " You can't do that !!.\n");
      return;
    }
  }
  else
    p2 = p;
  if (p->location)
  {
    previous = 0;
    scan = p->location->players_top;
    while (scan && scan != p)
    {
      previous = scan;
      scan = scan->room_next;
    }
    if (!scan)
      log("error", "Bad Location list");
    else if (!previous)
      p->location->players_top = p->room_next;
    else
      previous->room_next = p->room_next;
    tell_player(p, " Unlinked from location ...\n");
    return;
  }
  else
    tell_player(p, " Strange, you don't seem to be anywhere ?!\n");
}

/* the look command */

void look(player * p, char *str)
{
  room *r;
  char *oldstack, *text;
  int count = 0, i;
  player *scan, **list;

  r = p->location;
  if (!r)
  {
    tell_player(p, " Strange, you don't seem to actually be anywhere ?!\n");
    return;
  }
  oldstack = stack;

  if (!decompress_room(r))
  {
    tell_player(p, " Eeek, where did that room go?\n");
    return;
  }
  align(stack);
  list = (player **) stack;

  for (scan = r->players_top; scan; scan = scan->room_next)
    if (scan != p)
    {
      *(player **) stack = scan;
      stack += sizeof(player **);
      count++;
    }
  text = stack;

  /* str can be NULL, say when a person logs in */
  if (!(p->tag_flags & BLOCK_ROOM_DESC && (str == NULL || strcmp(str, "+")))
      && (str == NULL || strcmp(str, "-")))
  {
    /* omit room desc if argument is "-" */
    strcpy(stack, r->text.where);
    stack = strchr(stack, 0);
    strcpy(stack, "^N");
    stack = strchr(stack, 0);
  }
  if (count)
  {
    if (count == 1)
      strcpy(stack, get_rooms_msg("one_here"));
    else
      sprintf(stack, get_rooms_msg("multiple_here"), number2string(count));
    stack = strchr(stack, 0);
    if (count > 16)
      construct_name_list(list, count);
    else
      for (i = count; i; i--, list++)
      {
	if (emote_no_break(*(*list)->title))
	  sprintf(stack, "%s%s^N\n", (*list)->name, (*list)->title);
	else
	  sprintf(stack, "%s %s^N\n", (*list)->name, (*list)->title);
	stack = strchr(stack, 0);
      }
  }
  else
  {
    strcpy(stack, get_rooms_msg("noone_here"));
    stack = strchr(stack, 0);
  }
  *stack++ = 0;
  tell_player(p, text);
  stack = oldstack;

#ifdef INTERCOM
  if (r == intercom_room)
    intercom_room_look(p);
#endif


}

/* trans, move without messages */

void trans_to(player * p, char *str)
{
  room *r;
  player *previous, *scan;
  r = convert_room(p, str);
  if (!r)
    return;
  if (r == p->location)
    return;
  if (p->location)
  {
    previous = 0;
    scan = p->location->players_top;
    while (scan && scan != p)
    {
      previous = scan;
      scan = scan->room_next;
    }
    if (!scan)
      log("error", "Bad Location list");
    else if (!previous)
    {
      p->location->players_top = p->room_next;
      if (!(p->location->players_top))
	compress_room(p->location);
    }
    else
      previous->room_next = p->room_next;
  }
  p->room_next = r->players_top;
  r->players_top = p;
  p->location = r;
  look(p, 0);
}

/* check for possible move */

int possible_move(player * p, room * r, int verbose)
{
  list_ent *l;
  if (!strcmp(r->owner->lower_name, p->lower_name))
    return 1;
  l = fle_from_save(current_room->owner, p->lower_name);
  if (!l && p->residency & SU)
    l = fle_from_save(current_room->owner, "sus");
  if (!l && p->residency == NON_RESIDENT)
    l = fle_from_save(current_room->owner, "newbies");
  if (!l)
    l = fle_from_save(current_room->owner, "everyone");

  if ((r->flags & LOCKED && !(l && l->flags & (KEY | SHARE_ROOM))))
  {
    if (verbose)
      tell_player(p, " You can't enter that place, because it is locked.\n");
    return 0;
  }
  if (l && l->flags & SHARE_ROOM)
  {
    return 1;
  }
  if (r->flags & KEYLOCKED)
  {
    if (verbose)
      tell_player(p, " You can't enter that place, because it is bolted.\n");
    return 0;
  }
  if (l && l->flags & KEY)
    return 1;
  if (!(r->flags & OPEN))
    if (!l || !(l->flags & INVITE))
    {
      if (verbose)
	tell_player(p, " You have no invite to enter that "
		    "place.\n");
      return 0;
    }
  if (l && l->flags & BAR)
  {
    if (verbose)
      tell_player(p, " You have been barred from entering "
		  "that place.\n");
    return 0;
  }
  return 1;
}


/* normal move routine, with messages */
/* moveflag is 0:normal  1:grab  */

void move_to(player * p, char *str, int moveflag)
{
  room *r, *old_room;
  player *previous = 0, *scan, *old_current;
  char *oldstack, buf[1024];

  oldstack = stack;
  r = convert_room(p, str);
  if (!r)
  {
    tell_player(p, " Erk! That room doesn't exist!\n");
    return;
  }
  if (r == p->location)
  {
    tell_player(p, " But you're already there!\n");
    return;
  }
  if (!(command_type & ADMIN_BARGE))
  {
    if (moveflag != 1 && !possible_move(p, r, 1))
      return;
    if ((r->flags & LOCKED || r->flags & KEYLOCKED) && moveflag == 1
	&& current_player->location->owner != current_player->saved)
    {
      tell_current(" The room is locked !\n");
      return;
    }
  }
  old_current = current_player;
  current_player = p;

  old_room = p->location;

  if (old_room)
  {
    scan = old_room->players_top;
    if (scan == p)
      old_room->players_top = p->room_next;
    else
    {
      while (scan && scan != p)
      {
	previous = scan;
	scan = scan->room_next;
      }
      if (!scan)
	log("error", "Bad Location list");
      else
	previous->room_next = p->room_next;
    }

    if (old_room->players_top)
    {
      if (!moveflag)
      {
	if (r->flags & EXITMSGS_OK && p->exitmsg[0])
	{
	  if (emote_no_break(*p->exitmsg))
	    sprintf(stack, " %s%s^N\n", p->name, p->exitmsg);
	  else
	    sprintf(stack, " %s %s^N\n", p->name, p->exitmsg);
	}
	else
	{
	  if (emote_no_break(*r->enter_msg))
	    sprintf(stack, " %s%s^N\n", p->name, r->enter_msg);
	  else
	    sprintf(stack, " %s %s^N\n", p->name, r->enter_msg);
	}
      }
      else
      {				/* use buf on these to save soft formatting */
	sprintf(buf, get_rooms_msg("grabbed_from"), p->name);
	sprintf(stack, " %s^N\n", buf);
      }
      stack = end_string(stack);
      tell_room(p->location, oldstack);
      stack = oldstack;
    }
    else
    {
      compress_room(old_room);
      if ((old_room->flags & (LOCKABLE | LOCKED)) == (LOCKABLE | LOCKED))
	old_room->flags &= ~LOCKED;
    }
  }
  p->room_next = r->players_top;
  r->players_top = p;
  p->location = r;
  if (moveflag)
  {
    sprintf(buf, get_rooms_msg("grabbed_to"), p->name);
    sprintf(stack, " %s\n", buf);
  }
  else
  {
    if (emote_no_break(*p->enter_msg))
      sprintf(stack, " %s%s^N\n", p->name, p->enter_msg);
    else
      sprintf(stack, " %s %s^N\n", p->name, p->enter_msg);
  }
  stack = end_string(stack);
  tell_room(p->location, oldstack);
  stack = oldstack;
  look(p, 0);
  if (p->custom_flags & SHOW_EXITS)
    check_exits(p, 0);
  current_player = old_current;
}


/* trans command */

void trans_fn(player * p, char *str)
{
  room *r;

  if (!*str)
  {
    tell_player(p, " Format: trans <someone.some-room-id>\n");
    return;
  }
  if (p->no_move)
  {
    tell_player(p, " You appear to be stuck to the ground.\n");
    return;
  }
  r = p->location;
  move_to(p, str, 0);
  if (r != p->location)
    do_str_inform(p, str);
}


/* go, the normal move command */


void go_room(player * p, char *str)
{
  char *exits, *oldstack = stack, *start, newid[80], *dotter;

  if (!*str)
  {
    tell_player(p, " Format: go <room>\n");
    return;
  }
  if (!p->location)
  {
    tell_player(p, " Strange, you don't seem to be anyhwere.\n");
    return;
  }
  exits = p->location->exits.where;
  if (!exits)
  {
    tell_player(p, " There aren't any exits from this room.\n");
    return;
  }
  lower_case(str);
  while (*exits)
  {
    start = exits;
    if ((dotter = strchr(exits, '.')))
      exits = ++dotter;
    while (*exits != '\n')
      *stack++ = tolower(*exits++);
    *stack++ = 0;
    stack = oldstack;
    if (match_player(oldstack, str))
      break;
    exits++;
  }
  if (!*exits)
  {
    TELLPLAYER(p, " Can't find exit '%s' to go through.\n", str);
    stack = oldstack;
    return;
  }
  while (*start != '\n')
    *stack++ = *start++;
  *stack++ = 0;
  if (!strchr(oldstack, '.'))
    sprintf(newid, "%s.%s", p->location->owner->lower_name, oldstack);
  else
    strcpy(newid, oldstack);
  move_to(p, newid, 0);
  sprintf(stack, "%s.%s", p->location->owner->lower_name, p->location->id);
  if (!strcasecmp(newid, stack))
    do_str_inform(p, newid);
  stack = oldstack;
}

#ifdef NULLcode
void go_room(player * p, char *str)
{
  char *exits, *oldstack, *start;

  if (!*str)
  {
    tell_player(p, " Format: go <room>\n");
    return;
  }
  oldstack = stack;
  if (!p->location)
  {
    tell_player(p, " Strange, you don't seem to be anyhwere.\n");
    return;
  }
  exits = p->location->exits.where;
  if (!exits)
  {
    tell_player(p, " Eek ! this place hasn't got any exits.\n");
    return;
  }
  lower_case(str);
  while (*exits)
  {
    start = exits;
    while (*exits != '.')
      exits++;
    exits++;
    while (*exits != '\n')
      *stack++ = tolower(*exits++);
    *stack++ = 0;
    stack = oldstack;
    if (match_player(oldstack, str))
      break;
    exits++;
  }
  if (!*exits)
  {
    sprintf(oldstack, " Can't find exit '%s' to go through.\n", str);
    stack = end_string(oldstack);
    tell_player(p, oldstack);
    stack = oldstack;
    return;
  }
  while (*start != '\n')
    *stack++ = *start++;
  *stack++ = 0;
  move_to(p, oldstack, 0);
  sprintf(stack, "%s.%s", p->location->owner->lower_name, p->location->id);
  if (!strcasecmp(oldstack, stack))
    do_str_inform(p, oldstack);
  stack = oldstack;
}
#endif /* NULLcode */

/* load in the standard rooms and initialise */

char *get_line(file * f)
{
  char *start;
  while (*(f->where) == '#')
  {
    while ((f->length > 0) && *(f->where) != '\n')
    {
      f->where++;
      f->length--;
    }
    if (f->length == 0)
      return 0;
    f->where++;
    f->length--;
  }
  start = f->where;
  while (*(f->where) && *(f->where) != '\n' && *(f->where) != '\r')
  {
    f->where++;
    f->length--;
  }
  if (*(f->where))
  {
    *(f->where)++ = 0;
    f->length--;
  }
  if (*(f->where) == '\r' || *(f->where) == '\n')
  {
    *(f->where)++ = 0;
    f->length--;
  }
  return start;
}

void init_room(char *name, file rf)
{
  char *line, *oldstack, *file_start;
  saved_player *sp;
  room *r;
  player np;

  oldstack = stack;

  file_start = rf.where;

  sp = find_saved_player(name);
  if (!sp)
  {
    memset(&np, 0, sizeof(player));
    np.fd = 0;
    np.location = (room *) - 1;
    restore_player(&np, name);
    np.residency = SYSTEM_ROOM;
    np.saved_residency = SYSTEM_ROOM;
    np.email[0] = -1;
    np.password[0] = -1;
    np.max_exits = 30;
    np.max_rooms = 30;
    np.max_autos = 30;
    np.max_list = 1;
    save_player(&np);
    sp = find_saved_player(name);
    sp->list_top = 0;
  }
  sp->residency = SYSTEM_ROOM;
  sp->rooms = 0;

  while (rf.length > 0)
  {
    line = get_line(&rf);
    if (rf.length <= 0 || !(*line))
      break;
    r = (room *) MALLOC(sizeof(room));
    memset(r, 0, sizeof(room));
    r->owner = sp;
    r->players_top = 0;
    r->next = sp->rooms;
    sp->rooms = r;
    strncpy(r->id, line, MAX_ID - 3);
    line = get_line(&rf);
    strncpy(r->name, line, MAX_ROOM_NAME - 3);
    line = get_line(&rf);
    strncpy(r->enter_msg, line, MAX_ENTER_MSG - 3);
    r->flags = OPEN | ROOM_UPDATED;

    while (*(rf.where) != '#')
      if (*(rf.where) != '\r')
      {
	*stack++ = *rf.where++;
	rf.length--;
      }
      else
      {
	rf.where++;
	rf.length--;
      }
    *stack++ = 0;
    r->text.length = strlen(oldstack) + 1;
    r->text.where = (char *) MALLOC(r->text.length);
    strcpy(r->text.where, oldstack);
    stack = oldstack;
    line = get_line(&rf);
    while (strcasecmp(line, "end"))
    {
      while (*line)
	*stack++ = *line++;
      *stack++ = '\n';
      line = get_line(&rf);
    }
    *stack++ = 0;
    r->exits.length = strlen(oldstack) + 1;
    if (r->exits.length == 1)
    {
      r->exits.where = 0;
      r->exits.length = 0;
    }
    else
    {
      r->exits.where = (char *) MALLOC(r->exits.length);
      strcpy(r->exits.where, oldstack);
    }
    stack = oldstack;

    line = get_line(&rf);
    while (strcasecmp(line, "end"))
    {
      while (*line)
	*stack++ = *line++;
      *stack++ = '\n';
      line = get_line(&rf);
    }
    *stack++ = 0;
    r->automessage.length = strlen(oldstack) + 1;
    if (r->automessage.length == 1)
    {
      r->automessage.where = 0;
      r->automessage.length = 0;
    }
    else
    {
      r->automessage.where = (char *) MALLOC(r->automessage.length);
      strcpy(r->automessage.where, oldstack);
      r->flags |= AUTO_MESSAGE;
    }
    stack = oldstack;
  }
  FREE(file_start);
  stack = oldstack;
}


void init_rooms()
{
  file lf;

  if (sys_flags & VERBOSE)
    log("boot", "Loading rooms.");

  room_df = dynamic_init("rooms", 256);

  lf = load_file("files/system.rooms");
  init_room("system", lf);
  lf = load_file("files/main.rooms");
  init_room(SYS_ROOM_OWNER, lf);
#ifdef INTERCOM
  lf = load_file("files/intercom.rooms");
  init_room("intercom", lf);
  intercom_room = convert_room(stdout_player, "intercom.external");
#endif
  entrance_room = convert_room(stdout_player, sys_room_id(ENTRANCE_ROOM));
  entrance_room->auto_base = 924;
  prison = convert_room(stdout_player, "system.prison");
  relaxed = convert_room(stdout_player, sys_room_id(RELAXED_ROOM));
  comfy = convert_room(stdout_player, sys_room_id("comfy"));
  comfy->flags |= (OPEN | LOCKED | CONFERENCE | ISOLATED_ROOM);
  boot_room = convert_room(stdout_player, sys_room_id("boot"));

  if (sys_flags & VERBOSE)
    log("boot", "Common rooms loaded.");
}


/* delete room */

void delete_room(player * p, char *str)
{
  char *oldstack;
  room *scan, **previous, *r;
  oldstack = stack;
  if (!current_room)
  {
    tell_player(p, " Strange you don't appear to be anywhere.\n");
    return;
  }
  if (current_room->owner != p->saved && !(p->residency & LOWER_ADMIN))
  {
    tell_player(p, " This is not your room, though!\n");
    return;
  }
  if (!strcmp("system", current_room->owner->lower_name) &&
      !strcmp("void", current_room->id))
  {
    tell_player(p, " You can't delete the void !\n");
    return;
  }
  r = current_room;
  while (r->players_top)
  {
    tell_player(r->players_top, " The room around you vanishes !\n");
    trans_to(r->players_top, "system.void");
  }
  previous = &(r->owner->rooms);
  scan = *previous;
  for (; scan && scan != r; scan = scan->next)
    previous = &(scan->next);
  if (scan)
  {
    *previous = scan->next;
    if (scan->text.where)
      FREE(scan->text.where);
    if (scan->exits.where)
      FREE(scan->exits.where);
    if (scan->automessage.where)
      FREE(scan->automessage.where);
    dynamic_free(room_df, scan->data_key);
    FREE(scan);
  }
  else
    *previous = 0;
  if (p)
    tell_player(p, " Room deleted.\n");
  stack = oldstack;
}

/* the where command */

void stack_where_string(player * p, player * test)
{
  char namestring[MAX_PRETITLE + MAX_NAME + 10];
  list_ent *l;
  int entry = 0;

  if (p->custom_flags & NOPREFIX || strlen(test->pretitle) == 0)
    sprintf(namestring, " %s", test->name);
  else
    sprintf(namestring, " %s %s", test->pretitle, test->name);

  if (p == test && test->location)
    /* where the person doing the command is */
  {
    if (!(test->location))
    {
      sprintf(stack, " You find yourself, *eek*!!! Nowhere at all!\n");
    }
    else
    {
      sprintf(stack, " You find yourself %s^N\n", test->location->name);
    }
    stack = strchr(stack, 0);
    return;
  }
  /* now consider where other people are */
  if (!(test->location))
    /* if they're nowhere, wibble and exit */
  {
    sprintf(stack, " %s is, *eek*!!! Nowhere at all!\n", namestring);
    stack = strchr(stack, 0);
    return;
  }
  sprintf(stack, "%s.%s", test->location->owner->lower_name,
	  test->location->id);

  if (!strcmp(stack, sys_room_id(ENTRANCE_ROOM)))
    /* if they are in the main room */
  {
    sprintf(stack, " %s is %s^N\n", namestring, test->location->name);
    stack = strchr(stack, 0);
    return;
  }
  if (test->location->owner == p->saved)
  {
    sprintf(stack, " %s is %s^N\n", namestring, test->location->name);
    stack = strchr(stack, 0);
    return;
  }
  l = find_list_entry(test, p->lower_name);
  if (!l && p->residency & SU)
    l = find_list_entry(test, "sus");
  if (!l && p->residency == NON_RESIDENT)
    l = find_list_entry(test, "newbies");
  if (!l)
    l = find_list_entry(test, "everyone");
  if (l)
    entry = l->flags;
  if ((test->custom_flags & HIDING) && !(p->residency & ADMIN) &&
      !(entry & FIND))
  {
    sprintf(stack, " %s is hiding away.\n", namestring);
    stack = strchr(stack, 0);
    return;
  }
  if (!test->location)
  {
    sprintf(stack, " Wierd, %s doesn't appear to be anywhere.\n",
	    namestring);
    stack = strchr(stack, 0);
    return;
  }
  sprintf(stack, " %s is %s^N", namestring, test->location->name);
  stack = strchr(stack, 0);
  if (test->custom_flags & HIDING)
  {
    strcpy(stack, " (hiding)");
    stack = strchr(stack, 0);
  }
  if (p->residency & ADMIN)
  {
    sprintf(stack, " ( %s.%s )\n", test->location->owner->lower_name,
	    test->location->id);
    stack = strchr(stack, 0);
  }
  else
  {
    *stack++ = '\n';
    *stack = 0;
  }
}

/*
 * void where_friends(player *p) { char *oldstack; list_ent *l; player *p2;
 * int count=0;
 * 
 * oldstack=stack; l=p->saved->list_top; while (l) {
 * p2=find_player_global_quiet(l->name); if (p2) { stack_where_string(p,p2);
 * count++; } l=l->next; } stack++=0; if (!count) tell_player(p,"You have no
 * friends on at the moment.\n"); else tell_player(p,oldstack); }
 */


void where(player * p, char *str)
{
  player **list, **step;
  int i, n;
  player *scan;
  char *oldstack, middle[80], *wh;
  int page, pages, count;

  oldstack = stack;
  command_type |= SEE_ERROR;
  if (isalpha(*str) && !strstr(str, "everyone"))
  {
    if (!strcasecmp(str, "am i"))
    {
      sprintf(stack, " You are %s\n", p->location->name);
      stack = end_string(stack);
      tell_player(p, oldstack);
      stack = oldstack;
      return;
    }
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
    wh = stack;
    for (step = list, i = 0; i < n; i++, step++)
    {
      stack_where_string(p, *step);
      stack++;
      tell_player(p, wh);
      stack = wh;
    }
    cleanup_tag(list, n);
    stack = oldstack;
    return;
  }
  if (strstr(str, "everyone"))
  {
    wh = str;
    str = strchr(str, ' ');
    if (!str)
    {
      str = wh;
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

  if (current_players == 1)
  {
    strcpy(middle, "There is only you on the program at the moment");
  }
  else
  {
    sprintf(middle, "There are %s people on the program",
	    number2string(current_players));
  }
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
    {
      count--;
    }
  }

  for (count = 0; (count < (TERM_LINES - 1) && scan); scan = scan->flat_next)
  {
    if (scan->name[0] && scan->location)
    {
      stack_where_string(p, scan);
      count++;
    }
  }
  sprintf(middle, "Page %d of %d", page + 1, pages + 1);
  pstack_mid(middle);

  *stack++ = 0;
  tell_player(p, oldstack);
  stack = oldstack;
}


/* change room flags */

void room_entry(player * p, char *str)
{
  current_room->flags ^= CONFERENCE;
  if (current_room->flags & CONFERENCE)
    tell_player(p, " People are now able to connect to this room.\n");
  else
    tell_player(p, " People are now unable to connect to this room.\n");
}

void room_exitmsgs_ok(player * p, char *str)
{
  current_room->flags ^= EXITMSGS_OK;
  if (current_room->flags & EXITMSGS_OK)
    tell_player(p, " People may display their exitmsg when entering this room.\n");
  else
    tell_player(p, " People are forced to display the room entermsg when entering here.\n");
}

void room_soundproof(player * p, char *str)
{
  char *oldstack;

  oldstack = stack;
  current_room->flags ^= SOUNDPROOF;
  if (current_room->flags & SOUNDPROOF)
  {
    tell_player(p, " You seal the room in thick, soundproof plastic.\n");
    sprintf(stack, " %s seals the room in thick plastic... \n", p->name);
  }
  else
  {
    tell_player(p, " You unseal the room, and allow shouts to enter.\n");
    sprintf(stack, " %s removes the sound barrier.. \n", p->name);
  }
  stack = end_string(stack);
  to_room1(p->location, oldstack, p);
  stack = oldstack;
}

void room_isolate(player * p, char *str)
{
  char *oldstack;

  oldstack = stack;
  current_room->flags ^= ISOLATED_ROOM;
  if (current_room->flags & ISOLATED_ROOM)
  {
    tell_player(p, " You totally isolate the room from annoying tells.\n");
    sprintf(stack, " %s unlinks us from the rest of the world..\n", p->name);
  }
  else
  {
    tell_player(p, " You re-establish a link to the outside world.\n");
    sprintf(stack, " %s relinks the room to the rest of the world..\n", p->name);
  }
  stack = end_string(stack);
  to_room1(p->location, oldstack, p);
  stack = oldstack;
}


void room_antising(player * p, char *str)
{
  char *oldstack;

  oldstack = stack;
  current_room->flags ^= ANTISING;
  if (current_room->flags & ANTISING)
  {
    tell_player(p, " You chamber the room to prevent singing in this room.\n");
    sprintf(stack, " %s chambers the room, and singing becomes useless..\n", p->name);
  }
  else
  {
    tell_player(p, " You re-allow singing.. no matter how bad it is.\n");
    sprintf(stack, " %s unchambers the room, and you can sing again..\n", p->name);
  }
  stack = end_string(stack);
  to_room1(p->location, oldstack, p);
  stack = oldstack;
}



void room_lock(player * p, char *str)
{
  list_ent *l;
  char *oldstack;

  oldstack = stack;
  l = fle_from_save(current_room->owner, p->lower_name);
  if (!l && p->residency & SU)
    l = fle_from_save(current_room->owner, "sus");
  if (!l && p->residency == NON_RESIDENT)
    l = fle_from_save(current_room->owner, "newbies");
  if (!l)
    l = fle_from_save(current_room->owner, "everyone");


  if (strcmp(current_room->owner->lower_name, p->lower_name) &&
      !(current_room->flags & LOCKABLE) && !(p->residency & ADMIN)
      && (!(l && l->flags & (KEY | SHARE_ROOM))))
  {
    tell_player(p, " This is not your room to lock.\n");
    return;
  }
  if (!strcasecmp("off", str))
    current_room->flags &= ~LOCKED;
  else if (!strcasecmp("on", str))
    current_room->flags |= LOCKED;
  else
    current_room->flags ^= LOCKED;

  if (current_room->flags & LOCKED)
  {
    tell_player(p, " Room is now locked.\n");
    sprintf(stack, " %s turns the key in the door, locking the room ...\n",
	    p->name);
  }
  else
  {
    tell_player(p, " Room is now unlocked.\n");
    sprintf(stack, " %s unlocks the door ...\n", p->name);
  }
  stack = end_string(stack);
  to_room1(p->location, oldstack, p);
  stack = oldstack;
}

/* Make room lockable by all */

void room_lockable(player * p, char *str)
{
  if (!strcasecmp("off", str))
    current_room->flags &= ~LOCKABLE;
  else if (!strcasecmp("on", str))
    current_room->flags |= LOCKABLE;
  else
    current_room->flags ^= LOCKABLE;

  if (current_room->flags & LOCKABLE)
    tell_player(p, " Room is now lockable by everyone.\n");
  else
    tell_player(p, " Room is now lockable only by keyholders.\n");
}

/* Make room linkable by all */

void room_linkable(player * p, char *str)
{
  if (!strcasecmp("off", str))
    current_room->flags &= ~LINKABLE;
  else if (!strcasecmp("on", str))
    current_room->flags |= LINKABLE;
  else
    current_room->flags ^= LINKABLE;

  if (current_room->flags & LINKABLE)
    tell_player(p, " Room is now linkable by all. (ie. People can make "
		"links _from_ it.)\n");
  else
    tell_player(p, " Room is now not linkable by all.\n");
}

void room_open(player * p, char *str)
{
  char *oldstack;

  oldstack = stack;

  if (!strcasecmp("off", str))
    current_room->flags &= ~OPEN;
  else if (!strcasecmp("on", str))
    current_room->flags |= OPEN;
  else
    current_room->flags ^= OPEN;

  if (current_room->flags & OPEN)
  {
    tell_player(p, " Room is now open for all to come inside.\n");
    sprintf(stack, " %s opens up the room to the rabble outside ...\n",
	    p->name);
  }
  else
  {
    tell_player(p, " Room is now open for invites only.\n");
    sprintf(stack, " %s closes the room ...\n", p->name);
  }
  stack = end_string(stack);
  to_room1(p->location, oldstack, p);
  stack = oldstack;
}

/* HARD lock the room (noone at all can get in) */

void room_bolt(player * p, char *str)
{
  char *oldstack;

  oldstack = stack;
  if (!strcasecmp("off", str))
    current_room->flags &= ~KEYLOCKED;
  else if (!strcasecmp("on", str))
    current_room->flags |= KEYLOCKED;
  else
    current_room->flags ^= KEYLOCKED;
  if (current_room->flags & KEYLOCKED)
  {
    tell_player(p, " Room is now totally locked to anyone but you.\n");
    sprintf(stack, " %s bolts the door of the room ...\n", p->name);
  }
  else
  {
    tell_player(p, " Room is now not totally locked.\n");
    sprintf(stack, " %s unbolts the door ...\n", p->name);
  }
  stack = end_string(stack);
  to_room1(p->location, oldstack, p);
  stack = oldstack;
}


void set_home(player * p, char *str)
{
  room *r;
  if (!p->saved)
  {
    tell_player(p, " Strange, you appear to have no save file.\n");
    return;
  }
  r = p->saved->rooms;
  if (!r)
  {
    tell_player(p, " Strange, you appear to have no rooms.\n");
    return;
  }
  for (; r; r = r->next)
    r->flags &= ~HOME_ROOM;
  current_room->flags |= HOME_ROOM;
  tell_player(p, " Current room is now home room.\n");
}



/* go home */

void go_home(player * p, char *str)
{
  room *r;
  char *oldstack;

  oldstack = stack;
  if (!p->saved)
  {
    tell_player(p, " You have no save files, and therefore no home.\n");
    return;
  }
  if (p->no_move)
  {
    tell_player(p, " You appear to be stuck to the ground.\n");
    return;
  }
  r = p->saved->rooms;
  if (!r)
  {
    tell_player(p, " You have no rooms in which to have a home.\n");
    return;
  }
  for (; r; r = r->next)
    if (r->flags & HOME_ROOM)
      break;
  if (!r)
  {
    tell_player(p, " You don't appear to have a home room.\n");
    return;
  }
  sprintf(oldstack, "%s.%s", r->owner->lower_name, r->id);
  stack = end_string(oldstack);
  move_to(p, oldstack, 0);
  stack = oldstack;
}

/* visit someone */

void visit(player * p, char *str)
{
  room *r;
  saved_player *sp;
  player *p2;
  char *oldstack;
  oldstack = stack;

  if (p->no_move)
  {
    tell_player(p, " You appear to be stuck to the ground.\n");
    return;
  }
  strcpy(stack, str);
  lower_case(stack);
  stack = end_string(stack);
  p2 = find_player_global_quiet(oldstack);
  if (p2)
  {
    sp = p2->saved;
    if (!sp)
    {
      tell_player(p, " That player has no rooms.\n");
      stack = oldstack;
      return;
    }
  }
  else
  {
    sp = find_saved_player(oldstack);
  }
  if (!sp)
  {
    sprintf(oldstack, " Can't find player '%s' in save files.\n", str);
    stack = end_string(oldstack);
    tell_player(p, oldstack);
    stack = oldstack;
    return;
  }
  r = sp->rooms;
  if (!r)
  {
    tell_player(p, " That person has no rooms.\n");
    stack = oldstack;
    return;
  }
  for (; r; r = r->next)
  {
    if (r->flags & HOME_ROOM)
    {
      break;
    }
  }
  if (!r)
  {
    tell_player(p, " That player doesn't appear to have a home room.\n");
    stack = oldstack;
    return;
  }
  sprintf(oldstack, "%s.%s", r->owner->lower_name, r->id);
  stack = end_string(oldstack);
  move_to(p, oldstack, 0);
  do_room_inform(p, r, 4);
  stack = oldstack;
}

/* back to start */

void go_main(player * p, char *str)
{
  if (p->location && !strcmp(p->location->owner->lower_name, SYS_ROOM_OWNER) &&
      !(strcmp(p->location->id, "room")))
  {
    tell_player(p, " You are already in the main room.\n");
    return;
  }
  if (p->no_move)
  {
    tell_player(p, " You seem to be stuck to the ground\n");
    return;
  }
  move_to(p, sys_room_id(ENTRANCE_ROOM), 0);
}


/* grab someone */

void grab_once(player * p, player * p2)
{
  char *oldstack = stack;
  list_ent *l;

  l = find_list_entry(p2, p->lower_name);
  if (!(p->residency & ADMIN))
  {
    if ((p->location->flags & LOCKED || p->location->flags & KEYLOCKED)
	&& p->location->owner != p->saved && !(p->residency & ADMIN)
	&& !(l && l->flags & (KEY | SHARE_ROOM)))
    {
      tell_player(p, " You can't grab people into locked rooms ...\n");
      sys_flags |= FAILED_COMMAND;
      return;
    }
    if (!(p2->location))
    {
      TELLPLAYER(p, " '%s' doesn't appear to be anywhere !!\n", p2->name);
      sys_flags |= FAILED_COMMAND;
      return;
    }
    if (p2->location == prison)
    {
      TELLPLAYER(p, " You fail to grab %s from prison!!\n", p2->name);
      sys_flags |= FAILED_COMMAND;
      return;
    }
    if (!(p->residency & PSU) && p2->location == boot_room)
    {
      TELLPLAYER(p, " You fail to grab %s from the boot room!!\n", p2->name);
      sys_flags |= FAILED_COMMAND;
      return;
    }
    if (!(p2->residency == NON_RESIDENT && p->residency & SU))
    {
      l = find_list_entry(p2, p->lower_name);
      if (!l && p->residency & SU)
	l = find_list_entry(p2, "sus");
      if (!l && p->residency == NON_RESIDENT)
	l = find_list_entry(p2, "newbies");
      if (!l)
	l = find_list_entry(p2, "everyone");
      if (!l || !(l->flags & GRAB))
      {
	TELLPLAYER(p, " You can't grab %s !!\n", p2->name);
	sys_flags |= FAILED_COMMAND;
	return;
      }
    }
  }
  else
  {
    command_type |= ADMIN_BARGE;
  }
  if (p2->location == p->location)
  {
    TELLPLAYER(p, " %s is already in the same room as you.\n", p2->name);
    return;
  }
  sprintf(stack, get_rooms_msg("got_grabbed"), p->name);
  stack = end_string(stack);
  TELLPLAYER(p2, " %s\n", oldstack);
  stack = oldstack;
  command_type &= ~PERSONAL;

  if (p2->no_move && (!(p->residency & ADMIN)))
  {
    TELLPLAYER(p2, " %s\n", get_rooms_msg("cant_move"));
    TELLPLAYER(p, " %s appears to be stuck to the ground !!!\n", p2->name);
    sys_flags |= FAILED_COMMAND;
    return;
  }
  sprintf(stack, "%s.%s", current_room->owner->lower_name, current_room->id);
  stack = end_string(oldstack);
  move_to(p2, oldstack, 1);
  do_room_inform(p2, current_room, 3);
  stack = oldstack;

  if (p2->location != p->location)
  {
    TELLPLAYER(p, " You fail to grab %s.\n", p2->name);
    sys_flags |= FAILED_COMMAND;
    return;
  }
}


void do_grab(player * p, char *str)
{
  player **list, **step;
  char *oldstack, *text, *pstring;
  int n, i;
  oldstack = stack;

  align(stack);
  list = (player **) stack;

  if (!*str)
  {
    tell_player(p, " Format : grab <player(s)>\n");
    return;
  }
  if (!strcasecmp(str, "everyone"))
  {
    tell_player(p, " You can't grab *everyone*\n");
    return;
  }
#ifdef ALLOW_MULTIS
  n = global_tag(p, str, 0);
#else
  n = global_tag(p, str);
#endif
  if (!n)
    return;

  for (step = list, i = 0; i < n; i++, step++)
  {
    if (p != *step)
      grab_once(p, *step);
    else
    {
      text = stack;
      sprintf(stack, " %s grabs %s, oooya !!\n", p->name, name_string(0, p));
      stack = end_string(stack);
      tell_room(p->location, text);
      stack = text;
    }
  }

  if (!(sys_flags & FAILED_COMMAND))
  {
    pstring = tag_string(p, list, n);
    text = stack;
    sprintf(text, get_rooms_msg("did_grab"), pstring);
    stack = end_string(text);
    TELLPLAYER(p, " %s\n", text);
    stack = text;
  }
  cleanup_tag(list, n);
  stack = oldstack;
}



/* stuff for editing a room */

void quit_room_edit(player * p)
{
  tell_player(p, " Leaving editor WITHOUT storing changes.\n");
  p->mode &= ~ROOMEDIT;
}

void end_room_edit(player * p)
{
  room *r;
  if (!p->saved)
  {
    tell_player(p, " You don't seem to have a saved file.\n");
    return;
  }
  r = (room *) p->edit_info->misc;
  if (r->text.where)
    FREE(r->text.where);
  r->text.length = p->edit_info->size;
  r->text.where = (char *) MALLOC(r->text.length);
  memcpy(r->text.where, p->edit_info->buffer, r->text.length);
  r->flags |= ROOM_UPDATED;
  tell_player(p, " Ending edit and KEEPING changes.\n");
  p->mode &= ~ROOMEDIT;
  if (p->edit_info->input_copy == room_command)
  {
    do_prompt(p, "Room Mode >");
    p->mode |= ROOMEDIT;
  }
}

void room_edit(player * p, char *str)
{
  start_edit(p, MAX_ROOM_SIZE, end_room_edit, quit_room_edit,
	     current_room->text.where);
  if (!p->edit_info)
    return;
  p->edit_info->misc = (void *) current_room;
  p->mode |= ROOMEDIT;
}

/* find random open room */

room *random_room(void)
{
  saved_player *scan;
  char letter;
  int hash;
  room *r;

  letter = (char) (rand() % 26);
  hash = (int) (rand() % HASH_SIZE);
  if (letter < 0)
    letter = -letter;
  if (hash < 0)
    hash = -hash;

  while (1)
  {
    scan = find_top_player(letter, hash);
    for (; scan; scan = scan->next)
    {
      /* if you change the criteria, do change them above in
         check_bounceable() too */
      for (r = scan->rooms; r; r = r->next)
	if (r->flags & OPEN && !(r->flags & LOCKED) && r != prison
	    && !(r->flags & KEYLOCKED) && r != current_room)
	  return r;
    }
    hash = (hash + 1) % HASH_SIZE;
    if (!hash)
      /* hmmm - maybe a mod in here? rather unlikely to ever happen 
         with a decent room count though */
      letter++;
  }
}


/* bounce */

void bounce(player * p, char *str)
{
  room *r;
  char *oldstack;
  player *scan;

  command_type = 0;
  /* check for player stuck to ground */
  if (p->no_move)
  {
    tell_player(p, " You seem to be stuck to the ground\n");
    return;
  }
  /* tell the room the leavemsg for bounce */
  oldstack = stack;
  sprintf(oldstack, " %s spins round, and bounces into the air !!\n", p->name);
  stack = end_string(oldstack);
  tell_room(p->location, oldstack);

  /* find a random room */
  r = random_room();

  /* and move there */
  sprintf(oldstack, "%s.%s", r->owner->lower_name, r->id);
  stack = end_string(oldstack);
  trans_to(p, oldstack);

  stack = oldstack;
  if (emote_no_break(*p->enter_msg))
    sprintf(stack, " %s%s\n", p->name, p->enter_msg);
  else
    sprintf(stack, " %s %s\n", p->name, p->enter_msg);
  stack = end_string(stack);

  for (scan = p->location->players_top; scan; scan = scan->room_next)
    if (scan != p)
      tell_player(scan, oldstack);

  do_room_inform(p, r, 2);

  /* Show bouncee the exits if they asked */
  if (p->custom_flags & SHOW_EXITS)
    check_exits(p, 0);

  stack = oldstack;
}


/* actually do a boot */

int do_boot(player * p, player * booted)
{
  room *r;
  list_ent *l;
  char *oldstack;

  r = (booted)->location;

  if (!r)
  {
    oldstack = stack;
    sprintf(stack, " %s doesn't appear to be anywhere.\n", (booted)->name);
    stack = end_string(stack);
    tell_player(p, oldstack);
    stack = oldstack;
    return 0;
  }
  command_type = 0;
  if ((!strcmp(r->owner->lower_name, "system")
       || !strcmp(r->owner->lower_name, SYS_ROOM_OWNER))
      && ((p->residency & SU && !(sys_flags & EVERYONE_TAG))
	  || p->residency & ADMIN)
      && check_privs(p->residency, booted->residency))
  {
    if ((booted->location == boot_room) && (booted->no_move > 0))
    {
      tell_player(p, " That player is already in the boot room!\n");
      return 0;
    }
    *((player **) stack) = booted;
    stack += sizeof(player *);
    (booted)->flags &= ~TAGGED;
    oldstack = stack;
    sprintf(stack, " %s sends you to detention for being naughty.\n",
	    p->name);
    stack = end_string(stack);
    tell_player(booted, oldstack);
    booted->booted_count++;
    p->num_booted++;
    sprintf(oldstack, " %s is sent to detention by %s for being naughty.\n",
	    (booted)->name, p->name);
    stack = end_string(oldstack);
    tell_room_but(booted, (booted)->location, oldstack);

    if (!strcmp(r->owner->lower_name, SYS_ROOM_OWNER) && !strcmp(r->id, "boot"))
    {
      strcpy(oldstack, sys_room_id(ENTRANCE_ROOM));
    }
    else
    {
      strcpy(oldstack, sys_room_id("boot"));
    }

    stack = end_string(oldstack);
    trans_to(booted, oldstack);
    sprintf(oldstack, " %s dusts %sself off after getting booted.\n",
	    (booted)->name, get_gender_string(booted));
    stack = end_string(oldstack);
    tell_room_but(booted, (booted)->location, oldstack);
    stack = oldstack;
    booted->no_shout = 60;
    booted->no_move = 60;
    return 1;
  }
  l = fle_from_save(r->owner, p->lower_name);
  if (!l && p->residency & SU)
    l = fle_from_save(r->owner, "sus");
  if (!l && p->residency == NON_RESIDENT)
    l = fle_from_save(r->owner, "newbies");
  if (!l)
    l = fle_from_save(r->owner, "everyone");

  if (((r->owner) != (p->saved)) && !(p->residency & ADMIN) &&
      (!(l && l->flags & (KEY | SHARE_ROOM))))
  {
    oldstack = stack;
    sprintf(stack, " %s is not in one of your rooms\n", (booted)->name);
    stack = end_string(stack);
    tell_player(p, oldstack);
    stack = oldstack;
    return 0;
  }
  *((player **) stack) = booted;
  stack += sizeof(player *);
  (booted)->flags &= ~TAGGED;
  oldstack = stack;
  sprintf(stack, " %s picks you up and boots you out the room.\n", p->name);
  stack = end_string(stack);
  tell_player(booted, oldstack);
  sprintf(oldstack, " %s is picked up by %s and booted out the room.\n",
	  (booted)->name, p->name);
  stack = end_string(oldstack);
  tell_room_but(booted, (booted)->location, oldstack);
  trans_to(booted, sys_room_id(ENTRANCE_ROOM));
  sprintf(oldstack, " %s dusts %sself down after getting booted.\n",
	  (booted)->name, get_gender_string(booted));
  stack = end_string(oldstack);
  tell_room_but(booted, (booted)->location, oldstack);
  stack = oldstack;
  return 1;
}

/* the boot command */

void boot_out(player * p, char *str)
{
  char *pstring, *final, *space;
  char *oldstack;
  player **list, **step, **new_list;
  int i, n, count = 0;

  command_type = SEE_ERROR;
  oldstack = stack;
  align(stack);
  list = (player **) stack;

  if (!*str)
  {
    tell_player(p, " Format : boot person<s>\n");
    return;
  }
  space = strchr(str, ' ');
  if (space)
    *space = 0;

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
  align(stack);
  new_list = (player **) stack;
  for (step = list, i = 0; i < n; i++, step++)
  {
    if (*step != p)
    {
      count += do_boot(p, *step);
    }
  }

  if (!count)
  {
    tell_player(p, " You fail to boot anybody anywhere.\n");
  }
  else
  {
    pstring = tag_string(p, new_list, count);
    final = stack;
    sprintf(stack, " You boot %s out of your rooms.\n", pstring);
    stack = end_string(stack);
    tell_player(p, final);
  }

  cleanup_tag(list, n);
  stack = oldstack;
}



/* join someone soemwhere */

void join(player * p, char *str)
{
  list_ent *l;
  char *oldstack;
  player *p2;
  room *r;
  oldstack = stack;

  if (!*str)
  {
    tell_player(p, " Format: join <person>\n");
    return;
  }
  if (p->no_move)
  {
    tell_player(p, " You appear to be stuck to the ground.\n");
    return;
  }
  p2 = find_player_global(str);
  if (!p2)
    return;
  if (!(p2->saved))
    l = 0;
  else
  {
    l = fle_from_save(p2->saved, p->lower_name);
    if (!l && p->residency & SU)
      l = fle_from_save(p2->saved, "sus");
    if (!l && p->residency == NON_RESIDENT)
      l = fle_from_save(p2->saved, "newbies");
    if (!l)
      l = fle_from_save(p2->saved, "everyone");
  }

  if (p2->custom_flags & HIDING && !(p->residency & ADMIN) &&
      (!l || (l && !(l->flags & FIND))))
  {
    sprintf(stack, " %s is in hiding, so you don't know where %s is.\n",
	    full_name(p2), gstring(p2));
    stack = end_string(stack);
    tell_player(p, oldstack);
    stack = oldstack;
    return;
  }
  r = p2->location;
  if (!r)
  {
    sprintf(stack, " %s doesn't appear to be anywhere !\n", full_name(p2));
    stack = end_string(stack);
    tell_player(p, oldstack);
    stack = oldstack;
    return;
  }
  if (r == p->location)
  {
    sprintf(stack, " You are already in the same room as %s.\n",
	    full_name(p2));
    stack = end_string(stack);
    tell_player(p, oldstack);
    stack = oldstack;
    return;
  }
  sprintf(stack, "%s.%s", r->owner->lower_name, r->id);
  stack = end_string(stack);
  move_to(p, oldstack, 0);
  if (p->location == r)
  {
    do_room_inform(p, r, 1);
  }
  stack = oldstack;
}



/* change the base of the auto timer */

void change_auto_base(player * p, char *str)
{
  room *r;
  int i;
  char *oldstack;
  oldstack = stack;
  r = current_room;
  if (!r)
  {
    tell_player(p, " Strange  ! You don't appear to be anywhere.\n");
    return;
  }
  i = atoi(str);
  if (i <= 0)
  {
    tell_player(p, " Format: speed <number>\n");
    return;
  }
  r->auto_base = i;
  sprintf(stack, " Autos set to minimum speed of every %d seconds.\n", i);
  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
}



/* with command */

void with(player * p, char *str)
{
  char *oldstack, *text, namestring[40];
  player *p2, **list, *scan;
  list_ent *l;
  room *r;
  int count = 0, hide_count = 0, entry = 0;
  oldstack = stack;
  if (!*str)
  {
    tell_player(p, " Format: with <player>\n");
    return;
  }
  if (strcasecmp(str, "me"))
  {
    p2 = find_player_global(str);
  }
  else
  {
    p2 = p;
  }
  if (!p2)
    return;

  if (p->custom_flags & NOPREFIX || strlen(p2->pretitle) == 0)
    strcpy(namestring, p2->name);
  else
    sprintf(namestring, "%s %s", p2->pretitle, p2->name);

  l = find_list_entry(p2, p->lower_name);
  if (!l && p->residency & SU)
    l = find_list_entry(p2, "sus");
  if (!l && p->residency == NON_RESIDENT)
    l = find_list_entry(p2, "newbies");
  if (!l)
    l = find_list_entry(p2, "everyone");
  if (l)
    entry = l->flags;
  if (p2->custom_flags & HIDING && !(p->residency & ADMIN) && !(entry & FIND)
      && p != p2)
  {
    sprintf(stack, " You fail miserably trying to find '%s' never mind "
	    "anyone %s is with.\n", namestring, gstring(p2));
    stack = end_string(stack);
    tell_player(p, oldstack);
    stack = oldstack;
    return;
  }
  align(stack);
  list = (player **) stack;
  r = p2->location;
  if (!r)
  {
    tell_player(p, " That person doesn't appear to be anywhere !\n");
    stack = oldstack;
    return;
  }
  for (scan = r->players_top; scan; scan = scan->room_next)
    if (scan != p2 && !(scan->custom_flags & HIDING))
    {
      *((player **) stack) = scan;
      stack += sizeof(player *);
      count++;
    }
    else
    {
      if (scan != p2 && p->residency & ADMIN)
      {
	*((player **) stack) = scan;
	stack += sizeof(player *);
	count++;
      }
      else if (scan == p2)
      {
	;			/* do nothing */
      }
      else
      {
	l = 0;

	l = find_list_entry(scan, p->lower_name);
	if (!l && p->residency & SU)
	  l = find_list_entry(scan, "sus");
	if (!l && p->residency == NON_RESIDENT)
	  l = find_list_entry(scan, "newbies");
	if (!l)
	  l = find_list_entry(scan, "everyone");
	if (!l || !(l->flags & FIND))
	  hide_count++;
	else
	{
	  *((player **) stack) = scan;
	  stack += sizeof(player *);
	  count++;
	}
      }
    }
  text = stack;

  if (!count && (hide_count == 0))
  {
    sprintf(text, " Aaaw, %s is all alone.\n", namestring);
    stack = end_string(text);
    tell_player(p, text);
    stack = oldstack;
    return;
  }
  if (!count)
  {
    sprintf(text, " You cannot discern who is with %s.\n", namestring);
    stack = end_string(text);
    tell_player(p, text);
    stack = oldstack;
    return;
  }
  if ((hide_count != 0))
    sprintf(text, " %s is with, among others ....\n", namestring);
  else
    sprintf(text, " %s is with ...\n", namestring);
  stack = strchr(stack, 0);
  if (p2->custom_flags & HIDING)
  {
    sprintf(stack, " NOTE: %s is hidden.\n", p2->name);
    stack = strchr(stack, 0);
  }
  if (p->custom_flags & NOPREFIX)
    sys_flags |= NO_PRETITLES;
  construct_name_list(list, count);
  if (p->custom_flags & NOPREFIX)
    sys_flags &= ~NO_PRETITLES;
  *stack++ = 0;
  tell_player(p, text);
  stack = oldstack;
}

/* grabable command */

void grabable(player * p, char *str)
{
  char *oldstack, *text;
  player **list, *scan;
  int count = 0;
  list_ent *l;

  oldstack = stack;
  align(stack);
  list = (player **) stack;

  for (scan = flatlist_start; scan; scan = scan->flat_next)
  {
    l = find_list_entry(scan, p->lower_name);
    if (!l && p->residency & SU)
      l = find_list_entry(scan, "sus");
    if (!l && p->residency == NON_RESIDENT)
      l = find_list_entry(scan, "newbies");
    if (!l)
      l = find_list_entry(scan, "everyone");
    if (l && l->flags & GRAB && scan != p)
    {
      *((player **) stack) = scan;
      stack += sizeof(player *);
      count++;
    }
  }

  text = stack;

  if (!count)
  {
    tell_player(p, " You can't grab anyone.\n");
    stack = oldstack;
    return;
  }
  strcpy(text, " You can grab ...\n");
  stack = strchr(stack, 0);
  construct_name_list(list, count);
  *stack++ = 0;
  tell_player(p, text);
  stack = oldstack;
}


/* invites: All the invites you have */

void invites_list(player * p, char *str)
{
  char *oldstack, *text;
  player **list, *scan;
  int count = 0;
  list_ent *l;
  oldstack = stack;

  align(stack);
  list = (player **) stack;


  for (scan = flatlist_start; scan; scan = scan->flat_next)
  {
    l = find_list_entry(scan, p->lower_name);
    if (!l)
      l = find_list_entry(scan, "everyone");
    if (l && l->flags & INVITE)
    {
      *((player **) stack) = scan;
      stack += sizeof(player *);
      count++;
    }
  }

  text = stack;

  if (!count)
  {
    tell_player(p, " You aren't invited by anyone!\n");
    stack = oldstack;
    return;
  }
  strcpy(text, " You have invites from ...\n");
  stack = strchr(stack, 0);
  construct_name_list(list, count);
  *stack++ = 0;
  tell_player(p, text);
  stack = oldstack;
}

/* admin transfer room */

void transfer_room(player * p, char *str)
{
  saved_player *sp;
  char *oldstack;
  room *r, *prev = 0, *scan;
  oldstack = stack;

  if (!*str)
  {
    tell_player(p, " Format: transfer <saved player>\n");
    return;
  }
  strcpy(stack, str);
  lower_case(stack);
  sp = find_saved_player(stack);
  if (!sp)
  {
    tell_player(p, " No such person in saved files.\n");
    return;
  }
  r = current_room;
  if (!r)
  {
    tell_player(p, " You don't appear to be anywhere !!\n");
    return;
  }
  scan = r->owner->rooms;
  if (scan == r)
    r->owner->rooms = r->next;
  else
  {
    while (scan && scan != r)
    {
      prev = scan;
      scan = scan->next;
    }
    if (!scan)
    {
      tell_player(p, " Eeeek, bad room listing, abort.\n");
      return;
    }
    prev->next = r->next;
  }

  r->owner = sp;
  r->next = sp->rooms;
  sp->rooms = r;

  tell_player(p, " Room transfered.\n");
  stack = oldstack;
}

void to_room1(room * r, char *str, player * p)
{
  to_room2(r, str, p, 0);
}

void to_room2(room * r, char *str, player * p1, player * p2)
{
  player *scan;

  for (scan = r->players_top; scan; scan = scan->room_next)
    if ((scan != p1) && (scan != p2))
      tell_player(scan, str);
}

void room_link(player * p, char *str)
{
  static char roomname[80];
  room *linkto, *r;
  int nomove = 1;

  if (!*str)
  {
    tell_player(p, " Format: room link <owner-name>.<id>\n");
    return;
  }
  r = p->location;
  if ((linkto = convert_room_verbose(p, str, 0)) &&
      (nomove = possible_move(p, linkto, 1)) && (linkto->flags & LINKABLE ||
		!strcmp(linkto->owner->lower_name, p->saved->lower_name)) &&
      r != linkto)
  {
    tell_player(p, " Adding link to current room ...\n");
    sprintf(roomname, "%s.%s", r->owner->lower_name, r->id);
    add_exit(p, roomname);
    tell_player(p, " Adding link to other room ...\n");
    add_exit(p, str);
    return;
  }
  if (!linkto)
  {
    tell_player(p, " That other room doesn't exist.\n");
    return;
  }
  if (!nomove)
    tell_player(p, " But you're not allowed in the other room!\n");
  else
    tell_player(p, " Sorry, you can't do that.\n");
}

/* Fear this CPH rip-off */

void mindseye(player * p, char *str)
{
  char *oldstack;
  room *r;
  player *inroom;
  saved_player *sp;
  list_ent *l;

  if (!p->saved || !p->saved->rooms)
  {
    tell_player(p, " You have no rooms!\n");
    return;
  }
  if (!*str)
  {
    r = p->saved->rooms;
    for (; r; r = r->next)
      if (r->flags & HOME_ROOM)
	break;
    if (!r)
    {
      tell_player(p, " You don't have a home room, do you...\n");
      return;
    }
    if (r == p->location)
    {
      tell_player(p, " You're already in that room.\n");
      return;
    }
  }
  else
  {
    r = convert_room_verbose(p, str, 0);
    if (!r)
    {
      lower_case(str);
      sp = find_saved_player(str);
      if (sp)
      {
	for (r = sp->rooms; r; r = r->next)
	  if (r->flags & HOME_ROOM)
	    break;
      }
      if (!r)
      {
	tell_player(p, " Bad room id on mindseye.\n");
	return;
      }
    }
    l = fle_from_save(r->owner, p->lower_name);

    if (strcmp(r->owner->lower_name, p->lower_name)
	&& !(p->residency & ADMIN) && !(l && l->flags & SHARE_ROOM))
    {
      tell_player(p, " But you don't own that room!\n");
      return;
    }
  }
  inroom = r->players_top;
  if (!inroom)
  {
    if (r->flags & HOME_ROOM && p == (player *) r->owner)
    {
      tell_player(p, " There is no-one in your home room.\n");
    }
    else
    {
      tell_player(p, " There is no-one in that room.\n");
    }
    return;
  }
  oldstack = stack;
  if (r->flags & HOME_ROOM && p == (player *) r->owner)
  {
    strcpy(stack, "\n You peer into your home room, and you see ...\n");
  }
  else if (strcmp(str, r->owner->lower_name))
  {
    sprintf(stack, "\n You peer into \'%s\', and see ...\n", str);
  }
  else
  {
    sprintf(stack, "\n You peer into %s's home room, and see ...\n",
	    r->owner->lower_name);
  }
  stack = strchr(stack, 0);
  while (inroom)
  {
    if (p->custom_flags & NOPREFIX)
      sprintf(stack, "  %s %s (%s idle)\n", inroom->name, inroom->title,
	      word_time(inroom->idle));
    else
      sprintf(stack, "  %s %s (%s idle)\n", full_name(inroom),
	      inroom->title, word_time(inroom->idle));
    stack = strchr(stack, 0);
    inroom = inroom->room_next;
  }
  stack = strchr(stack, 0);
  *stack++ = '\n';
  *stack++ = 0;
  tell_player(p, oldstack);
  stack = oldstack;
}

void prison_player(player * p, char *str)
{
  char *oldstack, *mark;
  char fortime[128];
  int timeout = 1;
  player *p2;

  oldstack = stack;
  if (!*str)
  {
    tell_player(p, " Format: jail <person> [<timeout>]\n");
    return;
  }
  mark = strchr(str, ' ');
  if (mark)
  {
    *mark++ = 0;
    timeout = atoi(mark);
    if (!timeout)
      timeout = 1;
  }
  if (!strcasecmp(str, "me"))
  {
    p2 = p;
    timeout = -1;
  }
  else
    p2 = find_player_global(str);
  if (!p2)
    return;
  if (p2 == p)
  {
    p->jail_timeout = -1;
    move_to(p, "system.prison", 0);
    return;
  }
  if (!check_privs(p->residency, p2->residency))
  {
    tell_player(p, " Nah na nah nah naaaahh! You can't!\n");
    return;
  }
  if (timeout > 10)
    timeout = 10;
  if (timeout > 0)
    sprintf(fortime, "for %d minutes.", timeout);
  else
  {
    strcpy(fortime, "for ..ever!");
    p2->system_flags |= SAVEDJAIL;
  }
  command_type |= HIGHLIGHT;
  tell_player(p2, " You suddenly find yourself grabbed, and flung into the "
	      "prison ...\n");
  p2->jail_timeout = (timeout * 60);
  command_type &= ~HIGHLIGHT;
  move_to(p2, "system.prison", 0);
  p2->mode = 0;
  p2->input_to_fn = 0;
  p2->booted_count++;
  p->num_booted++;
  sprintf(stack, " You throw %s in prison ... %s\n", p2->name, fortime);
  stack = end_string(oldstack);
  tell_player(p, oldstack);
  stack = oldstack;
  sprintf(stack, " -=*> %s throws %s in jail, %s\n", p->name, p2->name, fortime);
  stack = end_string(stack);
  su_wall(oldstack);
  stack = oldstack;
  sprintf(stack, "%s throws %s in jail, %s", p->name, p2->name, fortime);
  stack = end_string(stack);
  log("jail", oldstack);
  if (timeout < 0)
    log("forever", oldstack);
  stack = oldstack;
}


void set_login_room(player * p, char *str)
{
  char *oldstack;
  room *r;
  list_ent *l;

  if (!*str)
  {
    tell_player(p, " You will now connect to the main room.\n");
    strcpy(p->room_connect, "");
    p->custom_flags &= ~TRANS_TO_HOME;
    return;
  }
  r = convert_room(p, str);
  if (!r)
    return;
  l = fle_from_save(r->owner, p->lower_name);
  if (!l || !(l->flags & (SHARE_ROOM | KEY)))
  {
    if (!possible_move(p, r, 1))
      return;
    if (!(r->flags & CONFERENCE))
    {
      tell_player(p, " That room is not open for connecting to.\n");
      return;
    }
  }
  oldstack = stack;
  sprintf(stack, " You will now attempt to connect to room '%s'\n", str);
  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
  strncpy(p->room_connect, str, MAX_ROOM_CONNECT - 3);
  p->custom_flags &= ~TRANS_TO_HOME;
}

void barge(player * p, char *str)
{
  char *oldstack;
  room *to;

  oldstack = stack;
  command_type |= ADMIN_BARGE;
  if (strchr(str, '.'))
  {
    if ((to = convert_room_verbose(p, str, 0)))
    {
      if (p->exitmsg[0])
      {
	if (emote_no_break(*p->exitmsg))
	  sprintf(stack, " %s%s\n", p->name, p->exitmsg);
	else
	  sprintf(stack, " %s %s\n", p->name, p->exitmsg);
      }
      else
      {
	if (emote_no_break(*to->enter_msg))
	  sprintf(stack, " %s%s\n", p->name, to->enter_msg);
	else
	  sprintf(stack, " %s %s\n", p->name, to->enter_msg);
      }
      stack = end_string(stack);
      tell_room(p->location, oldstack);
      stack = oldstack;
      trans_to(p, str);
      if (emote_no_break(*p->enter_msg))
	sprintf(stack, " %s%s\n", p->name, p->enter_msg);
      else
	sprintf(stack, " %s %s\n", p->name, p->enter_msg);
      stack = end_string(stack);
      tell_room(p->location, oldstack);
      stack = oldstack;
    }
  }
  else if (find_saved_player(str))
    visit(p, str);
  else
    join(p, str);
  /* just to clear the command_type (should be cleared, but what the hell */
  command_type &= ~ADMIN_BARGE;
}

void inform_room_enter(player * p, char *str)
{
  if (p->custom_flags & ROOM_ENTER)
    tell_player(p, " You won't be informed when people enter your rooms.\n");
  else
    tell_player(p, " You will now be informed when someone enters one of"
		" your rooms.\n");
  p->custom_flags ^= ROOM_ENTER;
}

/*
 * inform owner when a room is entered.  1 == join 2 == bounce, 3 == grab, 4
 * == visit, 0 == normal move or go
 */

void do_room_inform(player * p, room * r, int type)
{
  player *p2;
  saved_player *sp;
  char *oldstack, *out;

  sp = r->owner;
  p2 = find_player_absolute_quiet(sp->lower_name);
  if (!p2 || !(p2->custom_flags & ROOM_ENTER))
    return;
  if (p2->location == p->location)
    return;

  oldstack = stack;
  if (p2->custom_flags & NOPREFIX || p->pretitle[0] == 0)
    sprintf(stack, " %s has just", p->name);
  else
    sprintf(stack, " %s %s has just", p->pretitle, p->name);
  stack = end_string(stack);
  out = stack;
  switch (type)
  {
    case 0:
      sprintf(stack, " %s entered %s.%s\n", oldstack, p2->name,
	      r->id);
      break;
    case 1:
      sprintf(stack, " %s joined someone in %s.%s\n", oldstack,
	      p2->name, r->id);
      break;
    case 2:
      sprintf(stack, " %s bounced into %s.%s\n", oldstack, p2->name,
	      r->id);
      break;
    case 3:
      sprintf(stack, " %s been grabbed into %s.%s\n", oldstack,
	      p2->name, r->id);
      break;
    case 4:
      sprintf(stack, " %s visited you.\n", oldstack);
      break;
  }
  stack = end_string(stack);
  tell_player(p2, out);
  stack = oldstack;
}

void do_str_inform(player * p, char *str)
{
  room *r;
  r = convert_room(p, str);
  if (!r)
    return;
  do_room_inform(p, r, 0);
}


/* Get all players out of a certain players rooms, used on nuke */

void all_players_out(saved_player * sp)
{
  room *r;
  player *p, *p2;

  for (r = sp->rooms; r; r = r->next)
  {
    for (p = r->players_top; p; p = p2)
    {
      p2 = p->room_next;
      if (strcmp(p->lower_name, sp->lower_name))
	move_to(p, "system.void", 0);
    }
  }
}
char *sys_room_id(char *which)
{
  static char buf[160];

  memset(buf, 0, 160);
  sprintf(buf, "%s.%s", SYS_ROOM_OWNER, which);
  return buf;
}
