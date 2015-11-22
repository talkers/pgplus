/*
 * Playground+ - games.c
 * PRS game (which I hate) and a few other bits and bobs for your amusement
 * ---------------------------------------------------------------------------
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/config.h"
#include "include/player.h"
#include "include/proto.h"

/* for paper_rock_scissors -
   unsigned int prs; 
   prs%4 = selection (null, paper, rock, scissors)
   prs/4 = game number.
 */

void check_prs2(player * p)
{

  int wins = 0, loss = 0, ties = 0;

  loss = p->prs_record % 1000;
  wins = p->prs_record / 1000000;
  ties = (p->prs_record / 1000) - (wins * 1000);

  if (wins >= 1000)
  {
    TELLPLAYER(p, " -=*> Congratulations on your 1000th win!!\n");
    LOGF("prs", "%s makes grand champion with a record of %d-%d-%d", p->name, wins, ties, loss);
    TELLPLAYER(p, " Resetting your stats to avoid overflow.\n");
    p->prs_record = 0;
  }
  else if (loss >= 1000)
  {
    TELLPLAYER(p, " -=*> Ack! That's your 1000th loss!!\n");
    LOGF("prs", "%s becomes a big loooozer -- %d-%d-%d", p->name, wins, ties, loss);
    TELLPLAYER(p, " Resetting your stats (mercifly) to avoid overflow.\n");
    p->prs_record = 0;
  }
  else if (ties >= 1000)
  {
    TELLPLAYER(p, " -=*> *Yawn* 1000 ties already?!\n");
    LOGF("prs", "%s reaches the apex of mediocraty -- %d-%d-%d", p->name, wins, ties, loss);
    TELLPLAYER(p, " Resetting your stats (mercifly) to avoid overflow.\n");
    p->prs_record = 0;
  }
}

void check_prs(player * p1, player * p2)
{
  check_prs2(p1);
  check_prs2(p2);
}

int get_prs_choice(char *str)
{

  if (*str == 'p' || *str == 'P')
    return 1;
  if (*str == 'r' || *str == 'R')
    return 2;
  if (*str == 's' || *str == 'S')
    return 3;
  return 0;
}

int open_new_prsgame(void)
{
  int game, con = 1;
  player *scan;

  for (game = 4; con && game < 10000; game += 4)
  {
    con = 0;
    for (scan = flatlist_start; scan; scan = scan->flat_next)
      if (scan->prs / 4 == game)
	con = 1;
    if (!con)
      return game;
  }
  return 0;
}
void paper_rock_scissors(player * p, char *str)
{
  char *sel;
  player *p2;
  int choice, gameno;
  list_ent *l;

  if (!*str)
  {
    TELLPLAYER(p, " Format: prs <player> <paper/rock/scissors>\n");
    return;
  }
  if (!strcasecmp(str, "clear"))
  {
    TELLPLAYER(p, " You clear prs game status.\n");
    if (p->prs / 4 == 0)
      return;
    gameno = p->prs / 4;
    p->prs = 0;
    for (p2 = flatlist_start; p2; p2 = p2->flat_next)
      if (p2->prs / 4 == gameno)
      {
	if (p2->misc_flags & GAME_HI)
	  command_type |= HIGHLIGHT;
	TELLPLAYER(p2, " (%d) %s backs out of the game.\n", gameno / 4, p->name);
	if (p2->misc_flags & GAME_HI)
	  command_type &= ~HIGHLIGHT;
	p2->prs = 0;
      }
    return;
  }
  sel = next_space(str);
  if (*sel)
    *sel++ = 0;
  else
  {
    TELLPLAYER(p, " Format: prs <player> <paper/rock/scissors>\n");
    return;
  }
  p2 = find_player_global(str);
  if (!p2)
    return;

#ifdef ROBOTS
  if (p2->residency & ROBOT_PRIV)
  {
    tell_player(p, " You cannot play PRS against a robot\n");
    return;
  }
#endif

  if (p->misc_flags & NO_PRS)
  {
    TELLPLAYER(p, " You cannot play prs when blocking game offers.\n");
    return;
  }
  if (p2->misc_flags & NO_PRS)
  {
    TELLPLAYER(p, " That person doesn't like that game.\n");
    return;
  }
  if (p2->saved)
  {
    l = fle_from_save(p2->saved, p->lower_name);
    if (l && l->flags & (IGNORE | BLOCK))
    {
      TELLPLAYER(p, " That person is ignoring you.\n");
      return;
    }
  }
  if (p == p2)
  {
    TELLPLAYER(p, " Now where would the challenge in THAT be?\n");
    return;
  }
  if (p->prs / 4)
  {
    gameno = p->prs / 4;
    if (p2->prs / 4 != gameno)
    {
      if (p->misc_flags & GAME_HI)
	command_type |= HIGHLIGHT;
      TELLPLAYER(p, " (%d) You're not playing against %s..\n", gameno / 4, p2->name);
      if (p->misc_flags & GAME_HI)
	command_type &= ~HIGHLIGHT;
      return;
    }
    else
    {
      choice = get_prs_choice(sel);
      if (!choice)
      {
	if (p->misc_flags & GAME_HI)
	  command_type |= HIGHLIGHT;
	TELLPLAYER(p, " (%d) Try rock, paper, or scissors.\n", gameno / 4);
	if (p->misc_flags & GAME_HI)
	  command_type &= ~HIGHLIGHT;
	return;
      }
      if (p2->prs % 4 == 0)
      {
	if (p->misc_flags & GAME_HI)
	  command_type |= HIGHLIGHT;
	TELLPLAYER(p, " (%d) You've already offered a game to %s... be patient\n", gameno / 4, p2->name);
	if (p->misc_flags & GAME_HI)
	  command_type &= ~HIGHLIGHT;
	if (p2->misc_flags & GAME_HI)
	  command_type |= HIGHLIGHT;
	TELLPLAYER(p2, " (%d) %s waits for you go play against %s... \n", gameno / 4, p->name, get_gender_string(p));
	if (p2->misc_flags & GAME_HI)
	  command_type &= ~HIGHLIGHT;
	return;
      }
      if (choice == p2->prs % 4)
      {
	if (p->misc_flags & GAME_HI)
	  command_type |= HIGHLIGHT;
	TELLPLAYER(p, " (%d) You tie with %s.\n", gameno / 4, p2->name);
	if (p->misc_flags & GAME_HI)
	  command_type &= ~HIGHLIGHT;
	if (p2->misc_flags & GAME_HI)
	  command_type |= HIGHLIGHT;
	TELLPLAYER(p2, " (%d) You tie with %s.\n", gameno / 4, p->name);
	if (p2->misc_flags & GAME_HI)
	  command_type &= ~HIGHLIGHT;
	p->prs = 0;
	p2->prs = 0;
	p->prs_record += 1000;
	p2->prs_record += 1000;
	check_prs(p, p2);
	return;
      }
      if ((choice == 1 && p2->prs % 4 == 2) ||
	  (choice == 2 && p2->prs % 4 == 3) ||
	  (choice == 3 && p2->prs % 4 == 1))
      {
	if (p->misc_flags & GAME_HI)
	  command_type |= HIGHLIGHT;
	TELLPLAYER(p, " (%d) You defeat %s!\n", gameno / 4, p2->name);
	if (p->misc_flags & GAME_HI)
	  command_type &= ~HIGHLIGHT;
	if (p2->misc_flags & GAME_HI)
	  command_type |= HIGHLIGHT;
	TELLPLAYER(p2, " (%d) Rats! %s beat you!\n", gameno / 4, p->name);
	if (p2->misc_flags & GAME_HI)
	  command_type &= ~HIGHLIGHT;
	p->prs = 0;
	p2->prs = 0;
	p->prs_record += 1000000;
	p2->prs_record += 1;
	check_prs(p, p2);
      }
      else
      {
	if (p2->misc_flags & GAME_HI)
	  command_type |= HIGHLIGHT;
	TELLPLAYER(p2, " (%d) You defeat %s!\n", gameno / 4, p->name);
	if (p2->misc_flags & GAME_HI)
	  command_type &= ~HIGHLIGHT;
	if (p->misc_flags & GAME_HI)
	  command_type |= HIGHLIGHT;
	TELLPLAYER(p, " (%d) Rats! %s beat you!\n", gameno / 4, p2->name);
	if (p->misc_flags & GAME_HI)
	  command_type &= ~HIGHLIGHT;
	p->prs = 0;
	p2->prs = 0;
	p->prs_record += 1;
	p2->prs_record += 1000000;
	check_prs(p, p2);
      }
    }
  }
  else
  {				/* starting a new game */
    choice = get_prs_choice(sel);
    if (!choice)
    {
      TELLPLAYER(p, " Try rock, paper, or scissors.\n");
      return;
    }
    if (p2->prs / 4)
    {
      TELLPLAYER(p, " That person is playing someone else atm.\n");
      return;
    }
    gameno = open_new_prsgame();
    p->prs = gameno * 4 + choice;
    p2->prs = gameno * 4;
    if (p2->misc_flags & GAME_HI)
      command_type |= HIGHLIGHT;
    /* Ive removed the cheat that was here */
    TELLPLAYER(p2, " (%d) %s wants to play Paper-rock-scissors with you.\n", gameno / 4, p->name);
    if (p2->misc_flags & GAME_HI)
      command_type &= ~HIGHLIGHT;
    if (p->misc_flags & GAME_HI)
      command_type |= HIGHLIGHT;
    TELLPLAYER(p, " (%d) You ask %s to play Paper-rock-scissors with you.\n", gameno / 4, p2->name);
    if (p->misc_flags & GAME_HI)
      command_type &= ~HIGHLIGHT;
  }
}

void toggle_noprs(player * p, char *str)
{

  p->misc_flags ^= NO_PRS;

  if (p->misc_flags & NO_PRS)
    TELLPLAYER(p, " You are blocking all game offers.\n");
  else
    TELLPLAYER(p, " You unblock game offers.\n");
}

void prs_record_display(player * p)
{

  int wins = 0, loss = 0, ties = 0;
  char *tempstack;

  loss = p->prs_record % 1000;
  wins = p->prs_record / 1000000;
  ties = (p->prs_record / 1000) - (wins * 1000);

  tempstack = stack;
  sprintf(stack, "%d wins, %d losses, %d ties\n", wins, loss, ties);
  stack = strchr(stack, 0);
}

void game_hi(player * p, char *str)
{

  p->misc_flags ^= GAME_HI;

  if (p->misc_flags & GAME_HI)
    TELLPLAYER(p, " You hilight all game offers.\n");
  else
    TELLPLAYER(p, " You turn off game hilighting.\n");
}

/* Tonhe's magic8ball (adapted slightly for PG+) */
void eightball(player * p, char *str)
{
  int r;
  char msg[50];
  char *oldstack = stack;

  if (!*str)
  {
    tell_player(p, " Format: mball <your yes or no question>\n");
    return;
  }
  if (config_flags & cfNOSWEAR)
    if (!strcmp(p->location->owner->lower_name, SYS_ROOM_OWNER) ||
	!strcmp(p->location->owner->lower_name, "intercom"))
      str = filter_rude_words(str);

  /* Pick computer response */
  r = rand() % 5;
  switch (r)
  {
    case 0:
      strcpy(msg, "Reply hazy ... ask again later.");
      break;
    case 1:
      strcpy(msg, "Its a definite possibility.");
      break;
    case 2:
      strcpy(msg, "Probably not ...");
      break;
    case 3:
      strcpy(msg, "You can count on it!");
      break;
    case 4:
      strcpy(msg, "No way ...");
      break;
  }

  /* Players personal message */
  TELLPLAYER(p, "You ask the Magic8Ball '%s'\n", str);
  TELLPLAYER(p, "-> The Magic8Ball replies .oO( %s )Oo.\n", msg);

  /* Message to the room that ther player is in */
  sprintf(stack, "%s asks the Magic8Ball '%s'\n", p->name, str);
  stack = end_string(stack);
  tell_room_but(p, p->location, oldstack);
  stack = oldstack;
  sprintf(stack, "-> The Magic8Ball replies .oO( %s )Oo.\n", msg);
  stack = end_string(stack);
  tell_room_but(p, p->location, oldstack);
  stack = oldstack;

}
