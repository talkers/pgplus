/*
 * Playground+ - gag.c
 * Gag commands
 * ---------------------------------------------------------------------------
 */

#include <ctype.h>
#include <string.h>
#ifndef BSDISH
#include <malloc.h>
#else
#include <stdlib.h>
#endif
#include <fcntl.h>
#include <memory.h>

#include "include/config.h"
#include "include/player.h"
#include "include/fix.h"
#include "include/proto.h"

int count_gags(player * p)
{

  int cnt = 0;
  gag_entry *g;

  if (!(p->gag_top))
    return 0;

  else
    for (g = p->gag_top; g; g = g->next)
      cnt++;

  return cnt;
}

int match_gag(player * p, player * q)
{

  gag_entry *g;

  for (g = p->gag_top; g; g = g->next)
  {
    if (g->gagged == q)
      return 1;
  }
  return 0;
}

void create_gag(player * p, char *str)
{

  gag_entry *g;
  player *p2;

  if (!*str)
  {
    TELLPLAYER(p, " Format: gag <player>\n");
    return;
  }
  p2 = find_player_absolute_quiet(str);
  if (!p2)
  {
    TELLPLAYER(p, " That player isn't logged on at the moment.\n");
    return;
  }
  if (match_gag(p, p2))
  {
    TELLPLAYER(p, " You have already gagged %s.\n", p2->name);
    return;
  }
  /* create the entry */
  g = (gag_entry *) MALLOC(sizeof(gag_entry));
  if (!g)
  {
    log("error", "Failed to malloc in add_gag...ouch!");
    tell_player(p, " Ergs!  A lowlevel memory error occured, try again.\n");
    return;
  }

  g->gagged = p2;
  g->next = p->gag_top;
  p->gag_top = g;
  TELLPLAYER(p2, " (You have been gagged by %s...)\n", p->name);
  TELLPLAYER(p, " You gag %s.\n", p2->name);
}


void del_gag(player * p, char *str, int verbose)
{

  gag_entry *g, *gp;
  player *p2;

  if (!*str && verbose)
  {
    TELLPLAYER(p, " Format: ungag <player>\n");
    return;
  }
  p2 = find_player_absolute_quiet(str);
  if (!p2 && verbose)
  {
    TELLPLAYER(p, " That player isn't logged on at the moment.\n");
    return;
  }
  /* ok, delete the gag if it exists */
  if (!(p->gag_top))
  {
    TELLPLAYER(p, " You haven't got anyone gagged atm.\n");
    return;
  }
  if (p->gag_top->gagged == p2)
  {
    g = p->gag_top;
    p->gag_top = g->next;
    FREE(g);
    if (verbose)
      TELLPLAYER(p, " %s has been ungagged.\n", p2->name);
    return;
  }
  for (gp = p->gag_top, g = gp->next; g; gp = g, g = g->next)
  {

    if (g->gagged == p2)
    {
      /* FREE() the gag, and relink stuff */
      gp->next = g->next;
      FREE(g);
      if (verbose)
	TELLPLAYER(p, " %s has been ungagged.\n", p2->name);
      return;
    }
  }
  if (verbose)
    TELLPLAYER(p, " But you never gagged %s!\n", p2->name);
}

void delete_gag(player * p, char *str)
{

  del_gag(p, str, 1);
}

void list_all_gags(player * p, char *str)
{

  char *oldstack = stack;
  gag_entry *g;
  int gagcount = count_gags(p);

  if (!gagcount)
  {
    TELLPLAYER(p, " You aren't gagging anyone atm.\n");
    return;
  }
  ADDSTACK(" Listing all your current gags:\n  ");

  for (g = p->gag_top; g; g = g->next)
    ADDSTACK(" %s", g->gagged->name);

  if (gagcount > 1)
    ENDSTACK("\n You are gagging %d people\n", gagcount);
  else
    ENDSTACK("\n You are gagging one person\n");

  TELLPLAYER(p, oldstack);
  stack = oldstack;
}

void clear_gag_logoff(player * git)
{

  player *scan;

  for (scan = flatlist_start; scan; scan = scan->flat_next)
  {
    if (match_gag(scan, git))
      del_gag(scan, git->lower_name, 0);
  }
}

void purge_gaglist(player * p, char *str)
{

  gag_entry *g, *g2;

  if (!count_gags(p))
    return;
  for (g = p->gag_top, g2 = g; g2; g = g2)
  {
    g2 = g->next;
    FREE(g);
  }
  p->gag_top = 0;
}
