/*
 * Playground+ - examine.c
 * Examine, finger, z and y commands
 * ---------------------------------------------------------------------------
 */
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>

#include "include/config.h"
#include "include/player.h"
#include "include/fix.h"
#include "include/proto.h"

file ConnectionSpeeds[] =
{
  {"Pending", -1},
  {"Direct", 20},
  {"Blazing", 30},
  {"Fast", 60},
  {"Speedy", 80},
  {"Decent", 100},
  {"Sluggish", 120},
  {"Slow", 150},
  {"Painful", 180},
  {"Constipated", 220},
  {"Unbearable", 300},
  {"Horrid", 500},
  {"Lag Attack", 999999},
  {"", 0}
};

/* show what somone can do */

void privs(player * p, char *str)
{

  char *oldstack = stack, name[MAX_NAME + 2], middle[80];
  int priv, who = 0;
  player *p2 = 0;
  player dummy;

  /* assume you are looking at your privs */
  strcpy(name, " You");

  /* convert possible name to lower case */
  lower_case(str);
  /* check if the person executing it is an SU */
  if (*str && p->residency & (SU | ADMIN))
  {
    /* look for person on program - this will report if they
       are logged out at the time (ie if it fails) */
    p2 = find_player_global(str);
    if (!p2)
      /* if player is logged out, try and load them into dummy */
    {
      /* setup name */
      strcpy(dummy.lower_name, str);
      lower_case(dummy.lower_name);
      /* and an fd for messages */
      dummy.fd = p->fd;
      /* actually try loading them */
      if (!load_player(&dummy))
	/* if we can't load them, report abject failure and exit */
      {
	tell_player(p, " Couldn't find player in saved files.\n");
	return;
      }
      /* lets see if this fixes banished privs shit.. */
      if (dummy.residency & BANISHD)
      {
	tell_player(p, " That is a banished name or player.\n");
	return;
      }
      /* otherwise set p2 so the gender stuff below works */
      p2 = &dummy;
    }
    /* setup name */
    strcpy(name, p2->name);
    /* and privs */
    priv = p2->residency;
    /* flag it as another person's privs */
    who = 1;
    /* print the title to the stack */
    sprintf(middle, "Permissions for %s", name);
    pstack_mid(middle);
  }
  else
  {
    /* the person wants their own privs so get person's own privs :-) */
    priv = p->residency;
    pstack_mid("Your permissions");
  }

  /* capitalise name again */
  name[0] = toupper(name[0]);

  if (priv == NON_RESIDENT)
  {
    TELLPLAYER(p, "%s will not be saved... not a resident, you see..\n", name);
    stack = oldstack;
    return;
  }
  if (priv == SYSTEM_ROOM)
  {
    TELLPLAYER(p, "%s is a System Room\n", name);
    stack = oldstack;
    return;
  }
  if (who == 0)
    /* privs for yourself */
  {
    if (priv & BASE)
      ADDSTACK(" You are a resident.\n");
    else
      ADDSTACK(" You aren't a resident! EEK! Talk to a superuser!\n");

#ifdef ROBOTS
    if (priv & ROBOT_PRIV)
      ADDSTACK(" You are a robot.\n");
#endif

    if (priv & LIST)
      ADDSTACK(" You have a list.\n");
    else
      ADDSTACK(" You do not have a list.\n");

    if (priv & ECHO_PRIV)
      ADDSTACK(" You can echo.\n");
    else
      ADDSTACK(" You cannot echo.\n");

    if (priv & BUILD)
      ADDSTACK(" You can use room commands.\n");
    else
      ADDSTACK(" You can't use room commands.\n");

    if (priv & MAIL)
      ADDSTACK(" You can send mail.\n");
    else
      ADDSTACK(" You cannot send any mail.\n");

    if (priv & SESSION)
      ADDSTACK(" You can set sessions.\n");
    else
      ADDSTACK(" You cannot set sessions.\n");

    if (p->system_flags & MARRIED)
      ADDSTACK(" You are happily net-married to %s.\n", p->married_to);
    if (p->residency & MINISTER)
      ADDSTACK(" You can perform net-marriages.\n");
    if (p->residency & BUILDER)
      ADDSTACK(" You can create elaborate objects.\n");
    if (p->residency & SPECIALK)
      ADDSTACK(" You can fashion together original socials.\n");
    if (priv & SPOD)
      ADDSTACK(" You are a spod (but you already KNEW that).\n");
    if (priv & DEBUG)
      ADDSTACK(" You can see the debug channel.\n");
    if (priv & NO_TIMEOUT)
      ADDSTACK(" You will never time-out.\n");
    if (priv & WARN)
      ADDSTACK(" You can warn people.\n");
    if (priv & DUMB)
      ADDSTACK(" You can dumb people (turn em into tweedles).\n");
    if (priv & SCRIPT)
      ADDSTACK(" You can use extended scripting.\n");
    if (priv & TRACE)
      ADDSTACK(" You can trace login sites.\n");

    /* added a lil tiny extra for those talkers that get creative
       with there privs so staff will know exactly where they stand
     */
    ADDSTACK(" Your residency level is ");
    if (priv & CODER)
      ADDSTACK("%s (coder).\n", get_config_msg("coder_name"));
    else if (priv & HCADMIN)
      ADDSTACK("%s (hcadmin).\n", get_config_msg("hc_name"));
    else if (priv & ADMIN)
      ADDSTACK("%s (admin).\n", get_config_msg("admin_name"));
    else if (priv & LOWER_ADMIN)
      ADDSTACK("%s (lower admin).\n", get_config_msg("la_name"));
    else if (priv & ASU)
      ADDSTACK("%s (advanced su).\n", get_config_msg("asu_name"));
    else if (priv & SU)
      ADDSTACK("%s (super user).\n", get_config_msg("su_name"));
    else if (priv & PSU)
      ADDSTACK("%s (pseudo su).\n", get_config_msg("psu_name"));
    else
      ADDSTACK("standard resident.\n");

    /* in following with the tradition of changing this 
       as new releases are done.... */
    if (priv & HOUSE)
      ADDSTACK(" You are one with the inner child\n");

  }
  if (who == 1)
    /* privs for someone else */
  {
    {
      if (priv & BASE)
	ADDSTACK("%s is a resident.\n", name);
      else
	ADDSTACK("%s is not resident! EEK!\n", name);

#ifdef ROBOTS
      if (priv & ROBOT_PRIV)
	ADDSTACK("%s is a robot.\n", name);
#endif
      if (priv & LIST)
	ADDSTACK("%s has a list.\n", name);
      else
	ADDSTACK("%s hasno list.\n", name);

      if (priv & ECHO_PRIV)
	ADDSTACK("%s can echo.\n", name);
      else
	ADDSTACK("%s cannot echo.\n", name);

      if (priv & BUILD)
	ADDSTACK("%s can use room commands.\n", name);
      else
	ADDSTACK("%s can't use room commands.\n", name);

      if (priv & MAIL)
	ADDSTACK("%s can send mail.\n", name);
      else
	ADDSTACK("%s cannot send any mail.\n", name);

      if (priv & SESSION)
	ADDSTACK("%s can set sessions.\n", name);
      else
	ADDSTACK("%s cannot set sessions.\n", name);

      if (p2->system_flags & MARRIED)
	ADDSTACK("%s is happily net-married to %s.\n", name, p2->married_to);

      if (priv & SPOD)
	ADDSTACK("%s is a sad spod.\n", name);

      if (priv & DEBUG)
	ADDSTACK("%s can see the debug channel.\n", name);

      if (p2->residency & MINISTER)
	ADDSTACK("%s can perform net-marriages.\n", name);
      if (p2->residency & BUILDER)
	ADDSTACK("%s can create amazing items.\n", name);
      if (p2->residency & SPECIALK)
	ADDSTACK("%s can fashion together original socials.\n", name);
      if (priv & GIT)
	ADDSTACK("%s cant do much but sit back and enjoy the view.\n", name);

      if (priv & PROTECT)
	ADDSTACK("%s is carrying a golden parachute.\n", name);
      if (priv & NO_TIMEOUT)
	ADDSTACK("%s will never time-out.\n", name);

      if (priv & WARN)
	ADDSTACK("%s can warn people.\n", name);
      if (priv & DUMB)
	ADDSTACK("%s can dumb people.\n", name);
      if (priv & SCRIPT)
	ADDSTACK("%s can use extended scripting.\n", name);
      if (priv & TRACE)
	ADDSTACK("%s can trace login sites.\n", name);

      ADDSTACK("%s's residency level is ", name);
      if (priv & CODER)
	ADDSTACK("%s (coder).\n", get_config_msg("coder_name"));
      else if (priv & HCADMIN)
	ADDSTACK("%s (hcadmin).\n", get_config_msg("hc_name"));
      else if (priv & ADMIN)
	ADDSTACK("%s (admin).\n", get_config_msg("admin_name"));
      else if (priv & LOWER_ADMIN)
	ADDSTACK("%s (lower admin).\n", get_config_msg("la_name"));
      else if (priv & ASU)
	ADDSTACK("%s (advanced su).\n", get_config_msg("asu_name"));
      else if (priv & SU)
	ADDSTACK("%s (super user).\n", get_config_msg("su_name"));
      else if (priv & PSU)
	ADDSTACK("%s (pseudo su).\n", get_config_msg("psu_name"));
      else
	ADDSTACK("standard resident.\n");

      if (priv & HOUSE)
	ADDSTACK("%s was rode hard and put up wet.\n", name);


      stack += sprintf(stack, "%s            " RES_BIT_HEAD "\n"
		       "Residency   %s\n", LINE,
		       privs_bit_string(p2->residency));

    }
  }
  /* finish off the end of the chunk of data */
  ENDSTACK(LINE "\n");
  tell_player(p, oldstack);
  stack = oldstack;
}


void friend_finger(player * p)
{
  char *oldstack, *temp;
  list_ent *l;
  saved_player *sp;
  player dummy, *p2;
  int jettime, friend = 0;

  memset(&dummy, 0, sizeof(player));
  oldstack = stack;
  if (!p->saved)
  {
    tell_player(p, " You have no save information, and therefore no "
		"friends ...\n");
    return;
  }
  sp = p->saved;
  l = sp->list_top;
  if (!l)
  {
    tell_player(p, " You have no list ...\n");
    return;
  }
  strcpy(stack, "\n Your friends were last seen...\n");
  stack = strchr(stack, 0);
  do
  {
    if (l->flags & FRIEND && strcasecmp(l->name, "everyone"))
    {
      p2 = find_player_absolute_quiet(l->name);
      friend = 1;
      if (p2)
      {
	sprintf(stack, " %s is logged on.\n", p2->name);
	stack = strchr(stack, 0);
      }
      else
      {
	temp = stack;
	strcpy(temp, l->name);
	lower_case(temp);
	strcpy(dummy.lower_name, temp);
	dummy.fd = p->fd;
	if (load_player(&dummy))
	{
	  switch (dummy.residency)
	  {
	    case BANISHED:
	      sprintf(stack, " %s is banished (Old Style)\n",
		      dummy.lower_name);
	      stack = strchr(stack, 0);
	      break;
	    case SYSTEM_ROOM:
	      sprintf(stack, " %s is a system room ...\n", dummy.name);
	      stack = strchr(stack, 0);
	      break;
	    default:
	      if (dummy.residency == BANISHD)
	      {
		sprintf(stack, " %s is banished. (Name Only)\n",
			dummy.lower_name);
		stack = strchr(stack, 0);
	      }
	      else if (dummy.residency & BANISHD)
	      {
		sprintf(stack, " %s is banished.\n", dummy.lower_name);
		stack = strchr(stack, 0);
	      }
	      else
	      {
		if (dummy.saved)
		  jettime = dummy.saved->last_on + (p->jetlag * 3600);
		else
		  jettime = dummy.saved->last_on;
		sprintf(stack, " %s was last seen at %s.\n", dummy.name,
			convert_time(jettime));
		stack = strchr(stack, 0);
	      }
	      break;
	  }
	}
	else
	{
	  sprintf(stack, " %s doesn't exist.\n", l->name);
	  stack = strchr(stack, 0);
	}
      }
    }
    l = l->next;
  }
  while (l);
  if (!friend)
  {
    tell_player(p, " But you have no friends !!\n");
    stack = oldstack;
    return;
  }
  stack = strchr(stack, 0);
  *stack++ = '\n';
  *stack++ = 0;
  pager(p, oldstack);
  stack = oldstack;
  return;
}


/* command to list pinfo about a saved person */

void pinfo_saved_player(player * p, char *str)
{
  player dummy;
  char *oldstack = stack, top[70];

  memset(&dummy, 0, sizeof(player));

  strcpy(dummy.lower_name, str);
  lower_case(dummy.lower_name);
  dummy.fd = p->fd;
  if (!load_player(&dummy))
  {
    tell_player(p, " No such person in saved files.\n");
    return;
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
	return;
      }
      break;
  }


  sprintf(top, "Pinfo for %s (logged out)", dummy.name);
  pstack_mid(top);

  if (emote_no_break(*dummy.enter_msg))
    sprintf(stack, "Entermsg  : %s%s\n", dummy.name, dummy.enter_msg);
  else
    sprintf(stack, "Entermsg  : %s %s\n", dummy.name, dummy.enter_msg);

  stack = strchr(stack, 0);

  if (strlen(dummy.ignore_msg) > 0)
    sprintf(stack, "Ignoremsg : %s\n", dummy.ignore_msg);
  else
    strcpy(stack, "Ignoremsg : \n");
  stack = strchr(stack, 0);

  if (strlen(dummy.logonmsg) > 0)
  {
    if (emote_no_break(*dummy.logonmsg))
      sprintf(stack, "Logonmsg  : %s%s\n", dummy.name, dummy.logonmsg);
    else
      sprintf(stack, "Logonmsg  : %s %s\n", dummy.name, dummy.logonmsg);
  }
  else
    strcpy(stack, "Logonmsg  : \n");

  stack = strchr(stack, 0);

  if (strlen(dummy.logoffmsg) > 0)
  {
    if (emote_no_break(*dummy.logoffmsg))
      sprintf(stack, "Logoffmsg : %s%s\n", dummy.name, dummy.logoffmsg);
    else
      sprintf(stack, "Logoffmsg : %s %s\n",
	      dummy.name, dummy.logoffmsg);
  }
  else
    strcpy(stack, "Logoffmsg : \n");
  stack = strchr(stack, 0);

  if (strlen(dummy.blockmsg) > 0)
  {
    if (emote_no_break(*dummy.blockmsg))
      sprintf(stack, "Blockmsg  : %s%s\n", dummy.name, dummy.blockmsg);
    else
      sprintf(stack, "Blockmsg  : %s %s\n", dummy.name, dummy.blockmsg);
  }
  else
    strcpy(stack, "Blockmsg  : \n");
  stack = strchr(stack, 0);

  if (strlen(dummy.exitmsg) > 0)
  {
    if (emote_no_break(*dummy.exitmsg))
      sprintf(stack, "Exitmsg   : %s%s\n", dummy.name, dummy.exitmsg);
    else
      sprintf(stack, "Exitmsg   : %s %s\n", dummy.name, dummy.exitmsg);
  }
  else
    strcpy(stack, "Exitmsg   : \n");
  stack = strchr(stack, 0);

  strcpy(stack, LINE "\n");
  stack = end_string(stack);

  tell_player(p, oldstack);
  stack = oldstack;
}

void pinfo_command(player * p, char *str)
{
  player *p2;
  char *oldstack;
  char top[70];

  oldstack = stack;
  if ((*str) && (p->residency & SU))
  {
    p2 = find_player_global(str);
    if (!p2)
    {
      stack = oldstack;
      pinfo_saved_player(p, str);
      return;
    }
    else
    {
      sprintf(top, "Pinfo for %s", p2->name);
      pstack_mid(top);
    }
  }
  else
  {
    p2 = p;
    pstack_mid("Your Pinfo");
  }

  if (emote_no_break(*p2->enter_msg))
    sprintf(stack, "Entermsg  : %s%s\n",
	    p2->name, p2->enter_msg);
  else
    sprintf(stack, "Entermsg  : %s %s\n",
	    p2->name, p2->enter_msg);

  stack = strchr(stack, 0);

  if (strlen(p2->ignore_msg) > 0)
    sprintf(stack, "Ignoremsg : %s\n", p2->ignore_msg);
  else
    strcpy(stack, "Ignoremsg : \n");
  stack = strchr(stack, 0);
  if (strlen(p2->logonmsg) > 0)
  {
    if (emote_no_break(*p2->logonmsg))
      sprintf(stack, "Logonmsg  : %s%s\n",
	      p2->name, p2->logonmsg);
    else
      sprintf(stack, "Logonmsg  : %s %s\n",
	      p2->name, p2->logonmsg);
  }
  else
    strcpy(stack, "Logonmsg  : \n");
  stack = strchr(stack, 0);

  if (strlen(p2->logoffmsg) > 0)
  {
    if (emote_no_break(*p2->logoffmsg))
      sprintf(stack, "Logoffmsg : %s%s\n",
	      p2->name, p2->logoffmsg);
    else
      sprintf(stack, "Logoffmsg : %s %s\n",
	      p2->name, p2->logoffmsg);
  }
  else
    strcpy(stack, "Logoffmsg : \n");
  stack = strchr(stack, 0);

  if (strlen(p2->blockmsg) > 0)
  {
    if (emote_no_break(*p2->blockmsg))
      sprintf(stack, "Blockmsg  : %s%s\n",
	      p2->name, p2->blockmsg);
    else
      sprintf(stack, "Blockmsg  : %s %s\n",
	      p2->name, p2->blockmsg);
  }
  else
    strcpy(stack, "Blockmsg  : \n");
  stack = strchr(stack, 0);

  if (strlen(p2->exitmsg) > 0)
  {
    if (emote_no_break(*p2->exitmsg))
      sprintf(stack, "Exitmsg   : %s%s\n",
	      p2->name, p2->exitmsg);
    else
      sprintf(stack, "Exitmsg   : %s %s\n",
	      p2->name, p2->exitmsg);
  }
  else
    strcpy(stack, "Exitmsg   : \n");

  stack = strchr(stack, 0);


  strcpy(stack, LINE);
  stack = strchr(stack, 0);
  *stack++ = 0;
  tell_player(p, oldstack);
  stack = oldstack;
}

void su_examine(player * p, char *str)
{
  player dummy, *p2;
  char *oldstack;
  float partic;
  int jettime;

  oldstack = stack;
  if (!*str)
  {
    tell_player(p, " Format: y <player>\n");
    return;
  }
  if (!strcasecmp(str, "me"))
    p2 = p;
  else
  {
    p2 = find_player_absolute_quiet(str);
    if (!p2)
    {
      strcpy(dummy.lower_name, str);
      lower_case(dummy.lower_name);
      dummy.fd = p->fd;
      if (!load_player(&dummy))
      {
	tell_player(p, " No such person in saved files.\n");
	return;
      }
      p2 = &dummy;
    }
  }
  switch (p2->residency)
  {
    case BANISHED:
      tell_player(p, " That player has been banished from this program.\n");
      return;
    case SYSTEM_ROOM:
      tell_player(p, " That is where some of the standard rooms are stored."
		  "\n");
      return;
    default:
      if (p2->residency == BANISHD)
      {
	tell_player(p, " That name has been banished from this program.\n");
	return;
      }
      else if (p2->residency & BANISHD)
      {
	tell_player(p, " That player has been banished from"
		    " this program.\n");
	return;
      }
  }

  sprintf(stack, LINE
	  "%s %s \n"
	  LINE,
	  p2->name, p2->title);
  stack = strchr(stack, 0);

  if (p2->saved)
  {
    jettime = p2->saved->last_on + (p->jetlag * 3600);
  }
  else
  {
    jettime = 0;
  }
  if (p2 != &dummy)
  {
    sprintf(stack, "%s has been logged in for %s since\n%s.\n",
	    full_name(p2), word_time(time(0) - (p2->on_since)),
	    convert_time(p2->on_since));
  }
  else if (p2->saved)
  {
    if (p->jetlag)
    {
      sprintf(stack, "%s was last seen at %s. (Your time)\n",
	      p2->name, convert_time(jettime));
    }
    else
    {
      sprintf(stack, "%s was last seen at %s.\n", p2->name,
	      convert_time(p2->saved->last_on));
    }
  }
  stack = strchr(stack, 0);

  sprintf(stack, "%s total login time is %s.\n", caps(gstring_possessive(p2)),
	  word_time(p2->total_login));
  stack = strchr(stack, 0);
  sprintf(stack, "%s total adjusted spod time is %s.\n", caps(gstring_possessive(p2)),
	  word_time(p2->total_login - p2->total_idle_time));
  stack = strchr(stack, 0);

  if (p2->residency & BASE)
  {
    sprintf(stack, "%s was ressied by %s on %s\n",
	    p2->name, p2->ressied_by, convert_time(p2->first_login_date));
  }
  stack = strchr(stack, 0);

  if (p2->time_in_main && p2->total_login)
  {
    partic = ((float) p2->time_in_main / (float) p2->total_login) * 100;
    if (partic >= 100)
      sprintf(stack, "%s has spent 100%% of the time in a main room.\n", p2->name);
    else
      sprintf(stack, "%s has spent %.2f%% of the time in a main room.\n", p2->name, partic);
  }
  else
    sprintf(stack, "%s has spent no time at all in main rooms.\n", p2->name);

  stack = strchr(stack, 0);

  if (p2->warn_count)
  {
    sprintf(stack, "%s has been warned %d times.\n",
	    p2->name, p2->warn_count);
  }
  stack = strchr(stack, 0);
  if (p2->booted_count)
  {
    sprintf(stack, "%s has been booted or jailed %d times.\n",
	    p2->name, p2->booted_count);
  }
  stack = strchr(stack, 0);
  if (p2->idled_out_count)
  {
    sprintf(stack, "%s has idled out of the program %d times.\n",
	    p2->name, p2->idled_out_count);
  }
  stack = strchr(stack, 0);
  if (p2->eject_count)
  {
    sprintf(stack, "%s has been kicked off the program %d times.\n",
	    p2->name, p2->eject_count);
  }
  stack = strchr(stack, 0);

  if (p2->system_flags & SAVE_NO_SING)
  {
    sprintf(stack, "%s cannot sing -- ever.\n", p2->name);
  }
  else if (p2 != &dummy && p2->no_sing)
  {
    sprintf(stack, "%s cannot sing for %d more seconds.\n",
	    p2->name, p2->no_shout);
  }
  stack = strchr(stack, 0);

  if (p2->system_flags & SAVENOSHOUT)
  {
    sprintf(stack, "%s cannot shout -- ever.\n", p2->name);
  }
  else if (p2 != &dummy && p2->no_shout)
  {
    sprintf(stack, "%s cannot shout for %d more seconds.\n",
	    p2->name, p2->no_shout);
  }
  stack = strchr(stack, 0);

  if (p2->system_flags & SAVEDFROGGED)
  {
    sprintf(stack, "%s is currently \"dum\"med down.\n", p2->name);
  }
  stack = strchr(stack, 0);

  if (p2->system_flags & NO_MSGS)
  {
    sprintf(stack, "%s cannot change pinfo, x or f data.\n", p2->name);
  }
  stack = strchr(stack, 0);
  if (p2->system_flags & DECAPPED)
  {
    sprintf(stack, "%s cannot use capital letters.\n", p2->name);
  }
  stack = strchr(stack, 0);

  if (strlen(p2->swarn_sender) > 0)
  {
    sprintf(stack, "%s has a saved warning from %s: %s.\n", p2->name, p2->swarn_sender, p2->swarn_message);
  }
  stack = strchr(stack, 0);

  if (p->residency & LOWER_ADMIN)
  {
    if (p2->num_ressied)
    {
      sprintf(stack, "%s has granted residency to %d people.\n",
	      p2->name, p2->num_ressied);
    }
    stack = strchr(stack, 0);
    if (p2->num_warned)
    {
      sprintf(stack, "%s has warned %d people.\n",
	      p2->name, p2->num_warned);
    }
    stack = strchr(stack, 0);
    if (p2->num_ejected)
    {
      sprintf(stack, "%s has kicked %d gits off the program.\n",
	      p2->name, p2->num_ejected);
    }
    stack = strchr(stack, 0);

    if (p2->num_rmd)
    {
      sprintf(stack, "%s removed %d shouts, sings and moves.\n",
	      p2->name, p2->num_rmd);
    }
    stack = strchr(stack, 0);

    if (p2->num_booted)
    {
      sprintf(stack, "%s booted or jailed %d morons.\n",
	      p2->name, p2->num_booted);
    }
    stack = strchr(stack, 0);
  }

  if (p2->git_string[0])
  {
    pstack_mid(p2->git_by);
    sprintf(stack, "%s\n", p2->git_string);
    stack = strchr(stack, 0);
  }
  strcpy(stack, LINE);
  stack = end_string(stack);
  pager(p, oldstack);
  stack = oldstack;
}

void newfinger(player * p, char *str)
{
  player dummy, *p2;
  char *oldstack = stack, datastring[50];
  int jettime, overtime;
  float partic;
  list_ent *l = 0;

#ifdef INTERCOM
  if (strchr(str, '@'))
  {
    do_intercom_finger(p, str);
    return;
  }
#endif

  if (!strcasecmp(str, "friends"))
  {
    friend_finger(p);
    return;
  }
  if (!*str || !strcasecmp(str, "me"))
    p2 = p;
  else
  {
    if (reserved_name(str))
    {
      tell_player(p, " That is a reserved name.\n");
      return;
    }
    p2 = find_player_absolute_quiet(str);
    if (!p2)
    {
      strcpy(dummy.lower_name, str);
      lower_case(dummy.lower_name);
      dummy.fd = p->fd;
      if (!load_player(&dummy))
      {
	if (!p->location)
	  if (!*str)
	    tell_player(p, " You need to specify a name in order to finger someone!\n");
	  else
 	    TELLPLAYER(p, " No resident of the name '%s' found in files.\n", str);
	  return;
      }

      p2 = &dummy;
    }
  }
  if (p2->residency & LIST && !(p2->residency & NO_SYNC))
    l = fle_from_save(p2->saved, p->lower_name);
  switch (p2->residency)
  {
    case BANISHED:
      tell_player(p, " That player has been banished from this program.\n");
      if (!(p->residency & SU))
	return;
      break;
    case SYSTEM_ROOM:
      tell_player(p, " That is where some of the standard rooms are stored."
		  "\n");
      return;
    default:
      if (p2->residency == BANISHD)
      {
	tell_player(p, " That name has been banished from this program.\n");
	return;
      }
      else if (p2->residency & BANISHD)
      {
	tell_player(p, " That player has been banished from"
		    " this program.\n");
	if (!(p->residency & SU))
	  return;
      }
  }

  if (*p2->pretitle)
  {
    if (emote_no_break(*p2->title))
      ADDSTACK(LINE "%s %s%s^N \n"
	       LINE, p2->pretitle, p2->name, p2->title);
    else
      ADDSTACK(LINE "%s %s %s^N \n"
	       LINE, p2->pretitle, p2->name, p2->title);
  }
  else
    ADDSTACK(LINE "%s %s^N \n"
	     LINE, p2->name, p2->title);

  if (p->jetlag)
  {
    overtime = p2->on_since + (p->jetlag * 3600);
    strcpy(datastring, "(Your time)");
  }
  else
  {
    overtime = p2->on_since;
    sprintf(datastring, "(%s time)", get_config_msg("talker_name"));
  }

  if (p2->saved)
  {
    jettime = p2->saved->last_on + (p->jetlag * 3600);
  }
  else
  {
    jettime = 0;
  }
  if (p2 != &dummy)
  {
    ADDSTACK("Time on so far      : %s\nLogged on at        : %s %s\n",
    word_time(time(0) - (p2->on_since)), convert_time(overtime), datastring);
  }
  else if (p2->saved)
  {
    if (p->jetlag)
    {
      ADDSTACK("Date last logged on : %s %s\n",
	       convert_time(jettime), datastring);
    }
    else
    {
      ADDSTACK("Date last logged on : %s %s\n",
	       convert_time(p2->saved->last_on), datastring);
    }
  }
  ADDSTACK("Total login time    : %s.\n", word_time(p2->total_login));

  /* This is a (very poor) fix when truespod time becomes negative */
  if (p2->total_idle_time > p2->total_login)
    p2->total_idle_time = 0;

  if (config_flags & cfUSETRUESPOD)
    ADDSTACK("Truespod login time : %s.\n",
	     word_time(p2->total_login - p2->total_idle_time));

  calc_spodlist();
  ADDSTACK("Spodlist position   : %d (out of %d)\n", find_spodlist_position(p2->name), people_in_spodlist());

  if (p2->total_login && p2->first_login_date)
  {
    partic = ((float) p2->total_login / (float) (time(0) - p2->first_login_date)) * 100;
    if (partic < 100)
      ADDSTACK("Chronic spod factor : %.2f%%\n", partic);
    else
      ADDSTACK("Chronic spod factor : 100%%\n");
  }
  if (p2->age)
    ADDSTACK("Years of age        : %d\n", p2->age);
  if (p2->birthday)
    ADDSTACK("Date of birth       : %s\n", birthday_string(p2->birthday));
  if (p2->system_flags & NEW_MAIL)
    ADDSTACK("Mailbox status      : New mail received\n");

  if (p2->residency && (p == p2 || p->residency & (LOWER_ADMIN | ADMIN)
			|| !(p2->custom_flags & PRIVATE_EMAIL) ||
	       (l && l->flags & FRIEND && p2->custom_flags & FRIEND_EMAIL)))
  {
    if (!(p2->email[0]))
      ADDSTACK("Email address       : Not set.\n");
    else if (p2->email[0] == ' ')
      ADDSTACK("Email address       : Set as validated.\n");
    else
    {
      ADDSTACK("Email address       : %s", p2->email);
      if (p2->custom_flags & FRIEND_EMAIL)
	ADDSTACK(" (friends)\n");
      else if (!(p2->custom_flags & PRIVATE_EMAIL))
	ADDSTACK(" \n");
      else
	ADDSTACK(" (private)\n");
    }
  }
  /* alt_email stolen for URL -- you probably knew that tho :P */
  if (p2->alt_email[0])
    ADDSTACK("WWW homepage URL    : %s^N \n", p2->alt_email);
  if (p2->icq)
    ADDSTACK("ICQ number          : %d\n", p2->icq);
  if (p2->hometown[0])
    ADDSTACK("Place of residency  : %s^N \n", p2->hometown);
  if (p2->residency & (BUILDER | MINISTER | SPECIALK | SU | ADMIN | SPOD))
    ADDSTACK("Online Positions    : ");
  if (p2->residency & SPOD)
    ADDSTACK("Spod ");
  if (p2->residency & MINISTER)
    ADDSTACK("Minister ");
  if (p2->residency & BUILDER)
    ADDSTACK("Builder ");
  if (p2->residency & SPECIALK)
    ADDSTACK("Creator ");
  if (p2->residency & ADMIN)
    ADDSTACK("Administrator ");
  else if (p2->residency & SU)
    ADDSTACK("Staff ");
  if (p2->residency & (BUILDER | MINISTER | SPECIALK | SU | ADMIN | SPOD))
    ADDSTACK("\n");

  if (p2->system_flags & (MARRIED | FLIRT_BACHELOR | ENGAGED) || !(p2->system_flags & BACHELOR_HIDE))
  {
    ADDSTACK("Marital status      : ");
    stack = strchr(stack, 0);
    if (p2->system_flags & MARRIED)
      ADDSTACK("Happily net.married to %s\n", p2->married_to);
    else if (p2->system_flags & ENGAGED)
      ADDSTACK("Net.engaged to %s\n", p2->married_to);
    else if (p2->system_flags & FLIRT_BACHELOR)
      ADDSTACK("Horrible net.flirt =)\n");
    else if (p2->gender == FEMALE)
      ADDSTACK("Swinging net.bachelorette\n");
    else
      ADDSTACK("Swinging net.bachelor\n");
  }

  if (p2->ingredients[0])
  {
    switch (p2->gender)
    {
      case FEMALE:
	ADDSTACK("She is made from    : %s^N\n", p2->ingredients);
	break;
      case MALE:
	ADDSTACK("He is made from     : %s^N\n", p2->ingredients);
	break;
      default:
	ADDSTACK("It is made from     : %s^N\n", p2->ingredients);
	break;
    }
  }
  else
  {
    switch (p2->gender)
    {
      case FEMALE:
	ADDSTACK("She is made from    : %s^N\n", get_config_msg("female_made"));
	break;
      case MALE:
	ADDSTACK("He is made from     : %s^N\n", get_config_msg("male_made"));
	break;
      default:
	ADDSTACK("It is made from     : %s^N\n", get_config_msg("neuter_made"));
	break;
    }
  }
  if (p2->plan[0])
  {
    pstack_mid("plan");
    ADDSTACK("%s^N \n", p2->plan);
  }
  if (p2->finger_message[0])
  {
    pstack_mid("finger message");
    ADDSTACK("%s^N \n", p2->finger_message);
  }
  ENDSTACK(LINE);
  tell_player(p, oldstack);
  stack = oldstack;

  if (config_flags & cfSHOWXED)
    if (p->location && p != p2 && p2 != &dummy &&
	!(p->residency & (ADMIN | CODER | HCADMIN)) &&
	strcasecmp(p->name, "someone@intercom"))
      TELLPLAYER(p2, " (%s just took a peek at your finger file)\n",
		 p->name);
}

/* ping information */

char *ping_string(player * p)
{
  int i;

  for (i = 0; ConnectionSpeeds[i].where[0]; i++)
    if ((p->last_ping / 10000) <= (long) ConnectionSpeeds[i].length)
      return ConnectionSpeeds[i].where;

  return "Spanked!";
}


/* the examine command */

void newexamine(player * p, char *str)
{
  player *p2;
  char *oldstack = stack;
  char first[MAX_SPODCLASS], *second;
  list_ent *l = 0;

#ifdef INTERCOM
  if (strchr(str, '@'))
  {
    do_intercom_examine(p, str);
    return;
  }
#endif

  if (!*str || !strcasecmp("me", str))
    p2 = p;
  else
    p2 = find_player_global(str);
  if (!p2)
    return;
  if (p2->saved && p2->residency & LIST && !(p2->residency & NO_SYNC))
    l = fle_from_save(p2->saved, p->lower_name);
  else
    l = 0;

  if (p2->description[0])
  {
    ADDSTACK(LINE);
    if (*p2->pretitle)
      ADDSTACK("%s ", p2->pretitle);
    if (emote_no_break(*p2->title))
      ADDSTACK("%s%s^N \n", p2->name, p2->title);
    else
      ADDSTACK("%s %s^N \n", p2->name, p2->title);
    pstack_mid("Description");
    ADDSTACK("%s^N \n"
	     LINE, p2->description);
  }
  else
  {
    if (emote_no_break(*p2->title))
      ADDSTACK(LINE
	       "%s %s%s^N \n" LINE, p2->pretitle, p2->name, p2->title);
    else
      ADDSTACK(LINE
	       "%s %s %s^N \n" LINE, p2->pretitle, p2->name, p2->title);
  }
  if (p == p2 || p->residency & SU || p2->custom_flags & PUBLIC_SITE ||
      (l && l->flags & FRIEND && p2->custom_flags & FRIEND_SITE))
  {
    ADDSTACK("Site logged on from   : %s (%s)", get_address(p2, p), p2->num_addr);
    if (p2->custom_flags & FRIEND_SITE)
      ADDSTACK(" (friends)\n");
    else if (p2->custom_flags & PUBLIC_SITE)
      ADDSTACK(" \n");
    else
      ADDSTACK(" (private)\n");
  }
  /* Connection speed */
  ADDSTACK("Connection speed      : %s (%ld.%02ld seconds lag time)\n", ping_string(p2), p2->last_ping / 1000000, (p2->last_ping / 10000) % 1000000);

  if (p->jetlag)
    ADDSTACK("Time logged on so far : %s\n"
	     "Time logged in        : %s (Your time)\n",
	     word_time(time(0) - (p2->on_since)),
	     convert_time((p2->on_since + (p->jetlag * 3600))));
  else
    ADDSTACK("Time logged on so far : %s\n"
	     "Time logged in        : %s (%s time)\n",
	     word_time(time(0) - (p2->on_since)), convert_time(p2->on_since),
	     get_config_msg("talker_name"));

  ADDSTACK("Total login time      : %s\n", word_time(p2->total_login));

  /* A (very poor) fix when truespod time becomes negative */
  if (p2->total_idle_time > p2->total_login)
    p2->total_idle_time = 0;

  if (config_flags & cfUSETRUESPOD)
    ADDSTACK("Truespod login time   : %s\n",
	     word_time(p2->total_login - p2->total_idle_time));

  calc_spodlist();
  ADDSTACK("Spodlist position     : %d (out of %d)\n", find_spodlist_position(p2->name), people_in_spodlist());

  if (p2->tag_flags & (BLOCK_TELLS | BLOCK_SHOUT | BLOCK_FRIENDS | SINGBLOCK))
  {
    ADDSTACK("Block modes active    : ");
    if (p2->tag_flags & BLOCK_SHOUT)
      ADDSTACK("shouts, ");
    if (p2->tag_flags & BLOCK_TELLS)
      ADDSTACK("tells, ");
    if (p2->tag_flags & BLOCK_FRIENDS && !(p2->tag_flags & BLOCK_TELLS))
      ADDSTACK("friend tells, ");
    if (p2->tag_flags & SINGBLOCK)
      ADDSTACK("singing, ");
    stack -= 2;
    *stack++ = '.';
    *stack++ = '\n';
  }
  if (p2->age)
  {
    ADDSTACK("Years of age          : %d\n", p2->age);
  }
  if (p2->birthday)
  {
    ADDSTACK("Date of birth         : %s\n", birthday_string(p2->birthday));
  }
  if (p2->residency && p2->saved && (p == p2 || p->residency & (LOWER_ADMIN | ADMIN)
				   || !(p2->custom_flags & PRIVATE_EMAIL) ||
	       (l && l->flags & FRIEND && p2->custom_flags & FRIEND_EMAIL)))
  {

    if (!(p2->email[0]))
      ADDSTACK("Email address         : None set.\n");
    else if (p2->email[0] == ' ')
      ADDSTACK("Email address         : Validated.\n");
    else
    {
      ADDSTACK("Email address         : %s", p2->email);
      if (p2->custom_flags & FRIEND_EMAIL)
	ADDSTACK(" (friends)\n");
      else if (!(p2->custom_flags & PRIVATE_EMAIL))
	ADDSTACK(" \n");
      else
	ADDSTACK(" (private)\n");
    }
  }
  if (p2->icq)
    ADDSTACK("ICQ number            : %d\n", p2->icq);
  if (p2->alt_email[0])
    ADDSTACK("WWW homepage URL      : %s^N \n", p2->alt_email);
  if (p2->irl_name[0])
    ADDSTACK("Also known as (irl)   : %s^N \n", p2->irl_name);
  if (p2->prs_record)
  {
    ADDSTACK("Paper Rock Scissors   : ");
    prs_record_display(p2);
  }
  if (p2->ttt_win + p2->ttt_loose + p2->ttt_draw != 0)
    ADDSTACK("Tic Tac Toe           : %d wins, %d losses, %d ties\n", p2->ttt_win, p2->ttt_loose, p2->ttt_draw);

  if (p2->residency & (BUILDER | MINISTER | SPECIALK | SU | ADMIN | SPOD))
    ADDSTACK("Online Positions      : ");
  if (p2->residency & SPOD)
    ADDSTACK("Spod ");
  if (p2->residency & MINISTER)
    ADDSTACK("Minister ");
  if (p2->residency & BUILDER)
    ADDSTACK("Builder ");
  if (p2->residency & SPECIALK)
    ADDSTACK("Creator ");
  if (p2->residency & ADMIN)
    ADDSTACK("Administrator ");
  else if (p2->residency & SU)
    ADDSTACK("Staff ");
  if (p2->residency & (BUILDER | MINISTER | SPECIALK | SU | ADMIN | SPOD))
    ADDSTACK("\n");

  if (p2->system_flags & (MARRIED | FLIRT_BACHELOR | ENGAGED)
      || !(p2->system_flags & BACHELOR_HIDE))
  {
    ADDSTACK("Marital status        : ");
    if (p2->system_flags & MARRIED)
      ADDSTACK("Happily net.married to %s\n", p2->married_to);
    else if (p2->system_flags & ENGAGED)
      ADDSTACK("Net.engaged to %s\n", p2->married_to);
    else if (p2->system_flags & FLIRT_BACHELOR)
      ADDSTACK("Horrible net.flirt =)\n");
    else if (p2->gender == FEMALE)
      ADDSTACK("Swinging net.bachelorette\n");
    else
      ADDSTACK("Swinging net.bachelor\n");
  }
  strcpy(first, "");
  second = 0;
  if (p2->favorite1[0])
  {
    strcpy(first, p2->favorite1);
    second = next_space(first);
    *second++ = 0;
    ADDSTACK("Favorite %-12.12s : %s^N \n", first, second);
  }
  strcpy(first, "");
  second = 0;
  if (p2->favorite2[0])
  {
    strcpy(first, p2->favorite2);
    second = next_space(first);
    *second++ = 0;
    ADDSTACK("Favorite %-12.12s : %s^N \n", first, second);
  }
  strcpy(first, "");
  second = 0;
  if (p2->favorite3[0])
  {
    strcpy(first, p2->favorite3);
    second = next_space(first);
    *second++ = 0;
    ADDSTACK("Favorite %-12.12s : %s^N \n", first, second);
  }
  if (p2->hometown[0])
    ADDSTACK("Place of residence    : %s^N \n", p2->hometown);

  if (p2->ingredients[0])
  {
    switch (p2->gender)
    {
      case FEMALE:
	ADDSTACK("She is made from      : %s^N\n", p2->ingredients);
	break;
      case MALE:
	ADDSTACK("He is made from       : %s^N\n", p2->ingredients);
	break;
      default:
	ADDSTACK("It is made from       : %s^N\n", p2->ingredients);
	break;
    }
  }
  else
  {
    switch (p2->gender)
    {
      case FEMALE:
	ADDSTACK("She is made from      : %s^N\n", get_config_msg("female_made"));
	break;
      case MALE:
	ADDSTACK("He is made from       : %s^N\n", get_config_msg("male_made"));
	break;
      default:
	ADDSTACK("It is made from       : %s^N\n", get_config_msg("neuter_made"));
	break;
    }
  }
  check_clothing(p2);
  ENDSTACK(LINE);
  tell_player(p, oldstack);
  stack = oldstack;

  if (config_flags & cfSHOWXED)
    if (p != p2 && !(p->residency & (ADMIN | CODER | HCADMIN)) && strcasecmp(p->name, "someone@intercom"))
      TELLPLAYER(p2, " (%s just took a peek at your examine)\n",
		 p->name);

}

/* Dynamic Staff List by Silver */

int most_highest_priv(saved_player * sp)
{
  if (sp->residency & CODER)
    return CODER;
  if (sp->residency & HCADMIN)
    return HCADMIN;
  if (sp->residency & ADMIN)
    return ADMIN;
  if (sp->residency & LOWER_ADMIN)
    return LOWER_ADMIN;
  if (sp->residency & ASU)
    return ASU;
  if (sp->residency & SU)
    return SU;
  if (sp->residency & PSU)
    return PSU;

  return BASE;
}

/* thought this would make things nice... ~phypor */

int greatest_common_factor(int x, int y)
{
  int factor_x[51] =
  {0};
  int factor_y[51] =
  {0};
  int c, i;

  /* factorize x */
  for (i = 0, c = x; (c > 0 && i < 50); c--)
    if (!(x % c))
      factor_x[i++] = c;

  /* factorize y */
  for (i = 0, c = y; (c > 0 && i < 50); c--)
    if (!(y % c))
      factor_y[i++] = c;
  i++;

  /* get the gfc */
  for (i = 0; i < 50; i++)
    for (c = 0; c < 50; c++)
      if (factor_x[i] == factor_y[c])
	return factor_x[i];

  return 1;
}

void staff_list(player * p, char *str)
{
  char *oldstack = stack;
  char temp[70];
  int i = 1, counter, charcounter, flag = 0, len, numres = 0, numstaff = 0;
  int ratio_staff = 0, ratio_res = 0, gcf, maxi = 8;
  saved_player *scanlist, **hlist;

  sprintf(temp, "%s Staff", get_config_msg("talker_name"));
  pstack_mid(temp);

  sprintf(stack, "\n");
  stack = strchr(stack, 0);

  /* Only psu's and above can see psu's listed on "staff" */

  if (!(p->residency & (PSU | SU | ADMIN)))
    maxi = 7;

  for (i = 1; i < maxi; i++)
  {
    switch (i)
    {
      case 1:
	sprintf(stack, "%-18.18s - ", get_config_msg("hc_name"));
	stack = strchr(stack, 0);
	break;
      case 2:
	sprintf(stack, "%-18.18s - ", get_config_msg("coder_name"));
	stack = strchr(stack, 0);
	break;
      case 3:
	sprintf(stack, "%-18.18s - ", get_config_msg("admin_name"));
	stack = strchr(stack, 0);
	break;
      case 4:
	sprintf(stack, "%-18.18s - ", get_config_msg("la_name"));
	stack = strchr(stack, 0);
	break;
      case 5:
	sprintf(stack, "%-18.18s - ", get_config_msg("asu_name"));
	stack = strchr(stack, 0);
	break;
      case 6:
	sprintf(stack, "%-18.18s - ", get_config_msg("su_name"));
	stack = strchr(stack, 0);
	break;
      case 7:
	sprintf(stack, "%-18.18s - ", get_config_msg("psu_name"));
	stack = strchr(stack, 0);
	break;
    }

    flag = 0;
    len = 22;
    numres = 0;

    for (charcounter = 0; charcounter < 26; charcounter++)
    {
      hlist = saved_hash[charcounter];
      for (counter = 0; counter < HASH_SIZE; counter++, hlist++)
	for (scanlist = *hlist; scanlist; scanlist = scanlist->next)
	{
	  switch (scanlist->residency)
	  {
	    case SYSTEM_ROOM:
	    case BANISHED:
	    case BANISHD:
	      break;
	    default:
	      numres++;
/*
   if (((scanlist->residency & HCADMIN) && i == 1 &&
   !(scanlist->residency & CODER)) ||
   ((scanlist->residency & CODER) && i == 2) ||
   ((scanlist->residency & ADMIN) && i == 3 &&
   !(scanlist->residency & HCADMIN) &&
   !(scanlist->residency & CODER)) ||
   ((scanlist->residency & LOWER_ADMIN) && i == 4 &&
   !(scanlist->residency & ADMIN) &&
   !(scanlist->residency & CODER)) ||
   ((scanlist->residency & ASU) && i == 5 &&
   !(scanlist->residency & LOWER_ADMIN) &&
   !(scanlist->residency & CODER)) ||
   ((scanlist->residency & SU) && i == 6 &&
   !(scanlist->residency & ASU) &&
   !(scanlist->residency & CODER)) ||
   ((scanlist->residency & PSU) && i == 7 &&
   !(scanlist->residency & SU) &&
   !(scanlist->residency & CODER)))
 */
	      if ((i == 1 && most_highest_priv(scanlist) == HCADMIN) ||
		  (i == 2 && most_highest_priv(scanlist) == CODER) ||
		  (i == 3 && most_highest_priv(scanlist) == ADMIN) ||
		  (i == 4 && most_highest_priv(scanlist) == LOWER_ADMIN) ||
		  (i == 5 && most_highest_priv(scanlist) == ASU) ||
		  (i == 6 && most_highest_priv(scanlist) == SU) ||
		  (i == 7 && most_highest_priv(scanlist) == PSU))
	      {
		len += strlen(scanlist->lower_name) + 2;
		if (len > 65)
		{
		  sprintf(stack, "\n                     ");	/* 22 spaces */
		  stack = strchr(stack, 0);
		  len = 22;
		}
		sprintf(stack, "%s, ", check_legal_entry(p, scanlist->lower_name, 0));
		stack = strchr(stack, 0);
		flag = 1;
		numstaff++;
	      }
	      break;
	  }
	}
    }

    if (flag)
    {
      stack--;
      stack--;
      sprintf(stack, ".\n");
    }
    else
      sprintf(stack, "\n");
    stack = strchr(stack, 0);
  }

  sprintf(stack, "\n");
  stack = strchr(stack, 0);

  /* Calculate ratios */

/* this got the lowest common factor 
   ratio_staff = numstaff;
   ratio_res = numres;

   for (i = 2; i <= numstaff; i++)
   {
   flag = 1;
   while (flag)
   {
   if (ratio_res % i == 0 && ratio_staff % i == 0)
   {
   ratio_res /= i;
   ratio_staff /= i;
   flag = 1;
   }
   else
   flag = 0;
   }
   }
 */
  if (numstaff != 0)
  {
    gcf = greatest_common_factor(numres, numstaff);
    ratio_staff = numstaff / gcf;
    ratio_res = numres / gcf;
  }

  sprintf(temp, "%d staff listed (staff to resident ratio is %d:%d)", numstaff, ratio_staff, ratio_res);
  pstack_mid(temp);
  *stack++ = 0;

  pager(p, oldstack);
  stack = oldstack;
}
