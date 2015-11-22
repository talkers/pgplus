/*
 * Playground+ - channel.c
 * All new and improved channel code written by Silver (main, spod & zchannel)
 * ----------------------------------------------------------------------------
 * Serious rewrite of channel commands using generalisations, by blimey
 * saved on hundreds of lines of ghastly repeated code
 *
 * Repeated code? Me? Never .. oops [whistles innocently] -- Silver 
 */

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>

#include "include/config.h"
#include "include/player.h"
#include "include/proto.h"

/*
 * Shadow Realms channel code -- by blimey
 *
 */
char *how_to_say(char *str)
{
  for (; *str; str++);
  switch (*(--str))
  {
    case '?':
      return "asks";
    case '!':
      return "exclaims";
    default:
      return "says";
  }
}

/*
 * Here are the higher order definitions of channel commands -- blimey
 *
 */
void channel_emote(player * p, char *str,
		   void (*the_wall) (char *,...))
{

  if (p->flags & FROGGED)
    str = "shakes a broken rattle";

  if ((*str == '`') || (*str == '\''))
    the_wall("%s%s^N\n", p->name, str);
  else
    the_wall("%s %s^N\n", p->name, str);
}

void channel_say(player * p, char *str,
		 void (*the_wall) (char *,...))
{

  the_wall("%s %s '%s%s^N'\n",
	   p->name,
	   how_to_say(str),
	   str,
	   (p->flags & FROGGED ? " and WHERE'S MY RATTLE?!" : ""));
}

void channel_sing(player * p, char *str,
		  void (*the_wall) (char *,...))
{

  the_wall("%s sings o/~ %s ^No/~%s\n",
	   p->name,
	   str,
	   (p->flags & FROGGED ? " while shaking a broken rattle" : ""));
}

void channel_think(player * p, char *str,
		   void (*the_wall) (char *,...))
{

  the_wall("%s thinks %s. o O ( %s ^N)\n",
	   p->name,
	   (p->flags & FROGGED ? "in an infantile fashion " : ""),
	   str);
}

/* Silver did these 3 rather handy funtions :) */
/* Awwww, gee thanks :o) --Silver */

/* Main Channel */

void ppl_wall(char *str)
{
  player *scan;

  for (scan = flatlist_start; scan; scan = scan->flat_next)
  {
    if (scan->residency && !(scan->misc_flags & NO_MAIN_CHANNEL) && scan->location)
    {
      if (scan->misc_flags & CHAN_HI)
      {
	command_type |= HIGHLIGHT;
      }
      sys_color_atm = UCOsc;
      tell_player(scan, str);
      sys_color_atm = SYSsc;
      if (scan->misc_flags & CHAN_HI)
      {
	command_type &= ~HIGHLIGHT;
      }
    }
  }
}

/* Spod channel */

void spod_wall(char *str)
{
  player *scan;

  for (scan = flatlist_start; scan; scan = scan->flat_next)
  {
    if ((scan->residency & SPOD) && !(scan->misc_flags & NO_SPOD_CHANNEL) && scan->location)
    {
      if (scan->misc_flags & CHAN_HI)
      {
	command_type |= HIGHLIGHT;
      }
      sys_color_atm = UCEsc;
      tell_player(scan, str);
      sys_color_atm = SYSsc;
      if (scan->misc_flags & CHAN_HI)
      {
	command_type &= ~HIGHLIGHT;
      }
    }
  }
}

/* ZChannels */

void zwall(char *str)
{
  player *scan, *p = current_player;	/* blimey changed this */

  for (scan = flatlist_start; scan; scan = scan->flat_next)
  {
    if (scan->residency && !strcasecmp(scan->zchannel, p->zchannel) && scan->location)
    {
      if (scan->misc_flags & CHAN_HI)
      {
	command_type |= HIGHLIGHT;
      }
      sys_color_atm = ZCHsc;
      tell_player(scan, str);
      scan->zcidle = 0;		/* reset that channels idle time */
      sys_color_atm = SYSsc;
      if (scan->misc_flags & CHAN_HI)
      {
	command_type &= ~HIGHLIGHT;
      }
    }
  }
}

/* A debugging one */

void debug_wall(char *str)
{
  player *scan;

  for (scan = flatlist_start; scan; scan = scan->flat_next)
  {
    if ((scan->misc_flags & SEE_DEBUG_CHANNEL) && scan->location)
    {
      if (scan->misc_flags & CHAN_HI)
      {
	command_type |= HIGHLIGHT;
      }
      TELLPLAYER(scan, "%s %s\n", get_config_msg("debug_chan"), str);
      if (scan->misc_flags & CHAN_HI)
      {
	command_type &= ~HIGHLIGHT;
      }
    }
  }
}


/*
 * blimey wrote this ....
 *
 */

void any_chan(char *prefix,
	      void (*the_wall) (char *),
	      char *format,
	      va_list ap)
{

  char *oldstack = stack;
  command_type = 0;

  sprintf(stack, "%s ", prefix);
  stack = strchr(stack, 0);

  vsprintf(stack, format, ap);
  stack = end_string(stack);

  the_wall(oldstack);
  stack = oldstack;
}


void au_chan(char *format,...)
{
  va_list x;
  va_start(x, format);
  any_chan(get_config_msg("admin_chan"), au_wall, format, x);
  va_end(x);
}
void cu_chan(char *format,...)
{
  va_list x;
  va_start(x, format);
  any_chan(get_config_msg("main_chan"), ppl_wall, format, x);
  va_end(x);
}

#ifdef HC_CHANNEL
void hu_chan(char *format,...)
{
  va_list x;
  va_start(x, format);
  any_chan(get_config_msg("hc_chan"), hc_wall, format, x);
  va_end(x);
}

#endif
void pu_chan(char *format,...)
{
  va_list x;
  va_start(x, format);
  any_chan(get_config_msg("spod_chan"), spod_wall, format, x);
  va_end(x);
}
void su_chan(char *format,...)
{
  va_list x;
  va_start(x, format);
  any_chan(get_config_msg("su_chan"), su_wall, format, x);
  va_end(x);
}
void zu_chan(char *format,...)
{
  va_list x;
  char *oldstack = stack;

  sprintf(stack, "<(%s)>", current_player->zchannel);
  stack += 1 + strlen(stack);
  va_start(x, format);
  any_chan(oldstack, zwall, format, x);
  va_end(x);

  stack = oldstack;
}

int got_msg(player * p, char *str, char *cname)
{

  if (!*str)
    TELLPLAYER(p, " Format: %s <message>\n", cname);
  return *str;
}

void toggle_debug_channel(player * p, char *str)
{
  if (p->misc_flags & SEE_DEBUG_CHANNEL)
  {
    p->misc_flags &= ~SEE_DEBUG_CHANNEL;
    tell_player(p, " You ignore the debug channel.\n");
  }
  else
  {
    p->misc_flags |= SEE_DEBUG_CHANNEL;
    tell_player(p, " You start viewing the debug channel.\n");
  }
}

void channel_toggle(player * p,
		    void (*the_wall) (char *,...),
		    int flag)
{

  if (p->misc_flags & flag)
  {
    p->misc_flags &= ~flag;
    the_wall("++ %s joins the channel ++\n", p->name);
  }
  else
  {
    the_wall("-- %s leaves the channel --\n", p->name);
    p->misc_flags |= flag;
  }
}

int not_blocking_su_channel(player * p)
{
  if (!(p->flags & BLOCK_SU))
    return 1;
  tell_player(p, " You are off duty, so you can't use the channel.\n");
  return 0;
}

int is_on_channel(player * p, int flag)
{
  char *which = NULL;

  if (!(p->misc_flags & flag))
    return 1;

  switch (flag)
  {
    case NO_SPOD_CHANNEL:
      which = "spod";
      break;
    case NO_MAIN_CHANNEL:
      which = "main";
      break;
      /* add in any more channels here ... */
  }
  TELLPLAYER(p, " You aren't on the %s channel!\n", which);
  return 0;
}

int is_on_zchannel(player * p)
{
  if (!p->zchannel[0])
    tell_player(p, " You aren't on any zchannel!\n");
  return p->zchannel[0];
}

/*
 * Actual channel command functions now follow -- blimey
 *
 */
void chanhi(player * p, char *str)
{
  TELLPLAYER(p, " Channels will no%c be hilited.\n",
	     (p->misc_flags ^= CHAN_HI) & CHAN_HI ? 'w' : 't');
}

/* toggle whether the su channel is highlighted or not */
void su_hilited(player * p, char *str)
{
  TELLPLAYER(p, " You will %sget the su channel hilited.\n",
	     (p->misc_flags ^= SU_HILITED) & SU_HILITED ? "" : "not ");
}

/*
 * Admin channel
 *
 */
void adminemote(player * p, char *str)
{
  if (got_msg(p, str, "ae") && not_blocking_su_channel(p))
    channel_emote(p, str, au_chan);
}
void adminsing(player * p, char *str)
{
  if (got_msg(p, str, "as") && not_blocking_su_channel(p))
    channel_sing(p, str, au_chan);
}
void adminthink(player * p, char *str)
{
  if (got_msg(p, str, "at") && not_blocking_su_channel(p))
    channel_think(p, str, au_chan);
}
void ad(player * p, char *str)
{
  if (got_msg(p, str, "au") && not_blocking_su_channel(p))
    channel_say(p, str, au_chan);
}
/*
 * Main channel
 *
 */
void ce(player * p, char *str)
{
  if (got_msg(p, str, "ce") && is_on_channel(p, NO_MAIN_CHANNEL))
    channel_emote(p, str, cu_chan);
}
void cs(player * p, char *str)
{
  if (got_msg(p, str, "cs") && is_on_channel(p, NO_MAIN_CHANNEL))
    channel_sing(p, str, cu_chan);
}
void ct(player * p, char *str)
{
  if (got_msg(p, str, "ct") && is_on_channel(p, NO_MAIN_CHANNEL))
    channel_think(p, str, cu_chan);
}
void cu(player * p, char *str)
{
  if (got_msg(p, str, "cu") && is_on_channel(p, NO_MAIN_CHANNEL))
    channel_say(p, str, cu_chan);
}
#ifdef HC_CHANNEL
/* HCAdmin channel - not one I particulary want but this was requested */
/* Bah! -- Silver */

/* Note - HC channel colour is the same as the admin channel colour. I could
   have given it a new colour BUT it would mean that PG+ was not directly
   compatable with PG pfiles (you would have had to run the pfiles through
   a convertor) so its a programming challenge for you to code it in! */
void he(player * p, char *str)
{
  if (got_msg(p, str, "he") && not_blocking_su_channel(p))
    channel_emote(p, str, hu_chan);
}
void hs(player * p, char *str)
{
  if (got_msg(p, str, "hs") && not_blocking_su_channel(p))
    channel_sing(p, str, hu_chan);
}
void ht(player * p, char *str)
{
  if (got_msg(p, str, "ht") && not_blocking_su_channel(p))
    channel_think(p, str, hu_chan);
}
void hd(player * p, char *str)
{
  if (got_msg(p, str, "hu") && not_blocking_su_channel(p))
    channel_say(p, str, hu_chan);
}
#endif
/*
 * Spod channel
 *
 */
void pe(player * p, char *str)
{
  if (got_msg(p, str, "pe") && is_on_channel(p, NO_SPOD_CHANNEL))
    channel_emote(p, str, pu_chan);
}
void ps(player * p, char *str)
{
  if (got_msg(p, str, "ps") && is_on_channel(p, NO_SPOD_CHANNEL))
    channel_sing(p, str, pu_chan);
}
void pt(player * p, char *str)
{
  if (got_msg(p, str, "pt") && is_on_channel(p, NO_SPOD_CHANNEL))
    channel_think(p, str, pu_chan);
}
void pu(player * p, char *str)
{
  if (got_msg(p, str, "pu") && is_on_channel(p, NO_SPOD_CHANNEL))
    channel_say(p, str, pu_chan);
}
/*
 * SU channel
 *
 */
void suemote(player * p, char *str)
{
  if (got_msg(p, str, "se") && not_blocking_su_channel(p))
    channel_emote(p, str, su_chan);
}
void susing(player * p, char *str)
{
  if (got_msg(p, str, "ss") && not_blocking_su_channel(p))
    channel_sing(p, str, su_chan);
}
void suthink(player * p, char *str)
{
  if (got_msg(p, str, "st") && not_blocking_su_channel(p))
    channel_think(p, str, su_chan);
}
void su(player * p, char *str)
{
  if (got_msg(p, str, "su") && not_blocking_su_channel(p))
    channel_say(p, str, su_chan);
}
/*
 * Z - Channels
 *
 */
void ze(player * p, char *str)
{
  if (got_msg(p, str, "ze") && is_on_zchannel(p))
    channel_emote(p, str, zu_chan);
}
void zs(player * p, char *str)
{
  if (got_msg(p, str, "zs") && is_on_zchannel(p))
    channel_sing(p, str, zu_chan);
}
void zt(player * p, char *str)
{
  if (got_msg(p, str, "zt") && is_on_zchannel(p))
    channel_think(p, str, zu_chan);
}
void zu(player * p, char *str)
{
  if (got_msg(p, str, "zu") && is_on_zchannel(p))
    channel_say(p, str, zu_chan);
}
/*
 * Rest of file is Silver's work
 *
 */

/* Thank you blimey and I present to you ... ZChannel stuff :o) */

void zc(player * p, char *str)
{
  char *oldstack = stack;
  char *check;
  player *scan;
  int newchan = 1;
  char chan_copy[20];

  if (!*str)
  {
    if (p->zchannel[0])
    {
      ENDSTACK("<(%s)> -- %s leaves this channel --\n", p->zchannel, p->name);
      zwall(oldstack);
      stack = oldstack;
      p->zchannel[0] = '\0';
    }
    else
      tell_player(p, " Format: zc <channel name to join or create>\n");
    return;
  }
  if (strlen(str) < 3 || strlen(str) > 14)
  {
    tell_player(p, "Z Channel names must be between 3 and 14 characters\n");
    return;
  }
  for (check = str; *check; check++)
  {
    switch (*check)
    {
      case '^':
	tell_player(p, " You may not have colours in your channel name.\n");
	return;
      case ' ':
	tell_player(p, " You may not have spaces in your channel name.\n");
	return;
      case ',':
	tell_player(p, " You may not have commas in your channel name.\n");
	return;
      case '{':
      case '}':
      case '*':
      case '<':
      case '>':
      case '[':
      case ']':
	TELLPLAYER(p, " You may not have %c in your channel name.\n", *check);
	return;
      case '(':
	tell_player(p, " You may not have brackets in your channel name.\n");
	return;
      case ')':
	tell_player(p, " You may not have brackets in your channel name.\n");
	return;
    }
  }

  if (!strcasecmp(str, p->zchannel))
  {
    tell_player(p, "Erm, you're already on that channel!\n");
    return;
  }
  /* Makes a copy of the channel name, converts it into lower case and
     does all the necessary checks. This means that ststr can pick up
     on reserved names and people don't moan becuase they
     can't use mixed case channel names -- Silver */

  strcpy(chan_copy, str);	/* copy it */
  lower_case(chan_copy);	/* convert it all to lower case */

  /* Here you need to enter the name of any channel names you DONT want
     people to use */

  if (strstr(chan_copy, "channel") || strstr(chan_copy, "spod") ||
      strstr(chan_copy, "admin") || strstr(chan_copy, "su"))
  {
    tell_player(p, " Sorry but that channel name is reserved.\n");
    return;
  }
  if (p->zchannel[0])
  {
    ENDSTACK("<(%s)> -- %s leaves this channel --\n", p->zchannel, p->name);
    zwall(oldstack);
    stack = oldstack;
  }
  for (scan = flatlist_start; scan; scan = scan->flat_next)
    if (!strcasecmp(scan->zchannel, str) && scan->location)
    {
      strcpy(p->zchannel, scan->zchannel);
      newchan = 0;
    }
  if (newchan == 1)
    strcpy(p->zchannel, str);

  if (newchan)
    ENDSTACK("<(%s)> ++ %s creates and joins this channel ++\n", p->zchannel, p->name);
  else
    ENDSTACK("<(%s)> ++ %s joins this channel ++\n", p->zchannel, p->name);

  zwall(oldstack);
  stack = oldstack;

  LOGF("channel", "%s creates a zchannel called '%s'", p->name, str);
}


void lsz(player * p, char *str)
{

  char *oldstack;
  player *scan, *scan2;
  int count = 0;
  int noz = 0;

  oldstack = stack;
  pstack_mid("Active Z-Channels");
  ADDSTACK("\nZ Channel Name                Number of people       Idle Time\n");

  for (scan = flatlist_start; scan; scan = scan->flat_next)
    scan->zchannellisted = 0;

  for (scan = flatlist_start; scan; scan = scan->flat_next)
  {
    if (strlen(scan->zchannel) > 0 && scan->zchannellisted != 1)
    {
      count = 0;
      for (scan2 = flatlist_start; scan2; scan2 = scan2->flat_next)
      {
	if (!strcasecmp(scan2->zchannel, scan->zchannel))
	{
	  count++;
	  scan2->zchannellisted = 1;
	}
      }

      if (count > 0)
      {
	ADDSTACK("%-30s      %d               (%2d:%.2d idle)\n", scan->zchannel, count, scan->zcidle / 60, scan->zcidle % 60);
	noz++;
      }
    }
  }

  if (noz == 0)
  {
    tell_player(p, " There are no Z Channels currently active\n");
    stack = oldstack;
    return;
  }
  ENDSTACK("\n" LINE);

  tell_player(p, oldstack);
  stack = oldstack;
}

/* zw, cw and pw commands */

void z_who(player * p, char *str)
{
  char *oldstack = stack;
  char top[70];

  if (!is_on_zchannel(p))	/* blimey */
    return;

  sprintf(top, "People listening to zchannel '%s'", p->zchannel);
  pstack_mid(top);

  who_on_chan(p, 1);

  tell_player(p, oldstack);
  stack = oldstack;
}

void m_who(player * p, char *str)
{
  char *oldstack = stack;

  pstack_mid("People listening to the main channel");

  who_on_chan(p, 2);

  tell_player(p, oldstack);
  stack = oldstack;
}

void a_who(player * p, char *str)
{
  char *oldstack = stack;
  char midder[160];

  sprintf(midder, "People listening to the %s channel",
	  get_config_msg("admin_name"));

  pstack_mid(midder);

  who_on_chan(p, 3);

  tell_player(p, oldstack);
  stack = oldstack;
}

void s_who(player * p, char *str)
{
  char *oldstack = stack;
  char midder[160];

  sprintf(midder, "People listening to the %s channel",
	  get_config_msg("su_name"));

  pstack_mid(midder);

  who_on_chan(p, 4);

  tell_player(p, oldstack);
  stack = oldstack;
}

void p_who(player * p, char *str)
{
  char *oldstack = stack;

  pstack_mid("People listening to the spod channel");

  who_on_chan(p, 5);

  tell_player(p, oldstack);
  stack = oldstack;
}

#ifdef HC_CHANNEL
void h_who(player * p, char *str)
{
  char *oldstack = stack;
  char midder[160];

  sprintf(midder, "People listening to the %s channel",
	  get_config_msg("hc_name"));

  pstack_mid(midder);

  who_on_chan(p, 6);

  tell_player(p, oldstack);
  stack = oldstack;
}
#endif

void who_on_chan(player * p, int chan)
{
  player *scan;
  int flag = 0;

  sprintf(stack, "\n");
  stack = strchr(stack, 0);

  for (scan = flatlist_start; scan; scan = scan->flat_next)
  {
    if (scan->residency && scan->location)
    {
      flag = 0;

      if (chan == 1 && !strcasecmp(p->zchannel, scan->zchannel))
	flag = 1;

      if (chan == 2 && !(scan->misc_flags & NO_MAIN_CHANNEL))
	flag = 1;

      if (chan == 3 && !(scan->flags & BLOCK_SU) && (scan->residency & LOWER_ADMIN))
	flag = 1;

      if (chan == 4 && (!(scan->flags & BLOCK_SU)) && scan->residency & PSU)
	flag = 1;

      if (chan == 5 && (scan->residency & SPOD) && !(scan->misc_flags & NO_SPOD_CHANNEL))
	flag = 1;

#ifdef HC_CHANNEL
      if (chan == 6 && (!(scan->flags & BLOCK_SU)) && scan->residency & HCADMIN)
	flag = 1;
#endif

      if (flag)
      {
	sprintf(stack, "%s  ", scan->name);
	stack = strchr(stack, 0);
      }
    }
  }
  sprintf(stack, "\n\n"
	  LINE);
  stack = end_string(stack);
}

/* Superuser channel deletion */

void zdel(player * p, char *str)
{
  char *oldstack = stack;
  char *reason;
  player *scan;
  int found = 0;

  if (!(reason = strchr(str, ' ')))
  {
    tell_player(p, " Format: zdel <channel name> <reason>\n"
		"  Reason is logged and also sent to that channel!\n");
    return;
  }
  *reason++ = 0;
  for (scan = flatlist_start; scan; scan = scan->flat_next)
    if (!strcasecmp(scan->zchannel, str) && scan->location)
    {

      sprintf(stack, "<(%s)> -- %s is destroying this channel. Reason: %s --\n",
	      scan->zchannel, p->name, reason);
      stack = end_string(stack);

      if (scan->misc_flags & CHAN_HI)
	command_type |= HIGHLIGHT;
      sys_color_atm = UCEsc;
      tell_player(scan, oldstack);
      sys_color_atm = SYSsc;
      if (scan->misc_flags & CHAN_HI)
	command_type &= ~HIGHLIGHT;

      stack = oldstack;
      strcpy(scan->zchannel, "");
      found = 1;
    }
  if (found == 0)
  {
    tell_player(p, " That channel doesn't exist!\n");
    return;
  }
  SW_BUT(p, " -=*> %s destroys the '%s' channel for the reason: %s\n", p->name, str, reason);
  tell_player(p, " Channel destroyed and reason logged\n");
  LOGF("channel", "%s destroyed '%s'. Reason: %s", p->name, str, reason);
}

/* The muffle command */

void muffle(player * p, char *str)
{
  char *oldstack = stack;
  player *p2 = p;
  char title[70];

  if ((*str && p->residency & LOWER_ADMIN))
  {
    p2 = find_player_global_quiet(str);
    if (p2)
      *str = 0;
  }

  if (!*str)
  {
    if (p2 != p)
      sprintf(title, "Muffle information for %s", p2->name);
    else
      sprintf(title, "Muffle information");

    pstack_mid(title);
    ADDSTACK("\nMain channel                 ");
    if (p2->misc_flags & NO_MAIN_CHANNEL)
      ADDSTACK("<ignoring>\n");
    else
      ADDSTACK("<listening>\n");
    if (p2->residency & SPOD)
    {
      ADDSTACK("Spod channel                 ");
      if (p2->misc_flags & NO_SPOD_CHANNEL)
	ADDSTACK("<ignoring>\n");
      else
	ADDSTACK("<listening>\n");
    }
    ADDSTACK("Sessions                     ");
    if (p2->custom_flags & YES_SESSION)
      ADDSTACK("<off>\n");
    else
      ADDSTACK("<on>\n");
    ADDSTACK("Clock                        ");
    if (p2->custom_flags & NO_CLOCK)
      ADDSTACK("<off>\n");
    else
      ADDSTACK("<on>\n");
    ADDSTACK("Dynatext                     ");
    if (p2->custom_flags & NO_DYNATEXT)
      ADDSTACK("<ignoring>\n");
    else
      ADDSTACK("<seeing>\n");

    if (p2->residency & DEBUG)
    {
      ADDSTACK("Debug channel                ");
      if (p2->misc_flags & SEE_DEBUG_CHANNEL)
	ADDSTACK("<listening>\n");
      else
	ADDSTACK("<ignoring>\n");
    }

    ENDSTACK("\n" LINE);
    tell_player(p, oldstack);
    stack = oldstack;
    return;
  }
  if (strstr(str, "mai"))
    channel_toggle(p, cu_chan, NO_MAIN_CHANNEL);	/* blimey */
  else if (strstr(str, "spo") && p->residency & SPOD)
    channel_toggle(p, pu_chan, NO_SPOD_CHANNEL);	/* blimey */
  else if (strstr(str, "ses"))
    set_yes_session(p, "");
  else if (strstr(str, "clo"))
    toggle_view_clock(p, "");
  else if (strstr(str, "dyn"))
    toggle_yes_dynatext(p, "");
  else if (strstr(str, "deb") && p->residency & DEBUG)
    toggle_debug_channel(p, "");
  else
  {
    tell_player(p, " Format: muffle <section>\n"
		"  Where section can be: main");
    if (p->residency & SPOD)
      tell_player(p, ", spod");
    if (p->residency & DEBUG)
      tell_player(p, ", debug");
    tell_player(p, ", session, clock, dynatext\n");
  }
}

/* Version information */
void channel_version()
{
  sprintf(stack, " -=*> Channel code v2.0 (by Silver and blimey) enabled.\n");
  stack = strchr(stack, 0);
}
