/*
 * Playground+ - items.c
 * All the item code. Original was still under construction and not much
 * has been changed here - although it is still in a useable state
 * ---------------------------------------------------------------------------
 */

#include <ctype.h>
#include <string.h>
#ifndef BSDISH
#include <malloc.h>
#endif
#include <sys/stat.h>
#include <sys/time.h>
#include <stdio.h>
#include <fcntl.h>
#include <memory.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>

#include "include/config.h"
#include "include/player.h"
#include "include/fix.h"
#include "include/items.h"
#include "include/proto.h"

struct s_item *top_item;
int item_unique;

/* delete and entry from someones list */

void delete_entry_item(saved_player * sp, item * l)
{
  item *scan;
  if (!sp)
    return;
  scan = sp->item_top;
  if (scan == l)
  {
    sp->item_top = l->next;
    FREE(l);
    return;
  }
  while (scan)
    if (scan->next == l)
    {
      scan->next = l->next;
      FREE(l);
      return;
    }
    else
      scan = scan->next;
  log("error", "Tried to delete item that wasn't there.");
}


/* compress list */

void tmp_comp_item(saved_player * sp)
{
  char *oldstack;
  item *l, *next;

  l = sp->item_top;

  oldstack = stack;
  stack = store_int(stack, 0);

  while (l)
  {
    next = l->next;
    if (l->id < 0)
    {
      log("error", "Bad item entry on compress .. auto deleted.");
      delete_entry_item(sp, l);
    }
    else
    {
      stack = store_int(stack, l->id);
      stack = store_int(stack, l->number);
      stack = store_int(stack, l->flags);
    }
    l = next;
  }
  store_int(oldstack, ((int) stack - (int) oldstack));
}


void compress_item(saved_player * sp)
{
  char *oldstack;
  int length;
  item *new, *l, *next;
  if (sp->system_flags & COMPRESSED_ITEMS)
    return;
  sp->system_flags |= COMPRESSED_ITEMS;
  oldstack = stack;
  tmp_comp_item(sp);
  length = (int) stack - (int) oldstack;
  if (length == 4)
  {
    sp->item_top = 0;
    stack = oldstack;
    return;
  }
  new = (item *) MALLOC(length);
  memcpy(new, oldstack, length);

  l = sp->item_top;
  while (l)
  {
    next = l->next;
    FREE(l);
    l = next;
  }
  sp->item_top = new;
  stack = oldstack;
}

/* decompress list */

void decompress_item(saved_player * sp)
{
  item *l;
  char *old, *end, *start;
  int length;

  if (!(sp->system_flags & COMPRESSED_ITEMS))
    return;
  sp->system_flags &= ~COMPRESSED_ITEMS;

  old = (char *) sp->item_top;
  start = old;
  if (!old)
    return;
  old = get_int(&length, old);
  end = old + length - 4;
  sp->item_top = 0;
  while (old < end)
  {
    l = (item *) MALLOC(sizeof(item));
    old = get_int(&(l->id), old);
    old = get_int(&(l->number), old);
    old = get_int(&(l->flags), old);
    l->next = sp->item_top;
    l->loc_next = 0;
    sp->item_top = l;
    l->owner = sp;
  }
  FREE(start);
}

/* save list */

void construct_item_save(saved_player * sp)
{
  int length;
  char *where;

  if (!(sp->system_flags & COMPRESSED_ITEMS) &&
      (!find_player_absolute_quiet(sp->lower_name)))
    compress_item(sp);

  if (sp->system_flags & COMPRESSED_ITEMS)
  {
    if (sp->item_top)
    {
      where = (char *) sp->item_top;
      (void) get_int(&length, where);
      memcpy(stack, where, length);
      stack += length;
    }
    else
      stack = store_int(stack, 4);
  }
  else
    tmp_comp_item(sp);
}

/* retrieve list */

char *retrieve_item_data(saved_player * sp, char *where)
{
  int length;
  (void) get_int(&length, where);
  if (length == 4)
    sp->item_top = 0;
  else
  {
    sp->system_flags |= COMPRESSED_ITEMS;
    sp->item_top = (item *) MALLOC(length);
    memcpy(sp->item_top, where, length);
  }
  where += length;
  return where;
}

/* count list entries */

int count_items(player * p)
{
  item *l;
  int count = 0;
  if (!p->saved)
    return 0;
  if (!p->saved->item_top)
    return 0;
  for (l = p->saved->item_top; l; l = l->next)
  {
    count++;
  }
  return count;
}

/* s_item stuff starts here! */


int count_sitem(void)
{
  struct s_item *l;
  int count = 0;
  if (!top_item)
    return 0;
  for (l = top_item; l; l = l->next)
    count++;
  return count;
}

char *extract_sitem(char *stack2)
{

  struct s_item *d;

  d = (struct s_item *) MALLOC(sizeof(struct s_item));
  memset(d, 0, sizeof(struct s_item));
  stack2 = get_string(d->desc, stack2);
  stack2 = get_string(d->name, stack2);
  stack2 = get_string(d->author, stack2);
  stack2 = get_int(&d->id, stack2);
  stack2 = get_int(&d->sflags, stack2);
  stack2 = get_int(&d->value, stack2);

  d->next = top_item;
  top_item = d;
  return stack2;
}

void save_sitem(struct s_item *s)
{
  stack = store_string(stack, s->desc);
  stack = store_string(stack, s->name);
  stack = store_string(stack, s->author);
  stack = store_int(stack, s->id);
  stack = store_int(stack, s->sflags);
  stack = store_int(stack, s->value);
}

/* throw all the saved items to disk */

void sync_sitems(int background)
{
  int fd, count, len;
  char *oldstack;
  struct s_item *z;

  oldstack = stack;

  if (background && fork())
    return;

#ifdef BSDISH
  fd = open("files/items/saved.items", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
#else
  fd = open("files/items/saved.items", O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, S_IRUSR | S_IWUSR);
#endif
  if (fd < 0)
    handle_error("Failed to open items file.");
  count = count_sitem();
  stack = store_int(stack, count);
  stack = store_int(stack, item_unique);
  if (top_item)
    for (z = top_item; z; z = z->next)
    {
      save_sitem(z);
    }
  len = (int) stack - (int) oldstack;

  if (write(fd, oldstack, len) < 0)
    handle_error("Ack! Can't save the saved items!!");
  close(fd);

  stack = oldstack;

  if (background)
    exit(0);
}

/*
 * Load the items into the talker
 */


void init_sitems()
{
  int length, fd, count;
  char *oldstack;
  oldstack = stack;

  if (sys_flags & VERBOSE || sys_flags & PANIC)
    log("boot", "Loading items from disk");

  fd = open("files/items/saved.items", O_RDONLY | O_NDELAY);
  if (fd < 0)
  {
    sprintf(oldstack, "Failed to load items file");
    stack = end_string(oldstack);
    log("error", oldstack);
    top_item = 0;
    item_unique = 0;
    stack = oldstack;
  }
  else
  {
    length = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    if (length)
    {
      if (read(fd, oldstack, length) < 0)
	handle_error("Damnit, s_items aren't loading");
      stack = get_int(&count, stack);
      stack = get_int(&item_unique, stack);
      for (; count; count--)
	stack = extract_sitem(stack);
    }
    close(fd);
  }
  stack = oldstack;
}


struct s_item *find_item_before(struct s_item *scan)
{

  struct s_item *s;

  for (s = top_item; s; s = s->next)
  {
    if (s->next == scan)
      return s;
  }
  return 0;
}

struct s_item *find_item(char *name, int id)
{

  struct s_item *s;

  for (s = top_item; s; s = s->next)
    if (((name) && !strcasecmp(s->name, name)) || s->id == id)
      return s;

  return 0;
}

item *find_pitem(saved_player * sp, int id)
{

  item *i;

  if (sp->system_flags & COMPRESSED_ITEMS)
    decompress_item(sp);
  if (!(sp) || !(sp->item_top))
    return 0;
  for (i = sp->item_top; i; i = i->next)
    if (i->id == id)
      return i;

  return 0;
}


int create_sitem(char *name, char *desc, char *author)
{

  struct s_item *s, *scan;
  char *test;

  s = (struct s_item *) MALLOC(sizeof(struct s_item));
  if (!s)
    return -1;
  if (isdigit(*name))
    return -2;
  if (*name == '$')
    return -2;
  if (*name == '*')
    return -2;
  if (*name == '!')
    return -2;
  if (find_item(name, -1))
    return -1;
  for (test = name; *test; test++)
  {
    if (*test == '^')
      return -3;
  }

  item_unique = 0;
  scan = find_item(0, item_unique);
  while (scan)
  {
    item_unique++;
    scan = find_item(0, item_unique);
  }

  /* Just in case this item ID is already in use in peoples inventories */
  delete_item_from_all_players(item_unique, 0);


  strncpy(s->name, name, MAX_NAME - 2);
  strncpy(s->desc, desc, MAX_TITLE - 2);
  strncpy(s->author, author, MAX_NAME - 2);
  s->id = item_unique;
  s->next = top_item;
  s->sflags = NOT_READY_TO_BUY;
  s->value = 0;
  top_item = s;
  return s->id;
}

void create_object(player * p, char *str)
{

  char *desc;
  int new_id;

  if (!*str)
  {
    TELLPLAYER(p, " Format: create <object name> <description>\n");
    return;
  }
  desc = next_space(str);
  if (*desc)
    *desc++ = 0;
  else
  {
    TELLPLAYER(p, " Format: create <object name> <description>\n");
    return;
  }

  new_id = create_sitem(str, desc, p->name);
  if (new_id == -1)
  {
    TELLPLAYER(p, " An item with that name already exists..\n");
    return;
  }
  if (new_id == -2)
  {
    TELLPLAYER(p, " The first character of the item name cannot be $, *, !, or a digit\n");
    return;
  }
  if (new_id == -3)
  {
    TELLPLAYER(p, " The NAME of the item cannot contain color codes.\n");
    return;
  }
  TELLPLAYER(p, " Item created with ID number %d.\n", new_id);
  TELLPLAYER(p, " Now, set the flags on it with setbit %s (bit), and then the cost on it with setval %s (value)\n", str, str);
}

void delete_item_from_all_players(int id, int val)
{

  saved_player *scanlist, **hlist;
  item *i;
  int charcounter, counter;

  for (charcounter = 0; charcounter < 26; charcounter++)
  {
    hlist = saved_hash[charcounter];
    for (counter = 0; counter < HASH_SIZE; counter++, hlist++)
      for (scanlist = *hlist; scanlist; scanlist = scanlist->next)
      {

	i = find_pitem(scanlist, id);
	if (i)
	{
	  scanlist->pennies += (i->number * val);
	  delete_entry_item(scanlist, i);
	}
      }
  }
}

void reload_pennies(player * p)
{
  if (p->saved)
    p->pennies = p->saved->pennies;
}

void delete_item(player * p, char *str)
{

  struct s_item *s, *sp;
  char *reason;
  player *scan;

  if (!*str)
  {
    TELLPLAYER(p, " Format: delete <item name or number>\n");
    return;
  }
  reason = next_space(str);
  if (*reason)
    *reason++ = 0;
  if (isdigit(*str))
    s = find_item(0, atoi(str));
  else
    s = find_item(str, -1);

  if (!s)
  {
    TELLPLAYER(p, " That item doesn't exist...\n");
    return;
  }
  if (s->sflags & UNPURCHASEABLE && !(p->residency & ADMIN))
  {
    TELLPLAYER(p, " That item doesn't exist...\n");
    return;
  }
  if (p->residency & SU && *reason)
  {
    TELLPLAYER(p, " Recording your reason...\n");
  }
  else if (strcasecmp(s->author, p->name))
  {
    if (p->residency & SU)
      TELLPLAYER(p, " Format: delete <item> <reason>\n");
    else
      TELLPLAYER(p, " You can't kill an item you didn't create.\n");
    return;
  }
  if (s == top_item)
    top_item = s->next;
  else
  {
    sp = find_item_before(s);
    if (!sp)
    {
      TELLPLAYER(p, " Could not delete due to error.\n");
      LOGF("error", "Couldn't delete s_item...");
      return;
    }
    sp->next = s->next;
  }
  save_player(p);
  delete_item_from_all_players(s->id, s->value);
  for (scan = flatlist_start; scan; scan = scan->flat_next)
    reload_pennies(scan);
  if (strcasecmp(s->author, p->name))
  {
    SUWALL(" -=*> %s deleted item %s because: %s\n",
	   p->name, s->name, reason);
    LOGF("item_delete", "%s deleted %s (creator: %s) because: %s",
	 p->name, s->author, s->name, reason);
  }
  FREE(s);
}

void inventory(player * u, char *str)
{

  item *i, *inext;
  struct s_item *s;
  player *p;
  char *oldstack = stack;
  char top[70];

  if (*str && u->residency & ADMIN)
  {
    p = find_player_global(str);
    if (!p)
      return;
    sprintf(top, "%s is carrying", p->name);
  }
  else
  {
    p = u;
    sprintf(top, "You are carrying");
  }
  if (!p->saved)
  {
    TELLPLAYER(u, " Wait..you're not even saved yet....\n");
    stack = oldstack;
    return;
  }
  pstack_mid(top);

  if (!p->saved->item_top)
  {
    stack += sprintf(stack, "\n [$$$$] %d %s -- ", p->pennies,
		     get_config_msg("cash_name"));
    stack += sprintf(stack, "%s\n\n" LINE, get_config_msg("cash_desc"));
    stack = end_string(stack);
    tell_player(u, oldstack);
    stack = oldstack;
    return;
  }
  ADDSTACK("\n");
  for (i = p->saved->item_top; i; i = inext)
  {

    inext = i->next;
    s = find_item(0, i->id);
    if (!s)
    {
      delete_entry_item(p->saved, i);
    }
    else
    {
      if (i->flags & BROKEN)
	ADDSTACK(" [%-4d] (broken) %d %s -- %s\n", i->id, i->number, s->name, s->desc);
      else if (i->flags & WORN)
	ADDSTACK(" [%-4d] (worn) %d %s -- %s\n", i->id, i->number, s->name, s->desc);
      else if (i->flags & WIELDED)
	ADDSTACK(" [%-4d] (wielded) %d %s -- %s\n", i->id, i->number, s->name, s->desc);
      else if (i->flags & OPEN)
	ADDSTACK(" [%-4d] (open) %d %s -- %s\n", i->id, i->number, s->name, s->desc);
      else
	ADDSTACK(" [%-4d] %d %s -- %s\n", i->id, i->number, s->name, s->desc);
    }
  }
  ADDSTACK(" [$$$$] %d %s -- ", p->pennies, get_config_msg("cash_name"));
  ADDSTACK("%s\n\n", get_config_msg("cash_desc"));
  ENDSTACK(LINE);
  pager(u, oldstack);
  stack = oldstack;
}

void view_store(player * p, char *str)
{

  struct s_item *s;
  int scan = 0, scanflag = 0, do_desc = 1, count = 0, max = 0;
  char *oldstack, cr[] = "Creator", id[] = "ID#", na[] = "Item Name", co[] = "Cost",
    fl[] = "Flags";
  char top[70];
  char bot[80];

  if (!*str)
  {
    TELLPLAYER(p, " Format: lso [!]<$(+-)price|(+-)flag|&&creator|*|(1st char)>\n");
    return;
  }
  /* if we start with !, block the desc */
  if (*str == '!')
  {
    do_desc = 0;
    str++;
    if (!*str)
    {
      TELLPLAYER(p, " Format: lso !<other options>\n");
      return;
    }
  }
  /* at this point, make sure if we're dealing with a char that
     we have it in lower_case form */
  *str = tolower(*str);
  if (*str == '&')
  {
    scanflag = 5;
    str++;
    if (!*str)
    {
      TELLPLAYER(p, " Format: lso [!]&<creator name>\n");
      return;
    }
  }
  else if (*str == '$')
  {
    scan = atoi(str + 2);
    if (scan <= 0)
    {
      *(str + 1) = '%';
    }
    if (*(str + 1) == '-')
      scanflag = 1;
    else if (*(str + 1) == '+')
      scanflag = 2;
    else
    {
      TELLPLAYER(p, " Format: lso $+<price> for items costing more than <price>\n");
      TELLPLAYER(p, " Format: lso $-<price> for items costing less than <price>\n");
      return;
    }
  }
  else if (*str == '+')
  {
    scanflag = 3;
  }
  else if (*str == '-')
  {
    scanflag = 4;
  }
  if (scanflag == 3 || scanflag == 4)
  {
    if (!strcasecmp(str + 1, "wield"))
      scan = WIELDABLE;
    else if (!strcasecmp(str + 1, "wear"))
      scan = WEARABLE;
    else if (!strcasecmp(str + 1, "food"))
      scan = EDIBLE;
    else if (!strcasecmp(str + 1, "drink"))
      scan = DRINKABLE;
    else if (!strcasecmp(str + 1, "toy"))
      scan = PLAYTHING;
    else if (!strcasecmp(str + 1, "bag"))
      scan = CONTAINER;
    else if (!strcasecmp(str + 1, "unique"))
      scan = UNIQUE;

    if (scan == 0)
    {
      TELLPLAYER(p, " Try wield, wear, food, drink, toy, bag or unique...\n");
      return;
    }
  }
  oldstack = stack;
  sprintf(top, "%s items", get_config_msg("talker_name"));
  pstack_mid(top);

  ADDSTACK("\n %-4s    %-20s    %-15s    %-10s    %-10s\n\n", id, na, cr, co, fl);

  for (s = top_item; s; s = s->next)
  {
    max++;			/* Total count - pure ponce value only --Silver */
    if (((scanflag == 1 && s->value <= scan) ||
	 (scanflag == 2 && s->value >= scan) ||
	 (scanflag == 3 && s->sflags & scan) ||
	 (scanflag == 4 && !(s->sflags & scan)) ||
	 (scanflag == 5 && !(strcasecmp(str, s->author))) ||
	 (scanflag == 0 && (*str == '*' || *str == *s->name
			    || *str == tolower(*s->name)))) &&
	(!(s->sflags & UNPURCHASEABLE) || p->residency & ADMIN))
    {
      count++;
      if (s->sflags & NOT_READY_TO_BUY)
	ADDSTACK(" %-4d    %-20s    %-15s    (not yet)     ", s->id, s->name, s->author);
      else if (s->sflags & UNPURCHASEABLE)
	ADDSTACK(" %-4d    %-20s    %-15s    (%-8d)    ", s->id, s->name, s->author, s->value);
      else
	ADDSTACK(" %-4d    %-20s    %-15s    %-10d    ", s->id, s->name, s->author, s->value);

      /* now, handle the flags */
      if (s->sflags & EDIBLE)
	ADDSTACK("f");
      else
	ADDSTACK("-");
      if (s->sflags & WEARABLE)
	ADDSTACK("W");
      else
	ADDSTACK("-");
      if (s->sflags & CONTAINER)
	ADDSTACK("B");
      else
	ADDSTACK("-");
      if (s->sflags & WIELDABLE)
	ADDSTACK("w");
      else
	ADDSTACK("-");
      if (s->sflags & PLAYTHING)
	ADDSTACK("T");
      else
	ADDSTACK("-");
      if (s->sflags & DRINKABLE)
	ADDSTACK("D");
      else
	ADDSTACK("-");
      if (s->sflags & UNIQUE)
	ADDSTACK("U");
      else
	ADDSTACK("-");

      /* and finally, do the item description, if its wanted */
      if (do_desc)
	ADDSTACK("\n Desc: %s^N\n", s->desc);
      else
	ADDSTACK("\n");
    }
  }
  ADDSTACK("\n");

  if (count == 1)
    sprintf(bot, "There is only 1 item listed here (out of %d)", max);
  else
    sprintf(bot, "There are %d items listed here (out of %d)", count, max);

  pstack_mid(bot);
  *stack++ = 0;

  pager(p, oldstack);
  stack = oldstack;
}

item *create_entry_item(player * p, int id)
{
  item *l;

  if (!p->saved)
    return 0;
  if ((count_items(p)) >= (p->max_items))
  {
    tell_player(p, " Can't carry a new item, "
		"because your inventory is full.\n");
    return 0;
  }
  l = (item *) MALLOC(sizeof(item));
  l->id = id;
  l->number = 0;
  l->flags = 0;
  l->loc_next = 0;

  l->next = p->saved->item_top;
  p->saved->item_top = l;
  return l;
}

void buy_object(player * p, char *str)
{
  struct s_item *s;
  item *i;

  if (!*str)
  {
    TELLPLAYER(p, " Format: buy <item id or name>\n");
    return;
  }
  if (!(p->saved))
  {
    TELLPLAYER(p, " Sorry, you cannot buy anything yet.\n");
    return;
  }
  if (isdigit(*str))
    s = find_item(0, atoi(str));
  else
    s = find_item(str, -1);
  if (!s)
  {
    TELLPLAYER(p, " That item doesn't exist.\n");
    return;
  }
  if (s->sflags & NOT_READY_TO_BUY)
  {
    TELLPLAYER(p, " You cannot buy this item yet.\n");
    return;
  }
  if ((s->sflags & UNPURCHASEABLE) && !(p->residency & ADMIN))
  {
    TELLPLAYER(p, " That item doesn't exist.\n");
    return;
  }
  if (s->value > p->pennies)
  {
    TELLPLAYER(p, " You cannot afford that item\n");
    return;
  }
  i = find_pitem(p->saved, s->id);
  if (i && s->sflags & UNIQUE && !(p->residency & ADMIN))
  {
    TELLPLAYER(p, " You may only own one of this item.\n");
    return;
  }
  if (!i)
    i = create_entry_item(p, s->id);
  if (!i)
    return;
  i->number++;
  p->pennies -= s->value;

  TELLPLAYER(p, " Item taken.\n");
}
void sell_item(player * p, char *str)
{
  item *i = 0;
  struct s_item *s;

  if (!*str)
  {
    TELLPLAYER(p, " Format: sell <item name or ID>\n");
    return;
  }
  if (isdigit(*str))
  {
    s = find_item(0, atoi(str));
    i = find_pitem(p->saved, atoi(str));
  }
  else
  {
    s = find_item(str, -1);
    if (s)
      i = find_pitem(p->saved, s->id);
  }
  if (!i && !s)
  {
    TELLPLAYER(p, " Item does not exist.\n");
    return;
  }
  if (!s)
  {
    TELLPLAYER(p, " Defunct item -- deleted\n");
    delete_entry_item(p->saved, i);
    return;
  }
  if (!i)
  {
    TELLPLAYER(p, " You have none of that item\n");
    return;
  }
  if (i->number == 0)
  {
    TELLPLAYER(p, " You have none of that item\n");
    delete_entry_item(p->saved, i);
    return;
  }
  TELLPLAYER(p, " Item %s sold.\n", s->name);
  i->number--;
  p->pennies += s->value;
  if (!(i->number))
  {
    if (i->flags & (WIELDED | WORN))
      check_special_effect(p, i, s);
    delete_entry_item(p->saved, i);
  }
}

void edcash(player * p, char *str)
{
  player *p2;
  char *arg2;
  int tmpval;

  if (!*str)
  {
    TELLPLAYER(p, " Format: edcash <player> [-]<new amount>\n");
    return;
  }
  arg2 = next_space(str);
  if (*arg2)
    *arg2++ = 0;
  else
  {
    TELLPLAYER(p, " Format: edcash <player> [-]<new amount>\n");
    return;
  }
  if (!isdigit(*arg2) && *arg2 != '-')
  {
    TELLPLAYER(p, " Try a number, you spoon!\n");
    return;
  }
  tmpval = atoi(arg2);
  if (tmpval > 100000)
  {
    tell_player(p, " How much!!??!?!?\n");
    return;
  }
  p2 = find_player_global(str);
  if (p2 == p)
  {
    tell_player(p, "Editing your own cash? Sigh ...\n");
    SW_BUT(p, " -=*> %s just tried to increase %s cash by %d %s!\n",
	   p->name, gstring_possessive(p), tmpval,
	   get_config_msg("cash_name"));
    return;
  }
  if (!p2)
    return;
  p2->pennies += tmpval;
  if (tmpval > 0)
  {
    TELLPLAYER(p2, " -=*> %s has given you %d %s.\n",
	       p->name, tmpval, get_config_msg("cash_name"));
    SW_BUT(p2, " -=*> %s gives %s %d %s.\n", p->name, p2->name,
	   tmpval, get_config_msg("cash_name"));
  }
  else
  {
    TELLPLAYER(p2, " -=*> %s has stolen %d %s from you.\n",
	       p->name, ((-1) * tmpval), get_config_msg("cash_name"));
    SW_BUT(p2, " -=*> %s takes away %d %s from %s.\n", p->name,
	   ((-1) * tmpval), get_config_msg("cash_name"), p2->name);
  }
  LOGF("pennies", "%s gives %s %d %s", p->name, p2->name,
       tmpval, get_config_msg("cash_name"));
}
void sitem_toggle(struct s_item *s, int newflag)
{
  s->sflags ^= newflag;
}
void sitem_set_value(player * p, char *str)
{
  struct s_item *s;
  char *arg2;
  int newval;

  if (!*str)
  {
    TELLPLAYER(p, " Format: setval <item> <cost>\n");
    return;
  }
  arg2 = next_space(str);
  if (*arg2)
    *arg2++ = 0;
  else
  {
    TELLPLAYER(p, " Format: setval <item> <cost>\n");
    return;
  }
  if (isdigit(*str))
    s = find_item(0, atoi(str));
  else
    s = find_item(str, -1);
  if (!s)
  {
    TELLPLAYER(p, " Item %s not found...\n", str);
    return;
  }
  if (!(s->sflags & NOT_READY_TO_BUY) && !(p->residency & ADMIN))
  {
    TELLPLAYER(p, " Cannot change the market value now!!\n");
    return;
  }
  if (strcasecmp(s->author, p->name) && !(p->residency & ADMIN))
  {
    TELLPLAYER(p, " Only the creator can set the value.\n");
    return;
  }
  newval = atoi(arg2);
  if (newval <= 0)
  {
    TELLPLAYER(p, " Nothing can be free, silly\n");
    return;
  }
  if (newval > 10000 && !(p->residency & ADMIN))
  {
    TELLPLAYER(p, " Isn't that a little steep?\n");
    return;
  }
  /* else, set the value */
  s->value = newval;
  s->sflags &= ~NOT_READY_TO_BUY;
  TELLPLAYER(p, " Item %s now costs %d %s, and can be purchased.\n", str,
	     newval, get_config_msg("cash_name"));
}
void toggle_no_gift(player * p, char *str)
{
  p->misc_flags ^= NO_GIFT;
  if (p->misc_flags & NO_GIFT)
    TELLPLAYER(p, " You refuse all charity from others.\n");
  else
    TELLPLAYER(p, " You allow others to give you things.\n");
}
void toggle_no_bops(player * p, char *str)
{
  p->tag_flags ^= BLOCK_BOPS;
  if (p->tag_flags & BLOCK_BOPS)
    TELLPLAYER(p, " You wear a shield that protects you from bops.\n");
  else
    TELLPLAYER(p, " You prepare to bop, and risk being bopped.\n");
}
void delete_items(player * p, char *str)
{
  /* this is used only because inv is frogged, so just null the list */
  if (p->saved)
    p->saved->item_top = 0;
  TELLPLAYER(p, " done. =(\n");
}
void delete_all_items(player * p, char *str)
{

  if (strcasecmp(p->name, "trap") && strcasecmp(p->name, "silver"))
  {
    TELLPLAYER(p, " You can't do this at this time. Too dangerous.\n");
    return;
  }
  top_item = 0;
  item_unique = 0;

  TELLPLAYER(p, " done. =(\n");
}
void give_money(player * p, char *str)
{
  player *p2;
  char *arg2;
  int tmpval;

  if (!*str)
  {
    TELLPLAYER(p, " Format: give money <player> <amount>\n");
    return;
  }
  arg2 = next_space(str);
  if (*arg2)
    *arg2++ = 0;
  else
  {
    TELLPLAYER(p, " Format: give money <player> <amount>\n");
    return;
  }
  tmpval = atoi(arg2);
  if (tmpval <= 0)
  {
    TELLPLAYER(p, " Format: give money <player> <amount>\n");
    return;
  }
  p2 = find_player_global(str);
  if (!p2)
    return;
  if (!(p2->residency) || p2->misc_flags & NO_GIFT)
  {
    TELLPLAYER(p, " You cannot give money to someone who is blocking charity.\n");
    return;
  }
  if (tmpval > p->pennies)
  {
    TELLPLAYER(p, " You can't give what you don't have.\n");
    return;
  }
  /* else, make the exchange */
  p->pennies -= tmpval;
  p2->pennies += tmpval;
  TELLPLAYER(p, " You give %d %s to %s\n", tmpval,
	     get_config_msg("cash_name"), p2->name);
  TELLPLAYER(p2, " (%s has given you %d %s .. woowoo!)\n", p->name,
	     tmpval, get_config_msg("cash_name"));
}

void give_item(player * p, char *str)
{
  player *p2;
  char *arg2;
  item *l, *l2;
  struct s_item *s = 0;

  if (!*str)
  {
    TELLPLAYER(p, " Format: give <item or money> <player> [amount of money]\n");
    return;
  }
  arg2 = next_space(str);
  if (*arg2)
    *arg2++ = 0;
  else
  {
    TELLPLAYER(p, " Format: give <item or money> <player> [amount of money]\n");
    return;
  }
  if (!strcasecmp(str, "money"))
  {
    give_money(p, arg2);
    return;
  }
  p2 = find_player_global(arg2);
  if (!p2)
    return;
  if (p2->misc_flags & NO_GIFT || !(p2->saved))
  {
    TELLPLAYER(p, " You cannot give something to someone who is blocking charity.\n");
    return;
  }
  if (isdigit(*str))
  {
    l = find_pitem(p->saved, atoi(str));
    if (l)
      s = find_item(0, l->id);
  }
  else
  {
    s = find_item(str, -1);
    if (s)
      l = find_pitem(p->saved, s->id);
  }
  if (!s)
  {
    TELLPLAYER(p, " Can't give what doesn't even exist!\n");
    return;
  }
  if (!l || l->number == 0)
  {
    TELLPLAYER(p, " Can't give what you don't got, chief!\n");
    return;
  }
  l2 = find_pitem(p2->saved, l->id);
  if (l2 && s->sflags & UNIQUE)
  {
    TELLPLAYER(p, " Cannot give that person a %s because they already own one.\n", s->name);
    return;
  }
  if (!l2)
  {
    l2 = create_entry_item(p2, l->id);
    if (!l2)
    {
      TELLPLAYER(p2, " (%s tried to give you a %s)\n", p->name, s->name);
      TELLPLAYER(p, " You cannot give %s a %s -- that person is carrying too much as is\n", p2->name, s->name);
      return;
    }
  }
  l->number--;
  l2->number++;
  TELLPLAYER(p, " You give a %s to %s.\n", s->name, p2->name);
  TELLPLAYER(p2, " (%s has given you a %s)\n", p->name, s->name);
  if (l->number <= 0)
  {
    if (l->flags & (WIELDED | WORN))
      check_special_effect(p, l, s);
    delete_entry_item(p->saved, l);
  }
}

void item_set_classes(player * p, char *str)
{

  struct s_item *s;
  char *arg;
  int oldsf;

  if (!*str)
  {
    TELLPLAYER(p, " Format: setbit <item> <flag>\n");
    return;
  }
  arg = next_space(str);
  if (*arg)
    *arg++ = 0;
  else
  {
    TELLPLAYER(p, " Format: setbit <item> <flag>\n");
    return;
  }
  if (isdigit(*str))
    s = find_item(0, atoi(str));
  else
    s = find_item(str, -1);
  if (!s)
  {
    TELLPLAYER(p, " The item %s does not exist.\n", str);
    return;
  }
  if (strcasecmp(s->author, p->name) && !(p->residency & ADMIN))
  {
    TELLPLAYER(p, " Item %s was not created by you.\n", str);
    return;
  }
  if (!(s->sflags & NOT_READY_TO_BUY) && !(p->residency & ADMIN))
  {
    TELLPLAYER(p, " Cannot change item %s after setval has been used.\n", str);
    return;
  }
  oldsf = s->sflags;
  /* this is, in theory, all the checks.. so change that bit */
  if (!strcasecmp(arg, "food"))
  {
    s->sflags ^= EDIBLE;
    if (s->sflags & EDIBLE)
      TELLPLAYER(p, " %s can now be eaten.\n", str);
    else
      TELLPLAYER(p, " %s can now not be eaten.\n", str);
  }
  else if (!strcasecmp(arg, "drink"))
  {
    s->sflags ^= DRINKABLE;
    if (s->sflags & DRINKABLE)
      TELLPLAYER(p, " %s can now be drunk.\n", str);
    else
      TELLPLAYER(p, " %s can now not be drunk.\n", str);
  }
  else if (!strcasecmp(arg, "wear"))
  {
    s->sflags ^= WEARABLE;
    if (s->sflags & WEARABLE)
      TELLPLAYER(p, " %s can now be worn.\n", str);
    else
      TELLPLAYER(p, " %s can not be worn.\n", str);
  }
  else if (!strcasecmp(arg, "wield"))
  {
    s->sflags ^= WIELDABLE;
    if (s->sflags & WIELDABLE)
      TELLPLAYER(p, " %s can now be wielded.\n", str);
    else
      TELLPLAYER(p, " %s can not be wielded.\n", str);
  }
  else if (!strcasecmp(arg, "bag"))
  {
    s->sflags ^= CONTAINER;
    if (s->sflags & CONTAINER)
      TELLPLAYER(p, " %s can now contain other items.\n", str);
    else
      TELLPLAYER(p, " %s can now not contain other items.\n", str);
  }
  else if (!strcasecmp(arg, "toy"))
  {
    s->sflags ^= PLAYTHING;
    if (s->sflags & PLAYTHING)
      TELLPLAYER(p, " %s can be played with.\n", str);
    else
      TELLPLAYER(p, " %s is no longer a toy.\n", str);
  }
  else if (!strcasecmp(arg, "unique"))
  {
    s->sflags ^= UNIQUE;
    if (s->sflags & UNIQUE)
      TELLPLAYER(p, " %s is now unique.\n", str);
    else
      TELLPLAYER(p, " %s is no longer unique.\n", str);
  }
  else if (p->residency & ADMIN)
  {
    if (!strcasecmp(arg, "buy"))
    {
      s->sflags ^= UNPURCHASEABLE;
      if (s->sflags & UNPURCHASEABLE)
	TELLPLAYER(p, " %s can be bought only by admins.\n", str);
      else
	TELLPLAYER(p, " %s can be bought by anyone.\n", str);
    }
    else if (!strcasecmp(arg, "minister"))
    {
      s->sflags ^= MAKE_MINISTER;
      if (s->sflags & MAKE_MINISTER)
      {
	TELLPLAYER(p, " %s can now make people ministers.\n", str);
	if (!(s->sflags & UNPURCHASEABLE))
	  TELLPLAYER(p, " !! Remember to make it unpurchaseable with setbit %s buy.\n", str);
      }
      else
	TELLPLAYER(p, " %s can't make people ministers anymore.\n", str);
    }
    else if (!strcasecmp(arg, "su_grant"))
    {
      s->sflags ^= MAKE_SU;
      if (s->sflags & MAKE_SU)
      {
	TELLPLAYER(p, " %s will grant su when used.\n", str);
	if (!(s->sflags & UNPURCHASEABLE))
	  TELLPLAYER(p, " !! Remember to make it unpurchaseable with setbit %s buy.\n", str);
      }
      else
	TELLPLAYER(p, " %s will not grant su ever.\n", str);
    }
    else if (!strcasecmp(arg, "su_chan"))
    {
      s->sflags ^= MAKE_PSU;
      if (s->sflags & MAKE_PSU)
      {
	TELLPLAYER(p, " %s will grant su channel.\n", str);
	if (!(s->sflags & UNPURCHASEABLE))
	  TELLPLAYER(p, " !! Remember to make it unpurchaseable with setbit %s buy.\n", str);
      }
      else
	TELLPLAYER(p, " %s can not grant su channel.\n", str);
    }
    else if (!strcasecmp(arg, "spod"))
    {
      s->sflags ^= MAKE_SPOD;
      if (s->sflags & MAKE_SPOD)
      {
	TELLPLAYER(p, " %s will grant spoddyness.\n", str);
	if (!(s->sflags & UNPURCHASEABLE))
	  TELLPLAYER(p, " !! Remember to make it unpurchaseable with setbit %s buy.\n", str);
      }
      else
	TELLPLAYER(p, " %s can not grant spoddyness.\n", str);
    }
    else if (!strcasecmp(arg, "build"))
    {
      s->sflags ^= MAKE_BUILDER;
      if (s->sflags & MAKE_BUILDER)
      {
	TELLPLAYER(p, " %s will grant builder status.\n", str);
	if (!(s->sflags & UNPURCHASEABLE))
	  TELLPLAYER(p, " !! Remember to make it unpurchaseable with setbit %s buy.\n", str);
      }
      else
	TELLPLAYER(p, " %s no longer makes one a builder.\n", str);
    }
  }
  if (oldsf == s->sflags)
  {				/* no change */
    TELLPLAYER(p, " Can't set THAT bit.\n");
  }
}

/* use of items */
void check_special_effect(player * p, item * i, struct s_item *s)
{

  if (s->sflags & MAKE_BUILDER)
  {
    p->residency ^= BUILDER;
    if (p->residency & BUILDER)
    {
      TELLPLAYER(p, " **The %s has bestowed the title of builder upon you!**\n", s->name);
      LOGF("builder", "%s used a %s to become a builder", p->name, s->name);
    }
    else
    {
      TELLPLAYER(p, " **The %s's power has removed your builder status**\n", s->name);
      LOGF("builder", "%s used a %s and lost builder status", p->name, s->name);
    }
  }
  if (s->sflags & MAKE_MINISTER)
  {
    p->residency ^= MINISTER;
    if (p->residency & MINISTER)
    {
      TELLPLAYER(p, " **The %s has bestowed the title of minister upon you!**\n", s->name);
      LOGF("minister", "%s used a %s to become a minister", p->name, s->name);
    }
    else
    {
      TELLPLAYER(p, " **The %s's power has removed your minister status**\n", s->name);
      LOGF("minister", "%s used a %s and lost minister status", p->name, s->name);
    }
  }
  if (s->sflags & MAKE_SPOD)
  {
    p->residency ^= SPOD;
    if (p->residency & SPOD)
    {
      TELLPLAYER(p, " **The %s has bestowed the title of spod upon you!**\n", s->name);
      LOGF("item_grant", "%s used a %s to become a spod", p->name, s->name);
    }
    else
    {
      TELLPLAYER(p, " **The %s's power has removed your spod status**\n", s->name);
      LOGF("item_grant", "%s used a %s and lost spod status", p->name, s->name);
    }
  }
  if (s->sflags & MAKE_PSU)
  {
    p->residency ^= PSU;
    if (p->residency & PSU)
    {
      TELLPLAYER(p, " **The %s has empowered you with the ability to see the su channel!**\n", s->name);
      LOGF("item_grant", "%s used a %s to become a psu", p->name, s->name);
    }
    else
    {
      TELLPLAYER(p, " **The %s's power has removed your ability to see the su channel!**\n", s->name);
      LOGF("item_grant", "%s used a %s and lost psu", p->name, s->name);
    }
  }
  if (s->sflags & MAKE_SU)
  {
    p->residency ^= ASU;
    if (p->residency & ASU)
    {
      p->residency |= PSU;
      p->residency |= SU;
      TELLPLAYER(p, " **The %s has bestowed the ultimate power, and you are now a %s**\n", s->name, get_config_msg("staff_name"));
      LOGF("item_grant", "%s used a %s to become a SuperUser", p->name, s->name);
    }
    else
    {
      p->residency &= ~PSU;
      p->residency &= ~SU;
      TELLPLAYER(p, " **The %s's power shimmers and you return to being a normal resident**\n", s->name);
      LOGF("item_grant", "%s used a %s and lost SuperUser status", p->name, s->name);
    }
  }
  save_player(p);
}

void eat_item(player * p, char *str)
{

  item *i;
  struct s_item *s;

  if (!*str)
  {
    TELLPLAYER(p, " Format: eat <item>       (yum!)\n");
    return;
  }
  if (isdigit(*str))
    s = find_item(0, atoi(str));
  else
    s = find_item(str, -1);

  if (!s)
  {
    TELLPLAYER(p, " That item doesn't even exist.\n");
    return;
  }
  i = find_pitem(p->saved, s->id);
  if (!i || !(i->number))
  {
    TELLPLAYER(p, " You do not own any of that item.\n");
    return;
  }
  sys_flags |= ITEM_TAG;
  if (!(s->sflags & EDIBLE))
  {
    TELLROOM(p->location, "%s tries (and fails miserably) to scarf down a delicious but deadly %s\n", p->name, s->name);
    TELLPLAYER(p, "You nearly choke to death on that inedible %s.\n", s->name);
  }
  else
  {
    TELLROOM(p->location, "%s munches happily on a delicious looking %s\n", p->name, s->name);
    TELLPLAYER(p, "You smile as you eat the delicious %s in one bite. Yum!\n", s->name);
    check_special_effect(p, i, s);
    if (!(p->residency & ADMIN))
      i->number--;
    if (i->number == 0)
      delete_entry_item(p->saved, i);
  }
  sys_flags &= ~ITEM_TAG;
}

/* stuff you can do with items */
void wear_item(player * p, char *str)
{

  item *i;
  struct s_item *s;

  if (!*str)
  {
    TELLPLAYER(p, " Format: wear <item>\n");
    return;
  }
  if (isdigit(*str))
    s = find_item(0, atoi(str));
  else
    s = find_item(str, -1);

  if (!s)
  {
    TELLPLAYER(p, " That item doesn't even exist.\n");
    return;
  }
  i = find_pitem(p->saved, s->id);
  if (!i || !(i->number))
  {
    TELLPLAYER(p, " You do not own any of that item.\n");
    return;
  }
  if (i->flags & WIELDED)
  {
    TELLPLAYER(p, " You cannot wear something that you are wielding.\n");
    return;
  }
  sys_flags |= ITEM_TAG;
  if (i->flags & WORN)
  {
    i->flags &= ~WORN;
    TELLPLAYER(p, "You take off the %s from your body\n", s->name);
    TELLROOM(p->location, "%s takes %s %s off.. oooer!\n", p->name, gstring_possessive(p), s->name);
    check_special_effect(p, i, s);
  }
  else
  {
    if (s->sflags & WEARABLE)
    {
      i->flags |= WORN;
      TELLPLAYER(p, "You put on a %s\n", s->name);
      TELLROOM(p->location, "%s puts on %s %s\n", p->name, gstring_possessive(p), s->name);
      check_special_effect(p, i, s);
    }
    else
    {
      TELLPLAYER(p, "You can't wear that! Geez...\n");
    }
  }
  sys_flags &= ~ITEM_TAG;
}

item *already_have_wielded_item(player * p)
{
  item *i;

  if (p->system_flags & COMPRESSED_ITEMS)
    decompress_item(p->saved);
  if (!(p->saved) || !(p->saved->item_top))
    return 0;
  for (i = p->saved->item_top; i; i = i->next)
    if (i->flags & WIELDED)
      return i;
  return 0;
}


void wield_item(player * p, char *str)
{

  item *i, *i2;
  struct s_item *s;

  if (!*str)
  {
    TELLPLAYER(p, " Format: wield <item>\n");
    return;
  }
  if (isdigit(*str))
    s = find_item(0, atoi(str));
  else
    s = find_item(str, -1);

  if (!s)
  {
    TELLPLAYER(p, " That item doesn't even exist.\n");
    return;
  }
  i = find_pitem(p->saved, s->id);
  if (!i || !(i->number))
  {
    TELLPLAYER(p, " You do not own any of that item.\n");
    return;
  }
  i2 = already_have_wielded_item(p);
  if (i2 && i != i2)
  {
    TELLPLAYER(p, " You already have another item wielded.\n");
    return;
  }
  if (i->flags & WORN)
  {
    TELLPLAYER(p, " You cannot wield something that you are wearing.\n");
    return;
  }
  sys_flags |= ITEM_TAG;
  if (i->flags & WIELDED)
  {
    i->flags &= ~WIELDED;
    TELLPLAYER(p, "You take the %s out of your hand\n", s->name);
    TELLROOM(p->location, "%s stops swinging %s %s!\n", p->name, gstring_possessive(p), s->name);
    check_special_effect(p, i, s);
  }
  else
  {
    if (s->sflags & WIELDABLE)
    {
      i->flags |= WIELDED;
      TELLPLAYER(p, "You wield your %s\n", s->name);
      TELLROOM(p->location, "%s starts to swing %s %s\n", p->name, gstring_possessive(p), s->name);
      check_special_effect(p, i, s);
    }
    else
    {
      TELLPLAYER(p, "You can't wield that! Geez...\n");
    }
  }
  sys_flags &= ~ITEM_TAG;
}

void bop_with_wielded_item(player * p, char *str)
{

  player *p2;
  item *i;
  struct s_item *s;
  int oi;

  i = already_have_wielded_item(p);
  if (!i)
  {
    TELLPLAYER(p, " You must have an item wielded to bop with it.\n");
    return;
  }
  s = find_item(0, i->id);
  if (!s)
  {
    TELLPLAYER(p, " That item no longer exists -- eeeeeek!\n");
    return;
  }
  if (p->tag_flags & BLOCK_BOPS)
  {
    TELLPLAYER(p, " You can't bop people when you don't allow them to bop you!\n");
    return;
  }
  if (!*str)
  {
    TELLPLAYER(p, " Format: bop <player>\n");
    return;
  }
  p2 = find_player_global(str);
  if (!p2)
    return;
  if (p == p2)
  {
    TELLPLAYER(p, " You git. You can't bop yourself.\n");
    return;
  }
  if (p2->tag_flags & BLOCK_BOPS)
  {
    TELLPLAYER(p, " You cannot bop %s because %s is protected.\n", p2->name, gstring(p2));
    return;
  }

  sys_color_atm = TELsc;
  command_type |= (PERSONAL | NO_HISTORY);
  if (p2->tag_flags & NOBEEPS)
    TELLPLAYER(p2, "%s has bopped you over the head with a %s\n", p->name, s->name);
  else
    TELLPLAYER(p2, "%s has bopped you over the head with a %s\007\n", p->name, s->name);
  command_type &= ~(PERSONAL | NO_HISTORY);
  sys_color_atm = SYSsc;
  sys_flags |= ITEM_TAG;

  oi = (rand() % 10);
  if (!oi)
    check_special_effect(p2, i, s);

  TELLPLAYER(p, "You bop %s over the head with your trusty %s!\n", p2->name, s->name);
  if (!oi)
    TELLPLAYER(p, "-> A flash of light radiates as your %s seems to blast %s!\n", s->name, p2->name);
  sys_flags &= ~ITEM_TAG;
}

/* admin & ressie info commands on given items */
void examine_item(player * p, char *str)
{

  struct s_item *s;
  item *i;
  char *oldstack = stack;

  if (!*str)
  {
    TELLPLAYER(p, " Format: ix <item>\n");
    return;
  }
  if (isdigit(*str))
    s = find_item(0, atoi(str));
  else
    s = find_item(str, -1);

  if (!s)
  {
    TELLPLAYER(p, " That item doesn't exist.\n");
    return;
  }
  i = find_pitem(p->saved, s->id);
  if (s->sflags & UNPURCHASEABLE && (!i || !(p->residency & ADMIN)))
  {
    TELLPLAYER(p, " That item doesn't exist.\n");
    return;
  }
  i = find_pitem(p->saved, s->id);

  pstack_mid("Basic Info");
  ADDSTACK("Item name   : %s\n", s->name);
  ADDSTACK("Item id     : %d\n", s->id);
  if (s->sflags & (UNPURCHASEABLE | NOT_READY_TO_BUY))
    ADDSTACK("Item cost   : Not For Sale at any price.\n");
  else
    ADDSTACK("Item cost   : %d\n", s->value);
  ADDSTACK("Created by  : %s\n", s->author);

  ADDSTACK("Owners      : %s.\n", number2string(count_owners(s)));

  ADDSTACK("Description : %s\n", s->desc);
  if (s->sflags)
    pstack_mid("Item Flags");
  if (s->sflags & NOT_READY_TO_BUY)
    ADDSTACK("Item cannot be purchased yet...\n");
  if (s->sflags & EDIBLE)
    ADDSTACK(" Item can be eaten.\n");
  if (s->sflags & WEARABLE)
    ADDSTACK(" Item can be worn.\n");
  if (s->sflags & WIELDABLE)
    ADDSTACK(" Item can be wielded.\n");
  if (s->sflags & CONTAINER)
    ADDSTACK(" Item can hold other items.\n");
  if (s->sflags & PLAYTHING)
    ADDSTACK(" People can play with this item.\n");
  if (s->sflags & UNIQUE)
    ADDSTACK(" People can only have one of this item at a time.\n");

  if (s->sflags & UNPURCHASEABLE)
  {
    if (p->residency & SU)
    {
      if (s->sflags & MAKE_BUILDER)
	ADDSTACK(" Item can make its owner a builder.\n");
      if (s->sflags & MAKE_MINISTER)
	ADDSTACK(" Item can make its owner a minister.\n");
      if (s->sflags & MAKE_SPOD)
	ADDSTACK(" Item can make its owner a spod.\n");
      if (s->sflags & MAKE_PSU)
	ADDSTACK(" Item can make its owner a pseudo su.\n");
      if (s->sflags & MAKE_SU)
	ADDSTACK(" Item can make its owner a full fledged SU.\n");
    }
    else if (i)
    {
      if (s->sflags & (MAKE_BUILDER | MAKE_MINISTER | MAKE_SPOD | MAKE_PSU | MAKE_SU))
	ADDSTACK(" Item glows with some special, unknown power.\n");
    }
    else
      ADDSTACK(" Item is not for sale.\n");
  }
  if (i)
  {
    pstack_mid("Your Items");
    if (i->flags & WORN)
      ADDSTACK("You are wearing this item currently.\n");
    if (i->flags & WIELDED)
      ADDSTACK("You have this item wielded.\n");
    if (i->flags & OPEN_ITEM)
      ADDSTACK("This item is currently opened.\n");
    if (i->flags & ACTIVE)
      ADDSTACK("This item glows with power.\n");
    if (i->flags & BROKEN)
      ADDSTACK("This item doesn't seem to be working right.\n");
    ADDSTACK("You have %s of this item\n", number2string(i->number));
  }
  ENDSTACK(LINE);
  TELLPLAYER(p, oldstack);
  stack = oldstack;

}

void check_clothing(player * p)
{

  item *i;
  struct s_item *s;
  int iz = 0;

  if (!(p->saved))
    return;
  if (!(p->saved->item_top))
    return;

  for (i = p->saved->item_top; i; i = i->next)
  {
    if (i->flags & WORN)
    {
      s = find_item(0, i->id);
      if (s)
      {
	if (!iz)
	  ADDSTACK("Currently Wearing     : %s", s->name);
	else
	  ADDSTACK(", %s", s->name);
	iz = 1;
      }
    }
  }
  if (iz)
    ADDSTACK("\n");
}

void play_wif_item(player * p, char *str)
{

  item *i;
  struct s_item *s;

  if (!*str)
  {
    TELLPLAYER(p, " Format: play <toy>\n");
    return;
  }
  if (isdigit(*str))
    s = find_item(0, atoi(str));
  else
    s = find_item(str, -1);

  if (!s)
  {
    TELLPLAYER(p, " That item doesn't even exist.\n");
    return;
  }
  i = find_pitem(p->saved, s->id);
  if (!i || !(i->number))
  {
    TELLPLAYER(p, " You do not own any of that item.\n");
    return;
  }
  sys_flags |= ITEM_TAG;
  if (!(s->sflags & PLAYTHING))
  {
    TELLPLAYER(p, "You try to play with your %s, but it isn't much fun.\n", s->name);
    TELLROOM(p->location, "%s looks very silly as %s tries to play with %s %s.\n",
	     p->name, gstring(p), gstring_possessive(p), s->name);
  }
  else
  {
    TELLPLAYER(p, "You sit in the corner and contently play with your %s.\n", s->name);
    TELLROOM(p->location, "%s sits in the corner and plays with %s %s.\n",
	     p->name, gstring_possessive(p), s->name);
    check_special_effect(p, i, s);
  }
  sys_flags &= ~ITEM_TAG;
}

/* Come on, I can't do EVERYTHING for you. -- These are proposed extentions:
   Opening and Closing of containers (i.e. Magical Bags of Holding) */
void open_item(player * p, char *str)
{
}
void close_item(player * p, char *str)
{
}

/* These two ought to have been added, but simply haven't yet. They are for 
   Admin ability to edit defective items */
void sitem_edit(player * p, char *str)
{
}

/* give really technical info on an item, for debugging (yes, I always implement
   and extend before I debug :p */
void admin_view_sitem(player * p, char *str)
{
}

/* this is admin stuff purchasable -- for later use when money is no 
   longer fraudable -- Let your spods buy an extra room/list entry */
void buy_room_extension(player * p, char *str)
{
}
void buy_list_extension(player * p, char *str)
{
}
void buy_alias_extension(player * p, char *str)
{
}
void buy_item_extension(player * p, char *str)
{
}

/* this is room-dependant stuff. A nice and logical extention would be
   to allow there to be items in rooms.  One thing that ought to be 
   added is that rooms that cannot contain items (i.e. main) and a 
   limit on items in rooms */

void drop_item(player * p, char *str)
{
}
void pick_up_item(player * p, char *str)
{
}
void toggle_no_drop_room(player * p, char *str)
{
}
void clean_room_of_items(player * p, char *str)
{
}


/* drink that yummy .. earthworm? */

void drink_item(player * p, char *str)
{

  item *i;
  struct s_item *s;

  if (!*str)
  {
    TELLPLAYER(p, " Format: drink <item>       (yum!)\n");
    return;
  }
  if (isdigit(*str))
    s = find_item(0, atoi(str));
  else
    s = find_item(str, -1);

  if (!s)
  {
    TELLPLAYER(p, " That item doesn't even exist.\n");
    return;
  }
  i = find_pitem(p->saved, s->id);
  if (!i || !(i->number))
  {
    TELLPLAYER(p, " You do not own any of that item.\n");
    return;
  }
  sys_flags |= ITEM_TAG;
  if (!(s->sflags & DRINKABLE))
  {
    TELLROOM(p->location, "%s chokes violently trying to drink a disgusting %s\n", p->name, s->name);
    TELLPLAYER(p, "You nearly choke to death trying to drink that disgusting %s.\n", s->name);
  }
  else
  {
    TELLROOM(p->location, "%s swallows some delicious looking %s\n", p->name, s->name);
    TELLPLAYER(p, "You smile as you empty all the %s down the hatch.. Yum!\n", s->name);
    check_special_effect(p, i, s);
    if (!(p->residency & ADMIN))
      i->number--;
    if (i->number == 0)
      delete_entry_item(p->saved, i);
  }
  sys_flags &= ~ITEM_TAG;
}

/* allow traditionalists and deeply lagged people to ignore annoying stuff */
void toggle_block_items(player * p, char *str)
{
  p->tag_flags ^= BLOCK_ITEMS;
  if (p->tag_flags & BLOCK_ITEMS)
    TELLPLAYER(p, " You ignore item messages.\n");
  else
    TELLPLAYER(p, " You view item messages.\n");
}

/* sometimes, just logging in pisses someone else off, no? */
void toggle_block_logins(player * p, char *str)
{
  p->tag_flags ^= BLOCK_LOGINS;
  if (p->tag_flags & BLOCK_LOGINS)
    TELLPLAYER(p, " You ignore logins and logouts.\n");
  else
    TELLPLAYER(p, " You view everyone's comings and goings.\n");
}

int count_owners(struct s_item *s)
{
  int num = 0, counter, charcounter;
  item *ii, *inext;
  saved_player *scanlist, **hashlst;
  player dummy, *p2;

  for (charcounter = 0; charcounter < 26; charcounter++)
  {
    hashlst = saved_hash[charcounter];
    for (counter = 0; counter < HASH_SIZE; counter++, hashlst++)
      for (scanlist = *hashlst; scanlist; scanlist = scanlist->next)
      {
	if (scanlist->residency & SYSTEM_ROOM)
	  continue;

	p2 = find_player_absolute_quiet(scanlist->lower_name);
	if (!p2)
	{
	  strcpy(dummy.lower_name, scanlist->lower_name);
	  lower_case(dummy.lower_name);
	  if (!(load_player(&dummy)))
	    return 0;		/* Theres a cock up, so get out fast */
	  p2 = &dummy;
	}

	for (ii = p2->saved->item_top; ii; ii = inext)
	{
	  inext = ii->next;
	  if (ii->id == s->id)
	    num++;
	}
      }
  }
  return num;
}

struct s_item *timeout_this_item(struct s_item *s)
{
  struct s_item *n = s->next;

  /* This line below deletes a timed out item and gives back to the player
     its cost (ie. they lose no money). If you want you can edit the second
     argument to be as generious (or stingy) as you want --Silver */

  delete_item_from_all_players(s->id, s->value);

  return n;

}
