/*
 * Playground+ - spodlist.c v1.0
 * Spodlist code copyright (c) Richard Lawrence (Silver) 1998
 * ---------------------------------------------------------------------------
 *
 *  Permission granted for extraction and usage in a running talker if
 *  you place credit for it either in help credits or your talkers
 *  "version" command and mail me the talker addy so I can see :o)
 *
 *  THIS CODE IS LICENCED FOR DISTRIBUTION IN PG+ ONLY.
 *  You may not distribute this code in any form other than within the
 *  code release PG+. You may only distribute it in a successor of PG+
 *  with express consent in writing from Silver (Richard Lawrence)
 *  <silver@whiting.co.uk>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef BSDISH
#include <malloc.h>
#endif

#include "include/config.h"
#include "include/player.h"
#include "include/fix.h"
#include "include/proto.h"

struct spodlist
{
  char name[MAX_NAME];		/* players name */
  int login;			/* players login time */
  float spodpc;			/* Players spod percentage */
  struct spodlist *next;	/* pointer to next in list */
};

struct spodlist *spodlist_top = NULL;

/* Find a person in the spodlist and return their position in the spodlist */

int find_spodlist_position(char *str)
{
  struct spodlist *q;
  int position = 1;

  /* Search for name */

  for (q = spodlist_top; q != NULL; q = q->next)
    if (!strcasecmp(str, q->name))
      return position;
    else
      position++;

  return 0;			/* no name found so return 0 */
}

/* Return the number of people in the spodlist */

int people_in_spodlist(void)
{
  struct spodlist *q;
  int people = 0;

  q = spodlist_top;
  while (q != NULL)
  {
    people++;
    q = q->next;
  }

  return people;
}

/* Add a name to the spodlist */

void add_name_to_spodlist(char *name, int login, float pc)
{
  struct spodlist *cur, *prev, *new_node;

  new_node = malloc(sizeof(struct spodlist));
  if (new_node == NULL)		/* out of memory */
  {
    log("error", "Out of memory to malloc in spodlist.c");
    return;
  }

  new_node->login = login;
  strcpy(new_node->name, name);
  new_node->spodpc = pc;

  for (cur = spodlist_top, prev = NULL; cur != NULL && new_node->login < cur->login; prev = cur, cur = cur->next)
    ;
  new_node->next = cur;
  if (prev == NULL)
    spodlist_top = new_node;
  else
    prev->next = new_node;
}

/* Delete whole list.
   (I could update the spodlist struct each time it is run but the time
   taken to find an element and then update it is longer than just deleting
   the whole lot and re-doing the whole database - trust me, I tested it) */

void delete_spodlist(void)
{
  struct spodlist *curr, *next;

  for (curr = spodlist_top; curr != NULL; curr = next)
  {
    next = curr->next;
    free(curr);
  }

  spodlist_top = NULL;
}


/* Calculate the spodlist */

void calc_spodlist(void)
{
  saved_player *scanlist, **hlist;
  int counter, charcounter;
  player dummy, *p2;
  float spodpc;
  char chk[40] = "burgleflop";	/* Owing to a funny bug which randomly repeats */
  /* some peoples names - this was fudged ... erm I mean added :o) -- Silver */

  /* Delete the whole list */

  delete_spodlist();

  /* Save all players so times are accurate */

  for (p2 = flatlist_start; p2; p2 = p2->flat_next)
    save_player(p2);

  /* Store all players */

  for (charcounter = 0; charcounter < 26; charcounter++)
  {
    hlist = saved_hash[charcounter];
    for (counter = 0; counter < HASH_SIZE; counter++, hlist++)
      for (scanlist = *hlist; scanlist; scanlist = scanlist->next)
      {
	switch (scanlist->residency)
	{
	  case BANISHED:
	  case SYSTEM_ROOM:
	  case BANISHD:
	    break;
	  default:
	    strcpy(dummy.lower_name, scanlist->lower_name);
	    lower_case(dummy.lower_name);
	    load_player(&dummy);
	    p2 = &dummy;

	    /* Quick fudge ... */
	    if (p2->total_idle_time > p2->total_login)
	      p2->total_idle_time = 0;

	    /* If defined, don't put robots on the spodlist */

#ifdef ROBOTS
            if (strcasecmp(p2->name, chk) && !(p2->residency & ROBOT_PRIV))
#else
            if (strcasecmp(p2->name, chk) && !(p2->residency & BANISHD))
#endif
	    {
	      strcpy(chk, p2->name);
	      spodpc = (((float) p2->total_login) / (((float) (time(0) - p2->first_login_date)))) * 100;
	      if (spodpc > 100.0)
		spodpc = 100.0;
	      if (config_flags & cfUSETRUESPOD)
		add_name_to_spodlist(p2->name,
			   (p2->total_login - p2->total_idle_time), spodpc);
	      else
		add_name_to_spodlist(p2->name, p2->total_login, spodpc);
	    }
	}
      }
  }
}

void show_spodlist(player * p, char *str)
{
  char *oldstack = stack;
  int start_pos = 1, end_pos, listed, pos = 0, hilight = 0;
  char temp[70];
  struct spodlist *q;

  calc_spodlist();
  q = spodlist_top;

  listed = people_in_spodlist();

  if (*str)
  {
    /* We'll assume its a position they want. If its a name then we
       can easily overwrite it */
    hilight = atoi(str);

    if (hilight == 0)
    {
      if (!find_saved_player(str))
      {
	tell_player(p, " That name is not in the player files.\n");
	return;
      }
      else
	hilight = find_spodlist_position(str);
    }

    if (hilight > listed)
    {
      start_pos = listed - 12;
      hilight = 0;
    }
    else
    {
      start_pos = hilight - 6;
      if (start_pos < 1)
	start_pos = 1;
    }
  }
  else
  {
    hilight = find_spodlist_position(p->name);
    start_pos = hilight - 6;
    if (start_pos < 1)
      start_pos = 1;;
  }

  end_pos = start_pos + 12;
  if (end_pos > listed)
  {
    end_pos = listed;
    start_pos = end_pos - 12;
  }


  /* Create the page */

  sprintf(temp, "%s Spodlist", get_config_msg("talker_name"));
  pstack_mid(temp);

  for (pos = 1; pos <= end_pos && q != NULL; pos++, q = q->next)
  {
    if (pos >= start_pos)
    {
      if (pos == hilight)
      {
	sprintf(stack, "^H");
	stack = strchr(stack, 0);
      }
      if (q->spodpc == 100.0)
	sprintf(stack, "%4d. %-20s 100.00%%   %s^N\n", pos, q->name, word_time(q->login));
      else if (q->spodpc >= 10.0)
	sprintf(stack, "%4d. %-20s  %.2f%%   %s^N\n", pos, q->name, q->spodpc, word_time(q->login));
      else
	sprintf(stack, "%4d. %-20s   %.2f%%   %s^N\n", pos, q->name, q->spodpc, word_time(q->login));
      stack = strchr(stack, 0);
    }
  }

  if (start_pos < 1)
    start_pos = 1;

  sprintf(temp, "Positions %d to %d (out of %d residents)", start_pos, end_pos, listed);
  pstack_mid(temp);
  *stack++ = 0;
  pager(p, oldstack);
  stack = oldstack;
}

void spodlist_version(void)
{
  sprintf(stack, " -=*> Dynamic spodlist v1.0 (by Silver) enabled.\n");
  stack = strchr(stack, 0);
}
