/*
 * Playground+ - dsc.c
 * Multiple dynamic user defined saved channels (c) phypor 1998
 * ---------------------------------------------------------------------------
 *
 * Modifications to original release:
 *  Include paths
 *  varible argument functions 
 *  remove plural gender cases
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "include/config.h"
#include "include/player.h"
#include "include/proto.h"

 /* if you want to have a diffrent char appear in front of channel stufs,
    change it here....(note, keep it a single char with '' around it ...)
  */
#define DSC_TAG         ')'



void tell_channels(char *channel, char *msg)
{
  player *scan;
  int i, oct = command_type;

  command_type = 0;

  for (scan = flatlist_start; scan; scan = scan->flat_next)
    for (i = 0; i < MAX_CHANNELS; i++)
      if (!strcasecmp(scan->channels[i], channel) &&
	  !(scan->dsc_flags & BLOCK_ALL_CHANS))
      {
	if (scan->dsc_flags & HI_CHANS)
	  command_type |= HIGHLIGHT;

	TELLPLAYER(scan, "%c %s", DSC_TAG, msg);

	if (scan->dsc_flags & HI_CHANS)
	  command_type &= ~HIGHLIGHT;
	break;
      }

  command_type = oct;

}




void connect_channels(player * p)
{
  char *oldstack = stack;
  int i;

  if (p->dsc_flags & BLOCK_ALL_CHANS)
    return;

  for (i = 0; i < MAX_CHANNELS; i++)
    if (*(p->channels[i]))
    {
      sprintf(stack, "[%s] ++ %s logs in and joins this channel ++\n", p->channels[i], p->name);
      stack = end_string(stack);
      tell_channels(p->channels[i], oldstack);
      stack = oldstack;
    }
}

void disconnect_channels(player * p)
{
  char *oldstack = stack;
  int i;

  if (p->dsc_flags & BLOCK_ALL_CHANS || !p->location || !p->name[0])
    return;

  for (i = 0; i < MAX_CHANNELS; i++)
    if (*(p->channels[i]))
    {
      sprintf(stack, "[%s] -- %s leaves this channel and quits %s --\n",
	      p->channels[i], p->name, get_config_msg("talker_name"));
      stack = end_string(stack);
      tell_channels(p->channels[i], oldstack);
      stack = oldstack;
    }
}


char *says_asks_exclaims(player * p, char *str)
{
  if (*str)
    switch (str[strlen(str) - 1])
    {
      case '!':
	return "exclaims";
      case '?':
	return "asks";
    }
  return "says";
}


int tell_command_channel(player * p, char *chan, char *msg)
{
  int i;
  char *oldstack = stack;

  if (p->dsc_flags & BLOCK_ALL_CHANS)
    return 0;

  for (i = 0; i < MAX_CHANNELS; i++)
  {
    if (!strncasecmp(p->channels[i], chan, strlen(chan)))
    {
      sprintf(stack, "[%s] %s %s '%s'\n", p->channels[i], p->name,
	      says_asks_exclaims(p, msg), msg);
      stack = end_string(stack);
      tell_channels(p->channels[i], oldstack);
      stack = oldstack;
      return 1;
    }
  }
  return 0;
}

int remote_command_channel(player * p, char *chan, char *msg)
{
  int i;
  char *oldstack = stack;

  if (p->dsc_flags & BLOCK_ALL_CHANS)
    return 0;

  for (i = 0; i < MAX_CHANNELS; i++)
  {
    if (!strncasecmp(p->channels[i], chan, strlen(chan)))
    {
      if ((*msg == '\'') || (*msg == ','))
	sprintf(stack, "[%s] %s%s\n", p->channels[i], p->name, msg);
      else
	sprintf(stack, "[%s] %s %s\n", p->channels[i], p->name, msg);
      stack = end_string(stack);
      tell_channels(p->channels[i], oldstack);
      stack = oldstack;
      return 1;
    }
  }
  return 0;
}


void list_whos_on_channel(player * p, char *channel)
{
  player *scan;
  int counted = 0, i;
  char *oldstack = stack, temp[70];

  sprintf(temp, "People on the '%s' channel", channel);
  pstack_mid(temp);

  for (scan = flatlist_start; scan; scan = scan->flat_next)
    for (i = 0; i < MAX_CHANNELS; i++)
      if (!strcasecmp(scan->channels[i], channel))
      {
	counted++;
	if (scan->dsc_flags & BLOCK_ALL_CHANS)
	  stack += sprintf(stack, "(%s), ", scan->name);
	else
	  stack += sprintf(stack, "%s, ", scan->name);
      }

  if (counted)
  {
    stack = end_string(stack);
    oldstack[strlen(oldstack) - 2] = '.';
    oldstack[strlen(oldstack) - 1] = '\n';
    tell_player(p, oldstack);
    tell_player(p, LINE);
  }
  else
    tell_player(p, " No-one on the channel ?!? *le boggle*\n");
  stack = oldstack;
}


void list_channels(player * p, char *str)
{
  int i, hit = 0;
  char *oldsac = stack, temp[70];

  for (i = 0; i < MAX_CHANNELS; i++)
    if (*(p->channels[i]))
      break;
  if (i == MAX_CHANNELS)
  {
    tell_player(p, " You aren't on any channels atm.\n");
    return;
  }

  if (*str)
  {
    for (i = 0; i < MAX_CHANNELS; i++)
      if (!strncasecmp(p->channels[i], str, strlen(str)))
      {
	list_whos_on_channel(p, p->channels[i]);
	return;
      }
    TELLPLAYER(p, " But you aren't on the '%s' channel?\n", str);
    return;
  }


  pstack_mid("You are on the following channels");

  for (i = 0; i < MAX_CHANNELS; i++)
    if (*(p->channels[i]))
    {
      stack += sprintf(stack, " %s\n", p->channels[i]);
      hit++;
    }
  if (hit)
  {
    if (hit == 1)
      sprintf(temp, "1 channel out of a maximum of %d", MAX_CHANNELS);
    else
      sprintf(temp, "%d channels out of a maximum of %d", hit, MAX_CHANNELS);
    pstack_mid(temp);
    *stack++ = 0;
    tell_player(p, oldsac);
  }
  else
    tell_player(p, " You are on no channels at all.\n");

  stack = oldsac;

  if (p->dsc_flags & BLOCK_ALL_CHANS)
    tell_player(p, " Btw, you are blocking all channels.\n");

}




void join_channel(player * p, char *str)
{
  char *oldstack = stack, *ptr = str;
  player *p2, dummy;
  int i;

  if (!*str)
  {
    tell_player(p, " Format : join_chan <channel to join>\n");
    return;
  }

  if (p->dsc_flags & BLOCK_ALL_CHANS)
  {
    tell_player(p, " You should unblock channels before doing that.\n");
    return;
  }
  while (*ptr++)
    if (*ptr == ' ')
    {
      *ptr = '\0';
      break;
    }

  if (strlen(str) > MAX_NAME - 2)
  {
    tell_player(p, " That is just too long to have as a channel name.\n");
    return;
  }
  p2 = find_player_absolute_quiet(str);
  if (p2)
  {
    tell_player(p, " You dont really want to name your channel "
		"after a person... do you?\n");
    return;
  }
  strncpy(dummy.lower_name, str, MAX_NAME - 1);
  lower_case(dummy.lower_name);
  dummy.fd = p->fd;
  if (load_player(&dummy))
  {
    tell_player(p, " You dont really want to name your channel "
		"after a person... do you?.\n");
    return;
  }
  for (i = 0; i < MAX_CHANNELS; i++)
    if (!strcasecmp(p->channels[i], str))
    {
      tell_player(p, " You are already on that channel.\n");
      return;
    }

  for (i = 0; i < MAX_CHANNELS; i++)
    if (!*(p->channels[i]))
      break;
  if (*(p->channels[i]))
  {
    tell_player(p, " You are on the most channels that you may join.\n");
    return;
  }
  strncpy(p->channels[i], str, MAX_NAME - 1);

  sprintf(stack, "[%s] ++ %s joins this channel ++\n", str, p->name);
  stack = end_string(stack);
  tell_channels(str, oldstack);
  stack = oldstack;
}

void leave_channel(player * p, char *str)
{
  char *oldstack = stack;
  int i;

  if (!*str)
  {
    tell_player(p, " Format : leave_chan <channel to leave>\n");
    return;
  }

  for (i = 0; i < MAX_CHANNELS; i++)
    if (!strcasecmp(p->channels[i], str))
    {
      sprintf(stack, "[%s] -- %s leaves this channel --\n", str, p->name);
      stack = end_string(stack);
      tell_channels(str, oldstack);
      stack = oldstack;
      memset(p->channels[i], 0, MAX_NAME);
      if (p->dsc_flags & BLOCK_ALL_CHANS)
	tell_player(p, " You leave the channel.\n");

      return;
    }

  tell_player(p, " You don't seem to be on that channel.\n");
}



void hichan(player * p, char *str)
{
  if (!*str)
    p->dsc_flags ^= HI_CHANS;

  else if (!strcasecmp(str, "on"))
    p->dsc_flags |= HI_CHANS;

  else if (!strcasecmp(str, "off"))
    p->dsc_flags &= ~HI_CHANS;

  else
    tell_player(p, " Format : chan_hi [on/off]\n");

  if (p->dsc_flags & HI_CHANS)
    tell_player(p, " You will get channels hilighted.\n");
  else
    tell_player(p, " You will not get channels hilighted.\n");
}



void block_all_channels(player * p, char *str)
{
  if (!*str)
    p->dsc_flags ^= BLOCK_ALL_CHANS;

  else if (!strcasecmp(str, "on"))
    p->dsc_flags |= BLOCK_ALL_CHANS;

  else if (!strcasecmp(str, "off"))
    p->dsc_flags &= ~BLOCK_ALL_CHANS;

  else
    tell_player(p, " Format : block_all_chans [on/off]\n");

  if (p->dsc_flags & BLOCK_ALL_CHANS)
    tell_player(p, " You are blocking off all channels.\n");
  else
    tell_player(p, " You are not blocking channels.\n");
}
