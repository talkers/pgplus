/*
 * Playground+ - admin.c
 * Admin commands
 * ---------------------------------------------------------------------------
 */

#include <string.h>
#include <memory.h>
#include <time.h>
#ifndef BSDISH
#include <malloc.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>

#include "include/config.h"
#include "include/player.h"
#include "include/admin.h"
#include "include/proto.h"

#ifdef AUTOSHUTDOWN
extern int auto_sd;
#endif

/* is this person a hard coded admin? */
int ishcadmin(char *name)
{
  char buf[512], *ptr, *sta;

  strncpy(buf, get_config_msg("hcadmins"), 511);
  for (ptr = sta = buf; *ptr; ptr++)
  {
    if (*ptr == ' ')
    {
      *ptr++ = 0;
      if (!strcasecmp(name, sta))
	return 1;
      sta = ptr;
    }
  }
  if (!strcasecmp(name, sta))	/* check the last one here */
    return 1;

  return 0;
}

/* do they need to be on duty to do something? */
int check_duty(player * p)
{
  if (p->flags & BLOCK_SU && !(p->residency & ADMIN))
  {
    tell_player(p, get_admin_msg("go_on_duty"));
    return 0;
  }
  return 1;
}

void     kill_all_of_player(player * p)
{
   player          *scan = flatlist_start, *kept;

   while (scan)
   {
      kept = scan->flat_next;
      if (!strcasecmp (p->lower_name, scan->lower_name) && p != scan)
         destroy_player(scan);
      scan = kept;
   }
}



/* reload everything */

void reload(player * p, char *str)
{
  tell_player(p, " Loading help\n");
  init_help();
  tell_player(p, " Loading messages\n");
  init_messages();
  tell_player(p, " Done\n");
}


/* edit the banish file from the program */

void quit_banish_edit(player * p)
{
  tell_player(p, " Leaving without changes.\n");
}

void end_banish_edit(player * p)
{
  if (banish_file.where)
    FREE(banish_file.where);
  banish_file.length = p->edit_info->size;
  banish_file.where = (char *) MALLOC(banish_file.length);
  memcpy(banish_file.where, p->edit_info->buffer, banish_file.length);

  if (save_file(&banish_file, "files/banish"))
    tell_player(p, " Banish file changed and saved.\n");

}

void banish_edit(player * p, char *str)
{
  start_edit(p, 10000, end_banish_edit, quit_banish_edit, banish_file.where);
}

/* the eject command , muhahahahaa */

void sneeze(player * p, char *str)
{
  time_t t;
  int nologin = 0;
  char *oldstack, *num;
  player *e;

  oldstack = stack;
  CHECK_DUTY(p);

  if (!*str)
  {
    tell_player(p, " Format: sneeze <person> <time>\n");
    return;
  }

  t = time(0);
  if ((num = strrchr(str, ' ')))
    nologin = atoi(num) * 60;
  if (nologin > (60 * 10) && !(p->residency & ADMIN))
  {
    tell_player(p, " Thats too strict.. setting to 10 minutes.\n");
    nologin = (60 * 10);
  }
  if (!nologin)
    nologin = 300;
  else
    *num = 0;
  while (*str)
  {
    while (*str && *str != ',')
      *stack++ = *str++;
    if (*str)
      str++;
    *stack++ = 0;
    if (*oldstack)
    {
      e = find_player_global(oldstack);
      if (e)
      {
	if (!check_privs(p->residency, e->residency))
	{
	  tell_player(p, " No way pal !!!\n");
	  TELLPLAYER(e, " -=*> %s TRIED to sneeze you !!\n", p->name);
	  LOGF("sneeze", "%s failed to sneeze all over %s", p->name, e->name);
	}
	else
	{
	  tell_player(e, sneeze_msg.where);
	  e->sneezed = t + nologin;
	  e->eject_count++;
	  p->num_ejected++;
	  quit(e, 0);
	  TELLROOM(e->location, get_admin_msg("sneezed"), e->name);
	  SUWALL(" -=*> %s sneezes on %s for %d mins.\n"
		 " -=*> %s was from %s\n", p->name, e->name, nologin / 60,
		 e->name, e->inet_addr);
	  LOGF("sneeze", " %s sneezed on %s for %d [%s]",
	       p->name, e->name, nologin / 60, get_address(e, NULL));
	  sync_to_file(*(e->lower_name), 0);
	}
      }
    }
    stack = oldstack;
  }
}


/*
 * reset person (in case the su over does it (which wouldn't be like an su at
 * all.. nope no no))
 */

void reset_sneeze(player * p, char *str)
{
  char *oldstack = stack, *newtime;
  time_t t, nologin;
  player dummy;

  CHECK_DUTY(p);

  if (!*str)
  {
    tell_player(p, " Format: reset_sneeze <player> <new time>\n");
    return;
  }

  t = time(0);
  newtime = next_space(str);
  if (*newtime)
  {
    *newtime++ = 0;
    nologin = atoi(newtime) * 60;
    if (nologin > (60 * 10) && !(p->residency & ADMIN))
    {
      tell_player(p, " Thats too strict.. setting to 10 minutes.\n");
      nologin = 60 * 10;
    }
    nologin += t;
  }
  else
  {
    nologin = 0;
  }
  memset(&dummy, 0, sizeof(player));
  strcpy(dummy.lower_name, str);
  lower_case(dummy.lower_name);
  dummy.fd = p->fd;
  if (!load_player(&dummy))
  {
    tell_player(p, WHO_IS_THAT);
    return;
  }
  switch (dummy.residency)
  {
    case SYSTEM_ROOM:
      tell_player(p, " That's a system room.\n");
      return;
    default:
      if (dummy.residency & BANISHD)
      {
	if (dummy.residency == BANISHD)
	  tell_player(p, " That Name is banished.\n");
	else
	  tell_player(p, " That Player is banished.\n");
	return;
      }
      break;
  }
  dummy.sneezed = nologin;
  dummy.location = (room *) - 1;
  save_player(&dummy);

  /* tell the SUs, too */
  if (!nologin)
    sprintf(stack, " -=*> %s resets the sneeze time on %s...\n",
	    p->name, dummy.name);
  else
    sprintf(stack, " -=*> %s changes the the sneeze time on %s to %d more seconds.\n",
	    p->name, dummy.name, (int) (nologin - t));
  stack = end_string(stack);
  su_wall(oldstack);
  LOGF("sneeze", "%s reset the sneeze on %s to %d", p->name,
       dummy.name, (int) nologin - (int) t);
  stack = oldstack;
}


/* SPLAT!!!! Wibble plink, if I do say so myself */

void soft_splat(player * p, char *str)
{
  char *oldstack, *reason;
  player *dummy;
  int no1, no2, no3, no4;

  oldstack = stack;

  if (!(reason = strchr(str, ' ')))
  {
    tell_player(p, " Format: splat <person> <reason>\n");
    return;
  }
  CHECK_DUTY(p);

  if (!*str)
  {
    tell_player(p, " Format: splat <person> <reason>\n");
    return;
  }

  *reason++ = 0;
  dummy = find_player_global(str);
  if (!dummy)
    return;
  sprintf(stack, "%s SPLAT: %s", str, reason);
  stack = end_string(stack);
  soft_eject(p, oldstack);
  *reason = ' ';
  stack = oldstack;
  if (!(dummy->flags & CHUCKOUT))
    return;
  soft_timeout = time(0) + (5 * 60);
  sscanf(dummy->num_addr, "%d.%d.%d.%d", &no1, &no2, &no3, &no4);
  soft_splat1 = no1;
  soft_splat2 = no2;
  sprintf(stack, " -=*> Site %d.%d.*.* banned to newbies for 5 mins.\n", no1, no2);
  stack = end_string(stack);
  su_wall(oldstack);
  stack = oldstack;
}


void splat_player(player * p, char *str)
{
  time_t t;
  char *space;
  player *dummy;
  int no1, no2, no3, no4, tme = 0;

  tme = 0;

  if (!(p->residency & (SU | ADMIN)))
  {
    soft_splat(p, str);
    return;
  }

  if (!*str)
  {
    tell_player(p, " Format: splat <person> <time>\n");
    return;
  }

  CHECK_DUTY(p);
  if ((space = strchr(str, ' ')))
  {
    *space++ = 0;
    tme = atoi(space);
  }

  dummy = find_player_global(str);
  if (!dummy)
    return;

  if (((p->residency & SU && !(p->residency & ADMIN)) && (tme < 0 || tme > 10)) ||
      (p->residency & ADMIN && (tme < 0)))
  {
    tell_player(p, " Thats too strict.. setting to 10 minutes.\n");
    tme = 10;
  }
  else
  {
    /* when no time specified */
    if (!tme)
    {
      tell_player(p, "Time set to 5 minutes.\n");
      tme = 5;
    }
  }

  sneeze(p, dummy->lower_name);
  if (!(dummy->flags & CHUCKOUT))
    return;
  t = time(0);
  splat_timeout = t + (tme * 60);
  sscanf(dummy->num_addr, "%d.%d.%d.%d", &no1, &no2, &no3, &no4);
  splat1 = no1;
  splat2 = no2;
  SUWALL(" -=*> %d.%d.*.* banned for %d minutes cause of %s\n",
	 no1, no2, tme, dummy->name);
}

void unsplat(player * p, char *str)
{
  char *spc;
  time_t t;
  int number = -1;

  CHECK_DUTY(p);

  t = time(0);
  if (*str)
  {
    spc = strchr(str, ' ');
    if (spc)
      number = atoi(spc);
    else
      number = 0;
  }
  if (!*str || number < 0)
  {
    number = splat_timeout - (int) t;
    if (number <= 0)
    {
      tell_player(p, " No site banned atm.\n");
      return;
    }
    SUWALL(" -=*> %s bans %d.%d.*.* for %d more seconds.\n", p->name,
	   splat1, splat2, number);
    return;
  }
  if (splat1 == 0 && splat2 == 0)
  {
    tell_player(p, " No site banned atm.\n");
    return;
  }
  if (number == 0)
  {
    SUWALL(" -=*> %s has unbanned site %d.%d.*.*\n",
	   p->name, splat1, splat2);
    splat_timeout = (int) t;
    return;
  }
  if (number > 600)
  {
    tell_player(p, " Thats too strict.. setting to 10 minutes.\n");
    number = 600;
  }
  SUWALL(" -=*> %s changes the ban on site %d.%d.*.* to a further %d seconds.\n",
	 p->name, splat1, splat2, number);
  splat_timeout = (int) t + number;
}


/* the eject command (too) , muhahahahaa */

void soft_eject(player * p, char *str)
{
  char *oldstack, *text, *reason;
  player *e;

  oldstack = stack;
  reason = next_space(str);
  if (*reason)
    *reason++ = 0;

  if (!*str)
  {
    tell_player(p, " Format: drag <person> <reason>\n");
    return;
  }

  CHECK_DUTY(p);

  e = find_player_global(str);
  if (e)
  {
    text = stack;
    if (!check_privs(p->residency, e->residency))
    {
      tell_player(p, " Sorry, you can't...\n");
      TELLPLAYER(e, " -=*> %s TRIED to drag you away!!\n", p->name);
      LOGF("drag", " -=*> %s TRIED to drag %s away", p->name, e->name);
      stack = text;
    }
    else
    {
      TELLPLAYER(e, " %s\n", get_admin_msg("dragged"));
      stack = text;
      e->eject_count++;
      p->num_ejected++;
      quit(e, 0);
      sprintf(stack, get_admin_msg("drag_msg"), e->name);
      stack = end_string(stack);
      TELLROOM(e->location, " %s\n", text);
      stack = text;
      SUWALL(" -=*> %s gets rid of %s because %s.\n"
	     " -=*> %s was from %s\n",
	     p->name, e->name, reason, e->name, e->inet_addr);
      LOGF("drag", " %s dragged %s because: %s", p->name, e->name, reason);
    }
  }
  stack = oldstack;
}

/* Privs Checking - compare 2 privs levels */

int check_privs(int p1, int p2)
{
  p1 &= ~NONSU;
  p2 &= ~NONSU;

#ifdef ROBOTS
  if (p2 & ROBOT_PRIV && !(p1 & ADMIN))		/* Only admin and above can mess with */
    return 0;			/* the robots */
#endif

  if (p1 & HCADMIN)
    return 1;			/* else if (p2 & PROTECT) return 0; */
  else if (p1 > p2)
    return 1;
  else
    return 0;
}


/* Blankpass, done _really_ _right_ ;) */

void new_blankpass(player * p, char *str)
{
  char *pass = "";
  player *p2, dummy;
  saved_player *sp;

  CHECK_DUTY(p);

  if (!*str)
  {
    tell_player(p, " Format: blankpass <person> [new password]\n");
    return;
  }
  if ((pass = strchr(str, ' ')))
  {
    *pass++ = 0;
    if (strlen(pass) > (MAX_PASSWORD - 2) || strlen(pass) < 3)
    {
      tell_player(p, " Try a reasonable length password.\n");
      return;
    }
  }
  lower_case(str);
  p2 = find_player_absolute_quiet(str);

  /* Hell, if their not on the program you SHOULD know */

  if (!p2)
    TELLPLAYER(p, " '%s' isnt on the talker.\n", str);
  else
  {
    if (!check_privs(p->residency, p2->residency))
    {
      tell_player(p, " You can't blankpass THAT person!\n");
      SW_BUT(p, " -=*> %s TRIED to blankpass %s!\n", p->name, p2->name);
      LOGF("blanks", "%s failed to blankpass %s (Nuke the ass for trying "
	   "*grin*)", p->name, p2->name);
      return;
    }
    if (!pass)
    {
      p2->password[0] = 0;
      tell_player(p, "Password blanked.\n");
      TELLPLAYER(p2, " -=*> %s has just blanked your password.\n", p->name);
      LOGF("blanks", "%s blanked %s's password (logged in)", p->name, p2->name);
      return;
    }
    strcpy(p2->password, do_crypt(pass, p2));
    tell_player(p, " Password changed. They have NOT been informed of"
		" what it is.\n");
    TELLPLAYER(p2, " -=*> %s has just changed your password.\n", p->name);
    LOGF("blanks", "%s changed %s's password (logged in)", p->name, p2->name);
    SW_BUT(p, " -=*> %s changes %s's password ... and %s doesn't know whut it is.\n",
	   p->name, p2->name, p2->name);
    set_update(*str);
    return;
  }

  sp = find_saved_player(str);
  if (!sp)
  {
    TELLPLAYER(p, " Couldn't find saved player '%s'.\n", str);
    return;
  }
  if (!check_privs(p->residency, sp->residency))
  {
    tell_player(p, " You can't do that !!!\n");
    LOGF("blanks", "%s TRIED to blankpass %s (logged out)", p->name, sp->lower_name);
    SW_BUT(p, " -=*> %s TRIED to blankpass %s !!!\n", p->name, sp->lower_name);
    quit(p, "");
    return;
  }
  strcpy(dummy.lower_name, str);
  dummy.fd = p->fd;
  if (!(load_player(&dummy)))
  {
    tell_player(p, " Not found in files ...\n");
    return;
  }
  if (dummy.residency == BANISHD || dummy.residency & SYSTEM_ROOM)
  {
    tell_player(p, " Thats a banished NAME or system room.\n");
    return;
  }
  if (dummy.residency & BANISHD)
    tell_player(p, " By the way, this player is currently BANISHD.");
  if (!pass)
  {
    tell_player(p,
		" You can't Blank a password when someone is logged out.\n"
    " Simply cause if they dont have a password, then they are not saved,\n"
    " if they aren't saved, then the blanked password doesn't have effect.\n"
	    " So youll have to actually set some new password for them.\n");
    return;
  }
  strcpy(dummy.password, do_crypt(pass, &dummy));
  tell_player(p, " Password changed in saved files.\n");
  LOGF("blanks", "%s changed %s's password (logged out)", p->name, dummy.name);
  SW_BUT(p, " -=*> %s changed %s's password (logged out)\n", p->name, dummy.name);
  dummy.script = 0;
  dummy.script_file[0] = 0;
  dummy.flags &= ~SCRIPTING;
  dummy.location = (room *) - 1;
  save_player(&dummy);
}

#ifdef NULLcode
/* Blankpass, done _right_ */

void new_blankpass(player * p, char *str)
{
  char *oldstack;
  char *pass;
  player *p2, dummy;
  saved_player *sp;
  char bpassd[MAX_NAME] = "";

  CHECK_DUTY(p);

  if (!*str)
  {
    tell_player(p, " Format: blankpass <person> [new password]\n");
    return;
  }
  oldstack = stack;
  pass = 0;
  pass = strchr(str, ' ');
  if (pass)
  {
    *pass++ = 0;
    if (strlen(pass) > (MAX_PASSWORD - 2) || strlen(pass) < 3)
    {
      tell_player(p, " Try a reasonable length password.\n");
      return;
    }
  }
  lower_case(str);
  p2 = find_player_absolute_quiet(str);

  /* Hell, if their not on the program you SHOULD know */

  if (!p2)
    tell_player(p, NOT_HERE_ATM);

  if (p2)
  {
    if (!check_privs(p->residency, p2->residency))
    {
      tell_player(p, " You can't blankpass THAT person!\n");
      SW_BUT(p, " -=*> %s TRIED to blankpass %s!\n", p->name, p2->name);
      LOGF("blanks", "%s failed to blankpass %s (Nuke the ass for trying "
	   "*grin*)", p->name, p2->name);
      return;
    }
    if (!pass)
    {
      TELLPLAYER(p2, " -=*> %s has just blanked your password.\n",
		 p->name);
      p2->password[0] = 0;
      tell_player(p, "Password blanked.\n");
      LOGF("blanks", "%s blanked %s's password (logged in)", p->name,
	   p2->name);
    }
    else
    {
      TELLPLAYER(p2, " -=*> %s has just changed your password.\n",
		 p->name);
      strcpy(p2->password, do_crypt(pass, p2));
      tell_player(p, " Password changed. They have NOT been informed of"
		  " what it is.\n");
      LOGF("blanks", "%s changed %s's password (logged in)", p->name,
	   p2->name);
    }
    set_update(*str);
    return;
  }
  else
    strcpy(bpassd, str);

  lower_case(bpassd);

  /* This is the setup for the saved priv check */

  sp = find_saved_player(bpassd);
  if (!sp)
  {
    TELLPLAYER(p, " Couldn't find saved player '%s'.\n", str);
    return;
  }
  /* This is what needed to be added (thanks to Mantis of Resort) */

  if (!check_privs(p->residency, sp->residency))
  {
    tell_player(p, " You can't blankpass that save file !\n");
    return;
  }
  strcpy(dummy.lower_name, str);
  dummy.fd = p->fd;
  if (load_player(&dummy))
  {
    if (dummy.residency == BANISHD)
    {
      tell_player(p, " Thats a banished NAME.\n");
      return;
    }
    if (dummy.residency & BANISHD)
      tell_player(p, " By the way, this player is currently BANISHD.");
    if (pass)
    {
      strcpy(dummy.password, do_crypt(pass, &dummy));
      tell_player(p, " Password changed in saved files.\n");
      LOGF("blanks", "%s changed %s's password (logged out)",
	   p->name, dummy.name);
      SW_BUT(p, " -=*> %s changed %s's password (logged out)\n",
	     p->name, dummy.name);
    }
    else
    {
      tell_player(p,
		  " You can't Blank a password when someone is logged out.\n"
		  " Simply cause if they dont have a password, then they are not saved,\n"
		  " if they aren't saved, then the banked password doesn't have effect.\n");
      return;
    }
    dummy.script = 0;
    dummy.script_file[0] = 0;
    dummy.flags &= ~SCRIPTING;
    dummy.location = (room *) - 1;
    save_player(&dummy);
  }
  else
    tell_player(p, " Can't find that player in saved files.\n");
}
#endif /* NULLcode */

void rm_shout_saved(player * p, char *str, int for_time)
{
  player *p2, dummy;

  if (for_time != 0 && for_time != -1)
  {
    tell_player(p, " Can only rm_shout a saved player forever, or clear one.\n");
    return;
  }
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


  if (!(p2->system_flags & SAVENOSHOUT) && for_time == 0)
  {
    tell_player(p, " That player is not rm_shoutted.\n");
    return;
  }
  else if (p2->system_flags & SAVENOSHOUT && for_time == -1)
  {
    tell_player(p, " That player is already rm_shoutted forever.\n");
    return;
  }
  p2->system_flags ^= SAVENOSHOUT;

  if (for_time == 0)
  {
    SUWALL(" -=*> %s regrants shouts to %s.\n", p->name, p2->name);
    LOGF("rm_shout", "%s regranted shouts to %s", p->name, p2->name);
  }
  else
  {
    SUWALL(" -=*> %s removes shouts from %s -- forever!!\n", p->name, p2->name);
    LOGF("rm_shout", "%s removed shouts from %s for -1", p->name, p2->name);
    LOGF("forever", "%s removed shouts from %s", p->name, p2->name);
  }
  save_player(&dummy);
}


void rm_sing_saved(player * p, char *str, int for_time)
{
  player *p2, dummy;

  if (for_time != 0 && for_time != -1)
  {
    tell_player(p, " Can only rm_sing a saved player forever, or clear one.\n");
    return;
  }
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


  if (!(p2->system_flags & SAVE_NO_SING) && for_time == 0)
  {
    tell_player(p, " That player is not rm_singed.\n");
    return;
  }
  else if (p2->system_flags & SAVE_NO_SING && for_time == -1)
  {
    tell_player(p, " That player is already rm_singed forever.\n");
    return;
  }
  p2->system_flags ^= SAVE_NO_SING;

  if (for_time == 0)
  {
    SUWALL(" -=*> %s regrants singing to %s.\n", p->name, p2->name);
    LOGF("rm_shout", "%s regranted singing to %s", p->name, p2->name);
  }
  else
  {
    SUWALL(" -=*> %s removes singing from %s -- forever!!\n", p->name, p2->name);
    LOGF("rm_sing", "%s removed singing from %s for -1", p->name, p2->name);
    LOGF("forever", "%s removed singing from %s", p->name, p2->name);
  }
  save_player(&dummy);
}


/* remove shout from someone for a period */

void remove_shout(player * p, char *str)
{
  char *size = 0;
  int new_size = 5;
  player *p2;

  if (!*str)
  {
    tell_player(p, " Format: rm_shout <player> <how long> (-1 for eternal, 0 for restore)\n");
    return;
  }

  CHECK_DUTY(p);

  size = strchr(str, ' ');
  if (size)
  {
    *size++ = 0;
    new_size = atoi(size);
  }
  p2 = find_player_global(str);
  if (!p2)
  {
    if (size)
      rm_shout_saved(p, str, new_size);
    return;
  }
  if (!check_privs(p->residency, p2->residency))
  {
    tell_player(p, " You can't do that !!\n");
    TELLPLAYER(p2, " -=*> %s tried to remove shout from you.\n", p->name);
    return;
  }
  p2->system_flags &= ~SAVENOSHOUT;
  if (new_size)
  {
    TELLPLAYER(p2, " %s\n", get_admin_msg("rm_shout"));
    p->num_rmd++;
  }
  else
    TELLPLAYER(p2, " %s\n", get_admin_msg("un_rm_shout"));
  if (new_size > 30)
    if (!(p->residency & ADMIN))
      new_size = 5;
  switch (new_size)
  {
    case -1:
      SUWALL(" -=*> %s removes shouts from %s forever!\n",
	     p->name, p2->name);
      p2->system_flags |= SAVENOSHOUT;
      p2->no_shout = -1;
      break;
    case 0:
      SUWALL(" -=*> %s restores shouts to %s.\n", p->name, p2->name);
      break;
    case 1:
      SUWALL(" -=*> %s just remove shouted %s for 1 minute.\n",
	     p->name, p2->name);
      break;
    default:
      SUWALL(" -=*> %s just remove shouted %s for %d minutes.\n",
	     p->name, p2->name, new_size);
      break;
  }
  new_size *= 60;
  if (new_size >= 0)
    p2->no_shout = new_size;

  if (new_size != 0)
    LOGF("rm_shout", "%s removed %s's shout for %d.", p->name, p2->name,
	 new_size);
  else
    LOGF("rm_shout", "%s regranted shouts to %s.", p->name, p2->name);
  if (new_size < 0)
    /* log in the "forever" log */
    LOGF("forever", "%s removes %s shout forever.", p->name, p2->name);

}

/* cut-and-paste of rm_shout -- someone shoot me, I'm a spooner --traP */
void remove_sing(player * p, char *str)
{
  char *size = 0;
  int new_size = 5;
  player *p2;

  if (!*str)
  {
    tell_player(p, " Format: rm_sing <player> [how long]\n");
    return;
  }

  CHECK_DUTY(p);

  size = strchr(str, ' ');
  if (size)
  {
    *size++ = 0;
    new_size = atoi(size);
  }
  p2 = find_player_global(str);
  if (!p2)
  {
    if (size)
      rm_sing_saved(p, str, new_size);
    return;
  }
  if (!check_privs(p->residency, p2->residency))
  {
    tell_player(p, " You can't do that !!\n");
    TELLPLAYER(p2, " -=*> %s tried to remove sing from you.\n", p->name);
    return;
  }
  p2->system_flags &= ~SAVE_NO_SING;
  if (new_size)
  {
    TELLPLAYER(p2, " %s\n", get_admin_msg("rm_sing"));
    p->num_rmd++;
  }
  else
    TELLPLAYER(p2, " %s\n", get_admin_msg("un_rm_sing"));
  if (new_size > 30)
    if (!(p->residency & ADMIN))
      new_size = 5;
  switch (new_size)
  {
    case -1:
      SUWALL(" -=*> %s just remove singed %s. (permanently!)\n",
	     p->name, p2->name);
      p2->system_flags |= SAVE_NO_SING;
      p2->no_sing = -1;
      break;
    case 0:
      SUWALL(" -=*> %s just allowed %s to sing again.\n", p->name,
	     p2->name);
      break;
    case 1:
      SUWALL(" -=*> %s just remove singed %s for 1 minute.\n",
	     p->name, p2->name);;
      break;
    default:
      SUWALL(" -=*> %s just remove singed %s for %d minutes.\n",
	     p->name, p2->name, new_size);
      break;
  }
  new_size *= 60;
  if (new_size >= 0)
    p2->no_sing = new_size;

  if (new_size < 0)
    LOGF("rm_sing", "%s removed %s's sing for %d.", p->name, p2->name,
	 new_size);
  else if (new_size == 0)
    LOGF("rm_sing", "%s regranted sings to %s.", p->name, p2->name);
  else
  {
    LOGF("rm_sing", "%s gave %s a permenant remove sing...", p->name, p2->name);
    LOGF("forever", "%s gave %s a permenant remove sing...", p->name, p2->name);
  }
}


/* remove trans movement from someone for a period */

void remove_move(player * p, char *str)
{
  char *size;
  int new_size = 5;
  player *p2;

  if (!*str)
  {
    tell_player(p, " Format: rm_move <player> [<for how long>]\n");
    return;
  }

  CHECK_DUTY(p);

  size = strchr(str, ' ');
  if (size)
  {
    *size++ = 0;
    new_size = atoi(size);
  }
  else
    new_size = 1;
  p2 = find_player_global(str);
  if (!p2)
    return;

  if (p2 == p && new_size != 0)
  {
    tell_player(p, " You're a bit stupid really aren't you?\n");
    return;
  }

  if (!check_privs(p->residency, p2->residency))
  {
    tell_player(p, " You can't do that !!\n");
    TELLPLAYER(p2, " -=*> %s tried to remove move from you.\n", p->name);
    return;
  }
  p2->system_flags &= ~SAVED_RM_MOVE;
  if (new_size)
  {
    tell_player(p2, " -=*> You step on some chewing-gum, and you suddenly "
		"find it very hard to move ...\n");
    p->num_rmd++;
  }
  else
    tell_player(p2, " -=*> Someone hands you a new pair of hi-tops ...\n");
  if (new_size > 30)
    new_size = 5;
  new_size *= 60;
  if (new_size >= 0)
    p2->no_move = new_size;
  else
  {
    p2->system_flags |= SAVED_RM_MOVE;
    p2->no_move = 100;		/* temp dummy value */
  }
  if ((new_size / 60) == 1)
    SUWALL(" -=*> %s remove moves %s for 1 minute.\n", p->name,
	   p2->name);
  else if (new_size == 0)
  {
    SUWALL(" -=*> %s allows %s to move again.\n", p->name, p2->name);
    LOGF("rm_move", "%s lets %s move again", p->name, p2->name);
  }
  else if (new_size < 0)
    SUWALL(" -=*> %s remove moves %s. Permanently!\n", p->name,
	   p2->name);
  else
    SUWALL(" -=*> %s remove moves %s for %d minutes.\n", p->name,
	   p2->name, new_size / 60);

  if (new_size != 0)
    LOGF("rm_move", "%s rm_moves %s for %d minutes", p->name,
	 p2->name, new_size / 60);
}

/* This is new residency code for Playground+.
   It looks horrible and you'd be right - because it is! --Silver */

void resident(player * p, char *str)
{
  char *first;
  player *p2;
#ifdef NEW_RES_CODE
  player *scan;
#endif
  int valid = 0, susonduty = 0;

  if (!*str)
  {
    tell_player(p, " Format: res <whoever>\n");
    return;
  }

  first = first_char(p);
  if (strstr(first, "validate"))
    valid = 1;

  if (*str == '\007')		/* self res */
    p2 = p;
  else
  {
    p2 = find_player_global(str);
    if (!p2)
      return;
  }
  if (!p2->location)
  {
    tell_player(p, "Easy tiger! Wait till they're actually properly connected.\n");
    return;
  }

  if (p2->residency)
  {
    tell_player(p, " That person is already a resident\n");
    return;
  }
  if (p2->awaiting_residency != 0)
  {
    tell_player(p, " That person is already awaiting residency.\n");
    return;
  }

#ifndef NEW_RES_CODE
  susonduty = 0;		/* Which means the following 'if' will always be done */
#else
  for (scan = flatlist_start; scan; scan = scan->flat_next)
    if ((scan->residency & SU) && !(scan->flags & (BLOCK_SU | OFF_LSU)))
      susonduty++;
#endif

  if (p == p2)
  {
    strcpy(p->ressied_by, "Self-res");
    strcpy(p->proposed_by, p->name);
  }
  else
  {
    strcpy(p2->ressied_by, p->name);
    strcpy(p2->proposed_by, p->name);
  }

  if (susonduty <= 1)
    make_resident(p2);		/* Make resident immediately */
  else
  {
    TELLPLAYER(p2, "\n -=*> %s has proposed you for residency.\n"
	       " -=*> Provided there are no objections you will be a resident in 1 minute.\n", p->name);
    TELLPLAYER(p, " You propose %s for residency\n", p2->name);
    SW_BUT(p, "\n");
    if (valid)
    {
      SW_BUT(p, " -=*> %s has proposed %s for VALIDATED residency.\n", p->name, p2->name);
      p2->validated_email = 1;
      LOGF("resident", "%s proposes %s for validated residency", p->name, p2->name);
    }
    else
    {
      SW_BUT(p, " -=*> %s has proposed %s for residency.\n", p->name, p2->name);
      LOGF("resident", "%s proposes %s for residency", p->name, p2->name);
    }
    SW_BUT(p, " -=*> If you have any objections use the \"obj\" command now.\n");
    SW_BUT(p, " -=*> Otherwise, %s will become a resident in 1 minute.\007\n\n", p2->name);

    p2->awaiting_residency = 61;	/* Start the timer */
  }
}

/* Make a resident time! */

void make_resident(player * p)
{
  player *p2 = (player *) NULL, dummy;

  p->awaiting_residency = 0;

  /* If they're in the pager, take them out of it */

  if (p->edit_info)
    quit_pager(p, p->edit_info);

  SUWALL(" -=*> %s is granted residency\n", p->name);

  if (strcasecmp(p->name, p->proposed_by))
  {
    p2 = find_player_global_quiet(p->proposed_by);
    if (!p2)
    {				/* Load in their stats cos they've quit by now */
      strcpy(dummy.lower_name, p->proposed_by);
      lower_case(dummy.lower_name);
      dummy.fd = p->fd;
      if (!load_player(&dummy))
	return;
      dummy.num_ressied++;
      dummy.script = 0;
      dummy.script_file[0] = 0;
      dummy.flags &= ~SCRIPTING;
      dummy.location = (room *) - 1;
      save_player(&dummy);
    }
    else
    {
      p2->num_ressied++;
      if (p2->num_ressied == 1)
	TELLPLAYER(p2, " -=*> You have now granted residency to 1 person.\n");
      else
	TELLPLAYER(p2, " -=*> You have now granted residency to %d people.\n", p2->num_ressied);
    }
  }

  p->residency |= get_flag(permission_list, "residency");
  p->residency |= NO_SYNC;
  if (p->validated_email)
  {
    p->email[0] = ' ';
    p->email[1] = 0;
  }
  else
  {
    p->email[0] = 2;
    p->email[1] = 0;
  }
  p->flags &= ~SCRIPTING;
  p->script = 0;
  p->script_file[0] = 0;
  strcpy(p->script_file, "dummy");
  tell_player(p, get_admin_msg("res_header"));
  p->saved_residency = p->residency;
  p->saved = 0;
  if (!p2)
  {
    LOGF("resident", "%s gets %sself residency.", p->name, get_gender_string(p));
    begin_ressie(p, 0);
  }
  else if (p->validated_email)
  {
    LOGF("resident", "%s becomes a resident (validated)", p->name);
    newsetpw0(p, 0);
  }
  else
  {
    LOGF("resident", "%s becomes a resident", p->name);
    begin_ressie(p, 0);
  }
}

/* The object conmmand */

void object_to_ressie(player * p, char *str)
{
  player *p2;
  char *reason;

  if (!(reason = strchr(str, ' ')))
  {
    tell_player(p, " Format: obj <person> <reason>\n"
	      "  Note: Reason is logged and broadcast to *all* parties!\n");
    return;
  }
  *reason++ = 0;

  p2 = find_player_global(str);
  if (!p2)
    return;

  if (p2->awaiting_residency == 0)
  {
    tell_player(p, " That person is not awaiting residency.\n");
    return;
  }
  TELLPLAYER(p, " You formally object to the residency of %s.\n", p2->name);
  p2->awaiting_residency = 0;

  SW_BUT(p, "\n");
  SW_BUT(p, " -=*> %s objects to the residency of %s.\n", p->name, p2->name);
  SW_BUT(p, " -=*> Reason: %s\n", reason);
  SW_BUT(p, " -=*> %s's residency has been cancelled.\n\n\007", p2->name);

  TELLPLAYER(p2, " -=*> There has been an objection to your residency!\n");
  TELLPLAYER(p2, " -=*> Reason: %s\n", reason);
  TELLPLAYER(p2, " -=*> You will not become a resident now. Sorry.\n");

  LOGF("residency", "%s objects to %s's residency application: %s", p->name, p2->name, reason);
}


void res_me(player * p, char *str)
{
  if (!(config_flags & cfSELFRES))
  {
    tell_player(p, " Sorry, this command is not implemented\n");
    return;
  }
  if (p->residency & BASE)
  {
    tell_player(p, " Hehehe, good one, but you are already a resident.\n");
    return;
  }

  if (true_count_su() != 0)
  {
    TELLPLAYER(p, " There's at least one %s on atm, talk to them.\n",
	       get_config_msg("staff_name"));
    SUWALL(" -=*> %s needs some help with residency ... (tried to 'resme')\n",
	   p->name);
    return;
  }
  resident(p, "\007");
}

void override(player * p, char *str)
{
  player *p2;
  char *nt;
  int newtime;


  if (!(nt = strchr(str, ' ')))
  {
    tell_player(p, " Format: override <player> <seconds to res>\n");
    return;
  }
  *nt++ = 0;
  newtime = atoi(nt);

  if (newtime > 600)
  {
    tell_player(p, " Anything over 10 mins is just wasting time ...\n");
    return;
  }
  if (newtime < 1)
    newtime = 1;

  if (!(p2 = find_player_global(str)))
    return;

  if (!p2->awaiting_residency)
  {
    TELLPLAYER(p, " Honey, %s aint anywhere near the res process.\n",
	       p2->name);
    return;
  }

  SW_BUT(p, " -=*> %s overrides %s to %s ...\n", p->name, p2->name,
	 word_time(newtime));
  TELLPLAYER(p, " You override %s res time to %s\n", p2->name,
	     word_time(newtime));
  LOGF("override", "%s overrides %s time to %d secs, was %d secs.",
       p->name, p2->name, newtime, p2->awaiting_residency);

  if (newtime > p2->awaiting_residency)
    TELLPLAYER(p2,
    " -=*> Due to unforseen circumstances, your request for residency will\n"
	   " -=*> be delayed for another %s.  Sorry for any inconvience.\n",
	       word_time(newtime));

  p2->awaiting_residency = newtime;
}


/* show premissions alterable */
void show_possible_privs(player * p, char *whut)
{
  char mid[160], *oldstack = stack;
  int i, j, hits = 0;

  memset(mid, 0, 160);
  sprintf(mid, "%sable privs", whut);
  mid[0] = toupper(mid[0]);

  pstack_mid(mid);
  stack += sprintf(stack, "\n");

  for (i = 0; permission_list[i].text[0]; i++)
  {
    /* oks, before it was assumed that the perm_required[]
       array elements will match up spot for spot with the
       permissions_list[] elements...which they dont...
       soooo ... lets do it like this... ~phy
     */
    for (j = 0; perm_required[j].text[0]; j++)
      if (!strcasecmp(permission_list[i].text,
		      perm_required[j].text))
	break;

    if (!perm_required[j].change)
    {
      LOGF("error", "in admin.h, the priv '%s' needs an entry in "
	   "perm_required[]", permission_list[i].text);
      continue;
    }

    /* only add it if they've got the priv from perm_required
       and theyve got the actual priv OR they are hcadmin
     */
    if ((p->residency & perm_required[j].change &&
	 p->residency & permission_list[i].change) ||
	p->residency & HCADMIN)
    {
      stack += sprintf(stack, "%s, ", permission_list[i].text);
      hits++;
    }
  }
  if (!hits)
  {
    stack = oldstack;
    TELLPLAYER(p, " You may not %s any privs at all ...\n", whut);
    return;
  }

  while (*stack != ',')
    stack--;
  *stack++ = '.';
  *stack++ = '\n';
  *stack++ = '\n';
  *stack = '\0';
  sprintf(mid, "Possible %d privs you may %s.", hits, whut);
  pstack_mid(mid);
  stack += sprintf(stack, " Format : %s <whoever> <whatever>\n", whut);
  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;

}

/* the grant command */

void grant(player * p, char *str)
{
  char *permission;
  player *p2;
  saved_player *sp;
  int change, required = 0;
  char *oldstack;

  CHECK_DUTY(p);

  oldstack = stack;
  permission = next_space(str);

  if (!*permission)
  {
    show_possible_privs(p, "grant");
    return;
  }

  *permission++ = 0;

  if (!strcasecmp(permission, "residency"))
  {
    tell_player(p, " You can't grant residency this way. Try the 'res' command.\n");
    return;
  }

  change = get_flag(permission_list, permission);
  required = get_flag(perm_required, permission);
  if (!change || !required)
  {
    tell_player(p, " Can't find that permission.\n");
    return;
  }
  if (!(p->residency & required))
  {
    tell_player(p, " You can't grant that permission.\n");
    return;
  }
  if (!(p->residency & change))
  {
    if (!(p->residency & HCADMIN))
    {
      tell_player(p, " You can't give out permissions you haven't got "
		  "yourself.\n");
      return;
    }
  }
  p2 = find_player_global(str);
  if (!p2)
  {
    lower_case(str);
    sp = find_saved_player(str);
    if (!sp)
    {
      tell_player(p, " Couldn't find player.\n");
      stack = oldstack;
      return;
    }
    if (sp->residency == BANISHD || sp->residency == SYSTEM_ROOM)
    {
      tell_player(p, " That is a banished NAME, or System Room.\n");
      stack = oldstack;
      return;
    }
    if ((change == SYSTEM_ROOM) && !(sp->residency == 0))
    {
      tell_player(p, " You can't grant sysroom to anything but a blank"
		  "playerfile.\n");
      stack = oldstack;
      return;
    }
    if (!check_privs(p->residency, sp->residency))
    {
      tell_player(p, " You can't alter that save file\n");
      LOGF("grant", "%s failed to grant %s to %s\n", p->name,
	   permission, str);

      AW_BUT(p, " -=*> %s FAILED to grant %s to %s\n",
	     p->name, permission, str);
      stack = oldstack;
      return;
    }
    tell_player(p, " Permission changed in player files.\n");
    LOGF("grant", "%s granted %s to %s (logged out)", p->name, permission,
	 sp->lower_name);
    sp->residency |= change;
    set_update(*str);
    AW_BUT(p, " -=*> %s granted %s to %s (logged out)\n",
	   p->name, permission, sp->lower_name);
    stack = oldstack;
    return;
  }
  else
  {
    if (p2->residency == NON_RESIDENT)
    {
      tell_player(p, " That player is non-resident!\n");
      stack = oldstack;
      return;
    }
    if (p2->residency == BANISHD || p2->residency == SYSTEM_ROOM)
    {
      tell_player(p, " That is a banished NAME, or System Room.\n");
      stack = oldstack;
      return;
    }
    if (!check_privs(p->residency, p2->residency))
    {
      tell_player(p, " No Way Pal !!\n");
      TELLPLAYER(p2, " -=*> %s tried to grant you '%s'.\n",
		 p->name, permission);
      stack = oldstack;
      return;
    }
    TELLPLAYER(p2, "\n -=*> %s has granted you `%s`.\007\n",
	       p->name, permission);
    p2->saved_residency |= change;
    p2->residency = p2->saved_residency;
    stack = oldstack;
    LOGF("grant", "%s granted %s to %s", p->name, permission, p2->name);
    SW_BUT(p2, " -=*> %s grants '%s' to %s\n", p->name, permission, p2->name);
    save_player(p2);
  }
  stack = oldstack;
}


/* the remove command */

void remove_privs(player * p, char *str)
{
  char *permission;
  player *p2;
  saved_player *sp;
  int change, required;
  char *oldstack;

  CHECK_DUTY(p);

  oldstack = stack;
  permission = next_space(str);

  if (!*permission)
  {
    show_possible_privs(p, "remove");
    return;
  }

  *permission++ = 0;

  if (!strcasecmp(permission, "residency"))
  {
    tell_player(p, " You can't remove residency this way. Try the 'nuke' command.\n");
    return;
  }

  change = get_flag(permission_list, permission);
  required = get_flag(perm_required, permission);
  if (!change || !required)
  {
    tell_player(p, " Can't find that permission.\n");
    return;
  }
  if (!(p->residency & change))
  {
    if (!(p->residency & HCADMIN))
    {
      tell_player(p, " You can't remove permissions you haven't got "
		  "yourself.\n");
      return;
    }
  }

  p2 = find_player_global(str);
  if (!p2)
  {
    sp = find_saved_player(str);
    if (!sp)
    {
      tell_player(p, " Couldn't find player.\n");
      return;
    }
    if (!check_privs(p->residency, sp->residency))
    {
      tell_player(p, " You cant change that save file !!!\n");
      LOGF("grant", "%s failed to remove %s from %s", p->name, permission, str);
      AW_BUT(p, " -=*> %s FAILED to remove %s from %s (logged out)\n",
	     p->name, permission, str);
      stack = oldstack;
      return;
    }
    sp->residency &= ~change;
    if (sp->residency == NON_RESIDENT)
      remove_player_file(sp->lower_name);
    set_update(*str);
    tell_player(p, " Permission changed in player files.\n");
    LOGF("grant", "%s removes %s from %s (logged out)", p->name,
	 permission, sp->lower_name);
    AW_BUT(p, " -=*> %s removed %s from %s (logged out)\n",
	   p->name, permission, sp->lower_name);
    stack = oldstack;
    return;
  }
  else
  {
    if (!check_privs(p->residency, p2->residency))
    {
      tell_player(p, " No Way Pal !!\n");
      TELLPLAYER(p2, " -=*> %s tried to remove '%s' from you. Sick'em!!\n",
		 p->name, permission);
      stack = oldstack;
      return;
    }
    p2->residency &= ~change;
    p2->saved_residency = p2->residency;
    TELLPLAYER(p2, " -=*> %s has altered your commands.\n", p->name);
    LOGF("grant", "%s removed %s from %s", p->name, permission, p2->name);
    AW_BUT(p2, " -=*> %s removes '%s' from %s\n", p->name, permission, p2->name);
    if (p2->residency != NON_RESIDENT)
      save_player(p2);
    else
      remove_player_file(p2->lower_name);
  }
  stack = oldstack;
}

/* remove player completely from the player files */

void nuke_player(player * p, char *str)
{
  player *p2, dummy;
  saved_player *sp;
  char *reason;
  char nuked[MAX_NAME] = "";
  char nukee[MAX_NAME] = "";
  char naddr[MAX_INET_ADDR] = "";
  int mesg_done = 0;
  int *scan, *scan_count, mcount = 0, sscan;
  note *smail, *snext;

  if (!(reason = strchr(str, ' ')))
  {
    tell_player(p, " Format: nuke <player> <reason>\n");
    return;
  }
  *reason++ = 0;

  if (strlen(reason) > 100)
  {
    tell_player(p, "Please make your reason more concise.\n");
    return;
  }
  CHECK_DUTY(p);

  p2 = find_player_absolute_quiet(str);
  if (!p2)
    TELLPLAYER(p, " No such person on %s - checking saved files ...\n",
	       get_config_msg("talker_name"));

  /* Original PG crashed if you tried to nuke a newbie - which was a very
     stupid and an easy bug to fix -- Silver */

  if (p2 && !(p2->residency))
  {
    tell_player(p, " That person isn't a resident.\n");
    return;
  }
  if (p2)
  {

    /* Full credit for this little beauty goes to DeathBoy who found that
       he (as HCAdmin) could nuke another HCAdmin ... me! --Silver */
    if (p2->residency & HCADMIN)
    {
      tell_player(p, " You can't nuke an HCAdmin DeathBoy!\n");
      SW_BUT(p, " -=*> %s just tried to nuke %s!?\n", p->name, p2->name);
      return;
    }
    if (!check_privs(p->residency, p2->residency))
    {
      tell_player(p, " You can't nuke them !\n");
      TELLPLAYER(p2, " -=*> %s tried to nuke you.  Yipes!!.\n", p->name);
      return;
    }
    if (p2->saved)
      all_players_out(p2->saved);

    /* time for a new nuke screen */
    TELLPLAYER(p2, nuke_msg.where);

    p->num_ejected++;
    p2->saved = 0;
    p2->residency = 0;
    strcpy(nuked, p2->name);
    strcpy(naddr, p2->inet_addr);
    kill_all_of_player(p2);
    quit(p2, "");
    SUWALL(" -=*> %s nukes %s to a crisp, TOASTY!!\n"
	   " -=*> %s was from %s\n"
	   " -=*> Reason: %s\n", p->name, nuked, nuked, naddr, reason);
    mesg_done = 1;
  }
  strcpy(nukee, str);
  lower_case(nukee);
  sp = find_saved_player(nukee);
  if (!sp)
  {
    TELLPLAYER(p, " Couldn't find saved player '%s'.\n", str);
    return;
  }
  if (!check_privs(p->residency, sp->residency))
  {
    tell_player(p, " You can't nuke that save file !\n");
    return;
  }
  if (sp->residency & HCADMIN)
  {
    tell_player(p, " You can't nuke somone who is HCAdmin! Doh!\n");
    SW_BUT(p, " -=*> %s just tried to nuke %s!?\n", p->name, sp->lower_name);
    return;
  }
  strcpy(dummy.lower_name, sp->lower_name);
  dummy.fd = p->fd;
  load_player(&dummy);
  dummy.residency = NON_RESIDENT;
  if (!*nuked)
  {
    strcpy(nuked, dummy.name);
    strcpy(naddr, sp->last_host);
  }
  kill_all_of_player(&dummy);

  /* Original code was commented out because it wibbled - now it doesn't
     (smug grin) -- Silver */

  scan = dummy.saved->mail_received;
  if (scan)
  {
    for (scan_count = scan; *scan_count; scan_count++)
    {
      mcount++;
    }
    for (; mcount; mcount--)
    {
      delete_received(&dummy, "1");
    }
  }
  mcount = 1;
  sscan = dummy.saved->mail_sent;
  smail = find_note(sscan);
  if (smail)
  {
    while (smail)
    {
      mcount++;
      sscan = smail->next_sent;
      snext = find_note(sscan);
      if (!snext && sscan)
      {
	smail->next_sent = 0;
	smail = 0;
      }
      else
      {
	smail = snext;
      }
    }
    for (; mcount; mcount--)
    {
      delete_sent(&dummy, "1");
    }
  }
  save_player(&dummy);

  all_players_out(sp);
  remove_player_file(nukee);
  if (!mesg_done)
  {
    SUWALL(" -=*> %s nukes \'%s\' to a crisp, TOASTY!!\n"
	   " -=*> Reason: %s\n", p->name, nuked, reason);
  }
  LOGF("nuke", "%s nuked %s [%s]. Reason: %s", p->name, nuked, naddr, reason);
}

/* Actually DO the suicide ... */

void suicide3(player * p, char *str)
{
  player *p2, dummy;
  saved_player *sp;
  char nuked[MAX_NAME] = "";
  char nukee[MAX_NAME] = "";
  char naddr[MAX_INET_ADDR] = "";
  int mesg_done = 0;

  p2 = find_player_absolute_quiet(str);
  if (p2)
  {
    if (p2->saved)
      all_players_out(p2->saved);
    p2->saved = 0;
    p2->residency = 0;
    strcpy(nuked, p2->name);
    strcpy(naddr, p2->inet_addr);
    SUWALL(" -=*> %s suicides, WHOOPSIE!!\n -=*> %s "
	   "was from %s\n", nuked, nuked, naddr);
    mesg_done = 1;
  }
  strcpy(nukee, str);
  lower_case(nukee);
  sp = find_saved_player(nukee);
  strcpy(dummy.lower_name, sp->lower_name);
  dummy.fd = p->fd;
  kill_all_of_player(p);
  quit(p, 0);
  load_player(&dummy);
  if (!*nuked)
  {
    strcpy(nuked, dummy.name);
    strcpy(naddr, sp->last_host);
  }
  all_players_out(sp);
  remove_player_file(nukee);
  LOGF("nuke", "%s suicided [%s]", nuked, naddr);
}

/* suicide try # 6254246352 -- asty (feeling useless) */

void confirm_suicide2(player * p, char *str)
{
  char *oldstack;

  oldstack = stack;
  if (check_password(p->password, str, p))
  {
    tell_player(p,
	  "\n\n -=>              You have suicided!                  \n\n");
    suicide3(p, (p->lower_name));
    return;
  }
  else
  {
    password_mode_off(p);
    p->flags |= PROMPT;
    p->input_to_fn = 0;
    tell_player(p, "\n\n Password does not match.\n"
		" You back away from the edge...\n");
    LOGF("suicide", " %s [%s] decides not to suicide", p->name, p->email);
  }
  stack = oldstack;
}

void suicide(player * p, char *str)
{
  if (!*str)
  {
    tell_player(p, " Format: suicide <reason>\n");
    return;
  }

  if (ishcadmin(p->name))
  {
    TELLPLAYER(p, " Um.. that'd be a security risk. No.\n");
    return;
  }
  LOGF("suicide", " %s [%s] is trying to suicide for the reason: %s", p->name, p->email, str);
  password_mode_on(p);
  p->flags &= ~PROMPT;
  do_prompt(p, "\007\n\n WARNING: You will lose ALL data including lists, rooms, etc.\n"
	    "To ABORT the process, type an incorrect password, and you will still live.\n"
	    " \n\n Enter your current password if you want to be deleted: ");
  p->input_to_fn = confirm_suicide2;
}


/* banish a player from the program */

void banish_player(player * p, char *str)
{
  char *i, ban_name[MAX_NAME + 1] = "";
  player *p2;
  saved_player *sp;
  int newbie = 0;

  CHECK_DUTY(p);


  if (!*str)
  {
    tell_player(p, " Format: banish <player>\n");
    return;
  }

  LOGF("banish", "%s is trying to banish %s ...", p->name, str);
  lower_case(str);

  if (strlen(str) > MAX_NAME - 2)
  {
    tell_player(p, " Too long a string to banish damn you!\n");
    log("banish", " ... but failed cause its tooo long");
    return;
  }

  p2 = find_player_absolute_quiet(str);
  if (!p2)
    tell_player(p, " No such person on the program.\n");
  if (p2)
  {
    if (p == p2)
    {
      tell_player(p, " Egads no!\n");
      SW_BUT(p, " -=*> %s needs to be took out n spanked ...\n", p->name);
      log("banish", " ... but failed cause you cant banish yerself");
      return;
    }
    if (!check_privs(p->residency, p2->residency))
    {
      tell_player(p, " You can't banish them !\n");
      TELLPLAYER(p2, " -=*> %s tried to banish you.\n", p->name);
      LOGF("banish", " ... but failed cause %s has more privs", p2->name);
      return;
    }
    if (p2->residency == 0)
      newbie = 1;
    tell_player(p2, "\n\n -=*> You have been banished !!!\n\n\n");
    p2->saved_residency |= BANISHD;
    p2->residency = p2->saved_residency;
    quit(p2, 0);
    strcpy(ban_name, p2->name);
  }
  if (!newbie)
  {
    sp = find_saved_player(str);
    if (sp)
    {
      if (sp->residency & BANISHD)
      {
	tell_player(p, " Already banished!\n");
	LOGF("banish", " ... but failed cause %s is already banished.\n",
	     sp->lower_name);
	return;
      }
      if (!check_privs(p->residency, sp->residency))
      {
	tell_player(p, " You can't banish that save file !\n");
	LOGF("banish", " ... but failed cause %s has more privs",
	     sp->lower_name);
	return;
      }
      sp->residency |= BANISHD;
      set_update(*str);
      tell_player(p, " Player successfully banished.\n");
      LOGF("banish", "%s sucessfully banishes player '%s'.",
	   p->name, sp->lower_name);
      SUWALL(" -=*> %s banished the player '%s'\n", p->name, sp->lower_name);
    }
    else
    {
      /* Create a new file with the BANISHD flag set */
      i = str;
      while (*i)
      {
	if (!isalpha(*i++))
	{
	  tell_player(p, " Banished names must only contain letters and must not be too large.\n");
	  log("banish", " ... but failed cause its got non alphas");
	  return;
	}
      }
      create_banish_file(str);
      tell_player(p, " Name successfully banished.\n");
      LOGF("banish", "%s sucessfully banishes name '%s'.", p->name, str);
      SUWALL(" -=*> %s banished the name '%s'\n", p->name, str);
    }
  }
}


/* Unbanish a player or name */

void unbanish(player * p, char *str)
{
  saved_player *sp;

  CHECK_DUTY(p);

  if (!*str)
  {
    tell_player(p, " Format: unbanish <name>\n");
    return;
  }

  lower_case(str);
  sp = find_saved_player(str);
  if (!sp)
  {
    tell_player(p, " Can't find saved player file for that name.\n");
    return;
  }
  if (!(sp->residency & BANISHD))
  {
    tell_player(p, " That player isn't banished!\n");
    return;
  }
  if (sp->residency == BANISHD || sp->residency == BANISHED)
  {
    remove_player_file(str);
    SUWALL(" -=*> %s unbanishes the name '%s'\n", p->name, str);
    return;
  }
  sp->residency &= ~BANISHD;
  set_update(*str);
  sync_to_file(str[0], 0);
  SUWALL(" -=*> %s unbanishes the player '%s'\n", p->name, str);
  LOGF("banish", "%s unbanished the player '%s'", p->name, str);
}


/* create a new character */
/* This command didn't work before, now it does --Silver */

void make_new_character(player * p, char *str)
{
  char *oldstack, *cpy, *email, *password = 0;
  player *np;
  int length = 0;

  CHECK_DUTY(p);

  oldstack = stack;
  email = next_space(str);

  if (!*str || !*email)
  {
    tell_player(p, " Format: make <character name> <email addr> "
		"<password>\n");
    return;
  }
/* chop the argument into "name\000email\000password\000" with ptrs as
   appropriate */
  *email++ = 0;

  password = end_string(email);
  while (*password != ' ')
    password--;
  *password++ = 0;

  for (cpy = str; *cpy; cpy++)
  {
    if (isalpha(*cpy))
    {
      *stack++ = *cpy;
      length++;
    }
  }
  *stack++ = 0;
  if (length > (MAX_NAME - 2))
  {
    tell_player(p, " Name too long.\n");
    stack = oldstack;
    return;
  }
  if (find_saved_player(oldstack))
  {
    tell_player(p, " That player already exists.\n");
    stack = oldstack;
    return;
  }

  np = create_player();
  np->flags &= ~SCRIPTING;
  strcpy(np->script_file, "dummy");
  np->fd = p->fd;
  np->location = (room *) - 1;

  restore_player(np, oldstack);
  np->flags &= ~SCRIPTING;
  strcpy(np->script_file, "dummy");
  strcpy(np->inet_addr, "NOT YET LOGGED ON");
  np->residency = get_flag(permission_list, "residency");
  np->saved_residency = np->residency;

  /* Crypt that password, why don't you */

  strcpy(np->password, do_crypt(password, np));

  strncpy(np->email, email, (MAX_EMAIL - 3));
  save_player(np);
  np->fd = -1;			/* NOT np->fd = 0; */
  np->location = 0;
  destroy_player(np);
  cpy = stack;
  sprintf(cpy, "%s creates %s.", p->name, oldstack);
  stack = end_string(cpy);
  log("make", cpy);
  tell_player(p, " Player created.\n");
  stack = oldstack;
  return;
}


/* port from EW dump file */

void port(player * p, char *str)
{
  char *oldstack, *scan;
  player *np;
  file old;

  oldstack = stack;
  old = load_file("files/old.players");
  scan = old.where;

  while (old.length > 0)
  {
    while (*scan != ' ')
    {
      *stack++ = *scan++;
      old.length--;
    }
    scan++;
    *stack++ = 0;
    strcpy(stack, oldstack);
    lower_case(stack);
    if (!find_saved_player(stack))
    {
      np = create_player();
      np->fd = p->fd;
      restore_player(np, oldstack);
      np->residency = get_flag(permission_list, "residency");
      stack = oldstack;
      while (*scan != ' ')
      {
	*stack++ = *scan++;
	old.length--;
      }
      *stack++ = 0;
      scan++;
      strncpy(np->password, oldstack, MAX_PASSWORD - 3);
      stack = oldstack;
      while (*scan != '\n')
      {
	*stack++ = *scan++;
	old.length--;
      }
      *stack++ = 0;
      scan++;
      strncpy(np->email, oldstack, MAX_EMAIL - 3);
      sprintf(oldstack, "%s [%s] %s\n", np->name, np->password, np->email);
      stack = end_string(oldstack);
      tell_player(p, oldstack);
      stack = oldstack;
      save_player(np);
      np->fd = 0;
      destroy_player(np);
    }
    else
    {
      while (*scan != '\n')
      {
	scan++;
	old.length--;
      }
      scan++;
    }
  }
  if (old.where)
    FREE(old.where);
  stack = oldstack;
}



/*
 * rename a person (yeah, right, like this is going to work .... )
 * (maybe one day I'll write a rename resident bit of code --Silver)
 */

void do_rename(player * p, char *str, int verbose)
{
  char *oldstack, *firspace, name[MAX_NAME + 2], *letter;
  char oldname[MAX_NAME + 2];
  int hash;
  player *oldp, *scan, *previous;
  saved_player *sp;

  CHECK_DUTY(p);

  oldstack = stack;

  if (!*str)
  {
    tell_player(p, " Format: rename <person> <new-name>\n");
    return;
  }

  if (!(firspace = strchr(str, ' ')))
    return;
  *firspace = 0;
  firspace++;
  letter = firspace;

  /* Test for a nice inputted name */
  if (strlen(letter) > MAX_NAME - 2 || strlen(letter) < 2)
  {
    tell_player(p, " Try picking a name of a decent length.\n");
    stack = oldstack;
    return;
  }

  oldp = find_player_global(str);
  if (!oldp || !oldp->location)
    return;

  if (oldp->residency & BASE)
  {
    sprintf(stack, " But you cannot rename %s. They are a resident.\n"
	    ,oldp->name);
    stack = end_string(stack);
    tell_player(p, oldstack);
    stack = oldstack;
    return;
  }
  strcpy(oldname, oldp->lower_name);
  scan = find_player_global_quiet(firspace);
  if (scan)
  {
    sprintf(stack, " There is already a person with the name '%s' "
	    "logged on.\n", scan->name);
    stack = end_string(stack);
    tell_player(p, oldstack);
    stack = oldstack;
    return;
  }
  strcpy(name, firspace);
  lower_case(name);
  sp = find_saved_player(name);
  if (sp)
  {
    sprintf(stack, " There is already a person with the name '%s' "
	    "in the player files.\n", sp->lower_name);
    stack = end_string(stack);
    tell_player(p, oldstack);
    stack = oldstack;
    return;
  }

  while (*letter)
  {
    if (!isalpha(*letter))
    {
      tell_player(p, " Letters in names only, please ...\n");
      stack = oldstack;
      return;
    }
    letter++;
  }

  /* right, newname doesn't exist then, safe to make a change (I hope) */
  /* Remove oldp from hash list */

  scan = hashlist[oldp->hash_top];
  previous = 0;
  while (scan && scan != oldp)
  {
    previous = scan;
    scan = scan->hash_next;
  }
  if (!scan)
    log("error", "Bad hash list (rename)");
  else if (!previous)
    hashlist[oldp->hash_top] = oldp->hash_next;
  else
    previous->hash_next = oldp->hash_next;

  strcpy(name, oldp->lower_name);
  strncpy(oldp->name, firspace, MAX_NAME - 3);
  lower_case(firspace);
  strncpy(oldp->lower_name, firspace, MAX_NAME - 3);

  /* now place oldp back into named hashed lists */

  hash = ((int) (oldp->lower_name[0]) - (int) 'a' + 1);
  oldp->hash_next = hashlist[hash];
  hashlist[hash] = oldp;
  oldp->hash_top = hash;

  if (oldp->saved)
    save_player(oldp);
  stack = oldstack;
  if (verbose)
  {
    sprintf(stack, " %s dissolves in front of your eyes, and "
	    "rematerialises as %s ...\n", name, oldp->name);
    stack = end_string(stack);

    /* tell room */
    scan = oldp->location->players_top;
    while (scan)
    {
      if (scan != oldp && scan != p)
	tell_player(scan, oldstack);
      scan = scan->room_next;
    }
    stack = oldstack;
    sprintf(stack, "\n -=*> %s has just changed your name to be '%s' ...\n\n",
	    p->name, oldp->name);
    stack = end_string(stack);
    tell_player(oldp, oldstack);
  }

  /* log it */
  LOGF("rename", "Rename by %s - %s to %s", p->name, name, oldp->name);
  stack = oldstack;
  sprintf(stack, " -=*> %s renames %s to %s.\n", p->name, name,
	  oldp->name);
  stack = end_string(stack);
  su_wall(oldstack);
  stack = oldstack;
}


/* User interface to renaming a newbie */

void rename_player(player * p, char *str)
{
  do_rename(p, str, 1);
}

void quiet_rename(player * p, char *str)
{
  CHECK_DUTY(p);

  do_rename(p, str, 0);
}


/* reset email of a player */
/* this version even manages to check if they are logged in at the time :-/ */
/* leave the old one in a little while until we are sure this works */

void blank_email(player * p, char *str)
{
  player dummy;
  player *p2;
  char *space, *oldstack;

  CHECK_DUTY(p);

  /* we need the stack for printing some stuff */
  oldstack = stack;

  /* spot incorrect syntax */

  if (!*str)
  {
    tell_player(p, " Format: blank_email <player> [<email>]\n");
    return;
  }

  /* spot lack of sensible email address */
  space = 0;
  space = strchr(str, ' ');
  if (space != NULL)
  {
    *space++ = 0;
    if (strlen(space) < 7)
    {
      tell_player(p, " Try a reasonable email address.\n");
      return;
    }
  }
  /* look for them on the prog */
  lower_case(str);
  p2 = find_player_absolute_quiet(str);

  /* if player logged in */
  if (p2)
  {
    /* no blanking the emails of superiors... */
    if (!check_privs(p->residency, p2->residency))
      /* naughty, naughty, so tell the person, the target, and the
         su channel */
    {
      tell_player(p, " You cannot blank that person's email address.\n");
      sprintf(stack, " -=*> %s tried to blank your email address, but "
	      "failed.\n", p->name);
      stack = end_string(stack);
      tell_player(p2, oldstack);
      stack = oldstack;
      sprintf(stack, " -=*> %s failed in an attempt to blank the email "
	      "address of %s.\n", p->name, p2->name);
      stack = end_string(stack);
      su_wall_but(p, oldstack);
      stack = oldstack;
      return;
    }
    else
      /* p is allowed to do things to p2 */
    {
      /* tell the target and the SUs all about it */
      if (space == NULL)
	sprintf(stack, " -=*> Your email address has been blanked "
		"by %s.\n", p->name);
      else
	sprintf(stack, " -=*> Your email address has been changed "
		"by %s.\n", p->name);
      stack = end_string(stack);
      tell_player(p2, oldstack);
      stack = oldstack;
      if (space == NULL)
	sprintf(stack, " -=*> %s has their email blanked by %s.\n",
		p2->name, p->name);
      else
	sprintf(stack, " -=*> %s has their email changed by %s.\n",
		p2->name, p->name);
      stack = end_string(stack);
      su_wall_but(p, oldstack);
      stack = oldstack;

      /* actually blank it, and flag the player for update */
      /* and avoid strcpy from NULL since it's very dodgy */
      if (space != NULL)
	strcpy(p2->email, space);
      else
	p2->email[0] = 0;
      set_update(*str);

      /* report success to the player */
      if (space == NULL)
	sprintf(stack, " -=*> You successfully blank %s's email.\n",
		p2->name);
      else
	sprintf(stack, " -=*> You successfully change %s's email.\n",
		p2->name);
      stack = end_string(stack);
      tell_player(p, oldstack);
      stack = oldstack;
      /* log the change */
      if (space == NULL)
	sprintf(stack, "%s blanked %s's email address (logged in)",
		p->name, p2->name);
      else
	sprintf(stack, "%s changed %s's email address (logged in)",
		p->name, p2->name);
      stack = end_string(stack);
      log("blanks", oldstack);
      return;
    }
  }
  else
    /* they are not logged in, so load them */
    /* set up the name and port first */
  {
    strcpy(dummy.lower_name, str);
    dummy.fd = p->fd;
    if (load_player(&dummy))
    {
      /* might as well point this out if it is so */
      if (dummy.residency & BANISHD)
      {
	tell_player(p, " By the way, this player is currently BANISHED.");
	if (dummy.residency == BANISHD)
	{
	  tell_player(p, " (Name Only)\n");
	}
	else
	{
	  tell_player(p, "\n");
	}
      }
      /* announce to the SU channel */
      if (space == NULL)
	sprintf(stack, " -=*> %s blanks the email of %s, who is "
		"logged out at the moment.\n", p->name, dummy.name);
      else
	sprintf(stack, " -=*> %s changes the email of %s, who is "
		"logged out at the moment.\n", p->name, dummy.name);
      stack = end_string(stack);
      su_wall_but(p, oldstack);
      stack = oldstack;
      /* change or blank the email address */
      if (space == NULL)
	dummy.email[0] = 0;
      else
	strcpy(dummy.email, space);

      /* report success to player */
      if (space == NULL)
	sprintf(stack, " -=*> Successfully blanked the email of %s, "
		"not logged in atm.\n", dummy.name);
      else
	sprintf(stack, " -=*> Successfully changed the email of %s, "
		"not logged in atm.\n", dummy.name);
      stack = end_string(stack);
      tell_player(p, oldstack);
      stack = oldstack;

      /* and log it */
      if (space == NULL)
	sprintf(stack, "%s blanked %s's email address (logged out)",
		p->name, dummy.name);
      else
	sprintf(stack, "%s changed %s's email address (logged out)",
		p->name, dummy.name);
      stack = end_string(stack);
      log("blanks", oldstack);
      stack = oldstack;

      /* save char LAST thing so maybe we won't blancmange the files */
      dummy.script = 0;
      dummy.script_file[0] = 0;
      dummy.flags &= ~SCRIPTING;
      dummy.location = (room *) - 1;
      save_player(&dummy);
      return;
    }
    else
      /* name does not exist, tell the person so and return */
    {
      sprintf(stack, " -=*> The name '%s' was not found in saved files.\n", str);
      stack = end_string(stack);
      tell_player(p, oldstack);
      stack = oldstack;
      return;
    }
  }
}

/* The almighty dumb command!!!! */

void dumb(player * p, char *str)
{
  player *d;

  CHECK_DUTY(p);

  if (!*str)
  {
    tell_player(p, " Format: dumb <player>\n");
    return;
  }

  if (!strcasecmp(str, "me"))
  {
    tell_player(p, " Do you REALLY want to tweedle yourself?\n");
    return;
  }
  d = find_player_global(str);
  if (d)
  {
    if (d == p)
    {
      tell_player(p, " Do you REALLY want to tweedle yourself?\n");
      return;
    }
    if (d->flags & FROGGED)
    {
      tell_player(p, " That player is ALREADY a tweedle\n");
      return;
    }
    if (!check_privs(p->residency, d->residency))
    {
      tell_player(p, " You can't do that!\n");
      TELLPLAYER(d, " -=*> %s tried to tweedle you!\n", p->name);
      return;
    }
    d->flags |= FROGGED;
    d->system_flags |= SAVEDFROGGED;
    TELLPLAYER(p, " You turn %s into a tweedle!\n", d->name);
    TELLPLAYER(d, " -=*> %s turns you into a tweedle!\n",
	       p->name);
    SW_BUT(p, " -=*> %s turns %s into a tweedle!\n", p->name,
	   d->name);
    LOGF("dumb", "%s turns %s into a tweedle", p->name,
	 d->name);
  }
}

/* Well, I s'pose we'd better have this too */

void undumb(player * p, char *str)
{
  player *d;
  saved_player *sp;

  CHECK_DUTY(p);

  if (!*str)
  {
    tell_player(p, " Format: undumb <player>\n");
    return;
  }

  d = find_player_global(str);
  if (d)
  {
    if (d == p)
    {
      if (p->flags & FROGGED)
      {
	tell_player(p, " You can't, you spoon!\n");
	SW_BUT(p, " -=*> %s tries to untweedle %s...\n", p->name,
	       self_string(p));
      }
      else
	tell_player(p, " But you're not a tweedle...\n");
      return;
    }
    if (!(d->flags & FROGGED))
    {
      tell_player(p, " That person isn't a tweedle...\n");
      return;
    }
    d->flags &= ~FROGGED;
    d->system_flags &= ~SAVEDFROGGED;
    TELLPLAYER(d, " -=*> %s zaps you and you are no longer a tweedle.\n",
	       p->name);
    TELLPLAYER(p, " You zap %s and %s is no longer a tweedle.\n", d->name,
	       gstring(d));
    SW_BUT(p, " -=*> %s zaps %s, and %s is no longer a tweedle.\n",
	   p->name, d->name, gstring(d));
    LOGF("dumb", "%s untweedles %s", p->name, d->name);
  }
  else
  {
    tell_player(p, " Checking saved files...\n");
    sp = find_saved_player(str);
    if (!sp)
    {
      tell_player(p, " Not found.\n");
      return;
    }
    if (!(sp->system_flags & SAVEDFROGGED))
    {
      tell_player(p, " But that person isn't a tweedle...\n");
      return;
    }
    sp->system_flags &= ~SAVEDFROGGED;
    TELLPLAYER(p, " Ok, %s is no longer a tweedle.\n", sp->lower_name);
    SW_BUT(p, " -=*> %s untweedles %s.\n", p->name, sp->lower_name);
    LOGF("dumb", " %s untweedles %s.", p->name, sp->lower_name);
  }
}


/* the yoyo commannd changed to the WHOMP messages (astyanax 5/2/95) */

void yoyo(player * p, char *str)
{
  player *p2;
  int tmp_nrd = 0;

  CHECK_DUTY(p);

  if (!*str)
  {
    tell_player(p, " Format: yoyo <player>\n");
    return;
  }

  p2 = find_player_global(str);
  if (!p2)
    return;

  if (!check_privs(p->residency, p2->residency))
  {
    TELLPLAYER(p, " You fail to kick %s's ass anywhere - uh oh...\n", p2->name);
    TELLPLAYER(p2, " -=*> %s tried to kick your ass,  Sick 'em!\n", p->name);
    return;
  }
  SUWALL(" -=*> %s kicks the crap out of %s.\n", p->name, p2->name);
  LOGF("yoyo", "%s kicked %s's ass", p->name, p2->name);
  /* can't let them avoid the yoyo just by blocking room desc... */
  if (p2->tag_flags & BLOCK_ROOM_DESC)
  {
    tmp_nrd = 1;
    p2->tag_flags &= ~BLOCK_ROOM_DESC;
  }
  command_type |= ADMIN_BARGE;
  TELLROOM(p2->location, " %s is in need of a medic after an encounter with"
	   " some %s!\n", p2->name, get_config_msg("staff_name"));
  trans_to(p2, sys_room_id("boot"));
  TELLROOM(p2->location, " %s staggers in reeling from a blow from some SU and then vanishes!\n",
	   p2->name);
  trans_to(p2, sys_room_id("room"));
  TELLROOM(p2->location, " %s falls back on earth bruised and battered *plop*\n", p2->name);
  command_type |= HIGHLIGHT;
  tell_player(p2, "  You musta been bad cause some SU just used you as a punching bag!\n");
  tell_player(p2, " If you'd rather not have it happen again, take a look"
	      " at the rules and consider following them...\n");
  tell_player(p2, " Have a nice day *smirk*\n");
  command_type &= ~HIGHLIGHT;
  if (tmp_nrd)
  {
    p2->tag_flags |= BLOCK_ROOM_DESC;
  }
}

void sban(player * p, char *str)
{
  char *oldstack, *site_alpha;


  CHECK_DUTY(p);

  if (!*str)
  {
    tell_player(p, " Format: site_ban <site_num> <reason>\n");
    return;
  }

  if (!isdigit(*str))
  {
    tell_player(p, " You can only site_ban numeric (IP) addresses. try again.\n");
    return;
  }
  site_alpha = next_space(str);

  if (!*site_alpha)
  {
    tell_player(p, " Format: site_ban <site_num> <reason>\n");
    return;
  }

  *site_alpha++ = 0;
  tell_player(p, " Site has been completely banned ...\n");
  oldstack = stack;
  sprintf(stack, "%s  C     # %s (%s)\n", str, site_alpha, p->name);
  stack = end_string(stack);
  banlog("banish", oldstack);
  stack = oldstack;
  if (banish_file.where)
    FREE(banish_file.where);
  banish_file = load_file("files/banish");
  tell_player(p, " New banish file uploaded.\n");
}

void nban(player * p, char *str)
{
  char *oldstack, *site_alpha;


  CHECK_DUTY(p);

  if (!*str)
  {
    tell_player(p, " Format: newbie_ban <site_num> <reason>\n");
    return;
  }

  if (!isdigit(*str))
  {
    tell_player(p, " You can only newbie_ban numeric (IP) addresses. try again.\n");
    return;
  }
  site_alpha = next_space(str);

  if (!*site_alpha)
  {
    tell_player(p, " Format: newbie_ban <site_num> <reason>\n");
    return;
  }

  *site_alpha++ = 0;
  tell_player(p, " Site has been newbie banned ...\n");
  oldstack = stack;
  sprintf(stack, "%s  N         # %s (%s)\n", str, site_alpha, p->name);
  stack = end_string(stack);
  banlog("banish", oldstack);
  stack = oldstack;
  if (banish_file.where)
    FREE(banish_file.where);
  banish_file = load_file("files/banish");
  tell_player(p, " New banish file uploaded.\n");
}


/* For an SU to go back on duty */

void on_duty(player * p, char *str)
{
  player *p2;

  if ((*str) && (p->residency & ADMIN))
  {
    p2 = find_player_global(str);
    if (!p2)
      return;
    else if (!(p2->residency & SU))
    {
      tell_player(p, " -=*> What?! That person isn't even an SU! *boggle*\n");
      return;
    }
    else if ((p2->flags & BLOCK_SU) == 0)
    {
      tell_player(p, " Ummmm... that player is ALREADY on_duty...\n");
      return;
    }
    else if (!check_privs(p->residency, p2->residency))
    {
      tell_player(p, " You can't to that to someone higher than you :o)\n");
      return;
    }
    else
      /* ok - so p2 is an off_duty su. Lets get the slacker back on duty */
    {
      p2->flags &= ~BLOCK_SU;
      p2->suh_seen = 0;
      TELLPLAYER(p2, " -=*> %s forces your ass back to work...\n", p->name);
      TELLPLAYER(p, " -=*> You force %s back on_duty... \n", p2->name);
      LOGF("duty", " -=*> %s forces %s to return to duty...", p->name, p2->name);
      SW_BUT(p2, " -=*> %s forces %s to return to duty...\n", p->name, p2->name);
    }
  }
  else
  {
    p2 = p;

    if ((p->flags & BLOCK_SU) != 0)
    {
      p->flags &= ~BLOCK_SU;
      if ((p->flags & OFF_LSU) != 0)
	p->flags &= ~OFF_LSU;
      tell_player(p, " You return to duty.\n");
      p->residency = p->saved_residency;
      LOGF("duty", "%s --> onduty", p->name);
      SW_BUT(p2, " -=*> %s returns to duty.\n", p->name);
      p2->suh_seen = 0;
    }
    else if ((p->flags & OFF_LSU) != 0)
    {
      p->flags &= ~OFF_LSU;
      tell_player(p, " You return to visible duty.\n");
      p->residency = p->saved_residency;
    }
    else
    {
      tell_player(p, " Are you asleep or something? You are ALREADY on visible duty!"
		  " <smirk>\n");
    }
  }
}


/* For an SU to go off duty */

void block_su(player * p, char *str)
{
  player *p2;

  if ((*str) && (p->residency & ADMIN))
  {
    p2 = find_player_global(str);
    if (!p2)
    {
      return;
    }
    else if (!(p2->residency & SU))
    {
      tell_player(p, " What?! That person isn't even an SU! *boggle*\n");
      return;
    }
    else if ((p2->flags & BLOCK_SU) != 0)
    {
      tell_player(p, " Ummmm... that player is ALREADY off_duty...\n");
      return;
    }
    else if (!check_privs(p->residency, p2->residency))
    {
      tell_player(p, " You can't to that to someone higher than you :o)\n");
      return;
    }
    else
      /* ok - so p2 is an on_duty su. Lets give the git a break */
    {
      p2->flags |= BLOCK_SU;
      TELLPLAYER(p2, " -=*> %s forces you to take a break...\n", p->name);
      TELLPLAYER(p, " -=*> You force %s to take a break... \n", p2->name);
      p2->residency = p2->saved_residency;
      LOGF("duty", " -=*> %s forces %s to take a break...", p->name, p2->name);
      SW_BUT(p2, " -=*> %s forces %s to take a break...\n", p->name, p2->name);
    }
  }
  else
  {
    p2 = p;
    if ((p->flags & BLOCK_SU) == 0)
    {
      p->flags |= BLOCK_SU;
      tell_player(p, " You're now off duty ... "
		  "\n");

      p->saved_residency = p->residency;

      SW_BUT(p2, " -=*> %s goes off duty.\n", p->name);
      LOGF("duty", "%s --> offduty", p->name);
    }
    else
    {
      tell_player(p, " But you are ALREADY Off Duty! <boggle>\n");
    }
  }
}

/* For an admin to go back on visible duty */

void on_lsu(player * p, char *str)
{
  player *p2;

  if ((*str) && (p->residency & ADMIN))
  {
    p2 = find_player_global(str);
    if (!p2)
    {
      return;
    }
    else if (!(p2->residency & LOWER_ADMIN))
    {
      tell_player(p, " What?! That person isn't even an admin! *boggle*\n");
      return;
    }
    else if ((p2->flags & OFF_LSU) == 0)
    {
      tell_player(p, " Ummmm... that player is ALREADY on_lsu...\n");
      return;
    }
    else
      /* ok - so p2 is an off_lsu su. Lets get the slacker back on duty */
    {
      p2->flags &= ~OFF_LSU;
      TELLPLAYER(p2, " -=*> %s forces you back onto ressie lsu...\n", p->name);
      TELLPLAYER(p, " -=*> You force %s back onto lsu... \n", p2->name);
      p2->residency = p2->saved_residency;
    }
  }
  else
  {
    p2 = p;
    if ((p->flags & OFF_LSU) != 0)
    {
      p->flags &= ~OFF_LSU;
      tell_player(p, " You return to visible duty.\n");
      p->residency = p->saved_residency;
    }
    else
    {
      tell_player(p, " Are you asleep or something? You are ALREADY On lsu!"
		  " <smirk>\n");
    }
  }
}

/* For an admin to go off visible duty */

void off_lsu(player * p, char *str)
{
  player *p2;

  if ((*str) && (p->residency & ADMIN))
  {
    p2 = find_player_global(str);
    if (!p2)
    {
      return;
    }
    else if (!(p2->residency & LOWER_ADMIN))
    {
      tell_player(p, " What?! That person isn't even an admin! *boggle*\n");
      return;
    }
    else if ((p2->flags & OFF_LSU) != 0)
    {
      tell_player(p, " Ummmm... that player is ALREADY off_duty...\n");
      return;
    }
    else
      /* ok - so p2 is an on_duty su. Lets give the git a break */
    {
      p2->flags |= OFF_LSU;
      TELLPLAYER(p2, " -=*> %s removes you from ressie lsu...\n", p->name);
      TELLPLAYER(p, " -=*> You force %s off of lsu... \n", p2->name);
      p2->residency = p2->saved_residency;
    }
  }
  else
  {
    p2 = p;
    if ((p->flags & OFF_LSU) == 0)
    {
      p->flags |= OFF_LSU;
      tell_player(p, " You're now off visible duty ... "
		  "no more ressies buggin ya ..."
		  "\n");

      p->saved_residency = p->residency;
    }
    else
    {
      tell_player(p, " But you are ALREADY off visible duty! <boggle>\n");
    }
  }
}

/* Marriage - not again *sigh* - astyanax & traP */
/* hey beavis, *I* didn't invent the damn thing ;)  */
void marry(player * p, char *str)
{
  player *p2;
  player *p3;
  player *scan;
  int change;
  char *temp;
  temp = next_space(str);

  if (!*str)
  {
    tell_player(p, " Format: marry <player> <player>\n");
    return;
  }

  change = MARRIED;
  *temp++ = 0;
  p2 = find_player_global(str);
  p3 = find_player_global(temp);


  if ((!p2))
  {
    tell_player(p, " Don't you think that it'd be nice to have the BRIDE AND GROOM HERE? *boggle*\n");
    return;
  }
  if ((!p3))
  {
    tell_player(p, " Well... marry to WHO? it takes 2 to tango... \n");
    return;
  }
  if (p2 == p3)
  {
    tell_player(p, " Wouldn't that be just a bit odd?\n");
    return;
  }
  if ((p2->system_flags & MARRIED) || (p3->system_flags & MARRIED))
  {
    tell_player(p, " But at least one of them is ALREADY married.\n");
    return;
  }
  if ((p2->residency == NON_RESIDENT) || (p3->residency == NON_RESIDENT))
  {
    tell_player(p, " That player is non-resident!\n");
    return;
  }
  if ((p2->residency == BANISHD || p2->residency == SYSTEM_ROOM)
      || (p3->residency == BANISHD || p3->residency == SYSTEM_ROOM))
  {
    tell_player(p, " That is a banished NAME, or System Room.\n");
    return;
  }
  if (!(p2->system_flags & ENGAGED) || !(p3->system_flags & ENGAGED) ||
   (strcmp(p2->married_to, p3->name)) || (strcmp(p3->name, p2->married_to)))
  {
    tell_player(p, " They aren't engaged to each other !!\n");
    return;
  }
  TELLPLAYER(p2, "\n\n -=*> You are now officially net.married.\n\n");
  TELLPLAYER(p3, "\n\n -=*> You are now officially net.married.\n\n");
  p2->system_flags |= MARRIED;
  p3->system_flags |= MARRIED;
  LOGF("marry", "%s married %s and %s", p->name, p2->name, p3->name);
  strncpy(p2->married_to, p3->name, MAX_NAME - 3);
  strncpy(p3->married_to, p2->name, MAX_NAME - 3);
  save_player(p2);
  save_player(p3);
  tell_player(p, " And they lived happily ever after...\n");

  if (config_flags & cfWALLMARRIGE)
    for (scan = flatlist_start; scan; scan = scan->flat_next)
      if (scan != p2 && scan != p3)
	TELLPLAYER(scan, "\n -=*> %s and %s are now married !!!\n",
		   p2->name, p3->name);

}

/* And they lived happily ever after .......until!! */

void divorce(player * p, char *str)
{
  player *p2, *p3, *scan;
  int change;
  char *tmp;

  tmp = next_space(str);
  if (!*str || *tmp)
  {
    tell_player(p, " Format: divorce <whoever>\n");
    return;
  }
  change = MARRIED;
  change |= ENGAGED;		/* just in case */
  p2 = find_player_global(str);
  if (!p2)
  {
    tell_player(p, " Erm... that's not nice... The person requesting divorce"
		" must be logged on... \n");
    return;
  }
  p3 = find_player_global(p2->married_to);
  if (!p3)
  {
    tell_player(p, "The mate is not logged on - ignoring.\n");
  }
  {
    p2->system_flags &= ~change;
    strncpy(p2->married_to, "", MAX_NAME - 3);
    if (p3)
    {
      p3->system_flags &= ~change;
      strncpy(p3->married_to, "", MAX_NAME - 3);
    }
    tell_player(p2, "\n\n You are now divorced. \n");
    if (p3)
    {
      tell_player(p3, "\n\n You are now divorced. \n");
    }
    if (p3)
      LOGF("marry", " %s divorced %s & %s", p->name, p2->name, p3->name);
    else
      LOGF("marry", " %s granted an anulment to %s", p->name, p2->name);
    if (p2->residency != NON_RESIDENT)
      save_player(p2);
    else
      remove_player_file(p2->lower_name);
    if (p3)
    {
      if (p3->residency != NON_RESIDENT)
	save_player(p3);
      else
	remove_player_file(p3->lower_name);
    }
    tell_player(p, " A wise being once said ..All good things must come to an end.\n");
  }
  if (config_flags & cfWALLDIVORCE && p3)
    for (scan = flatlist_start; scan; scan = scan->flat_next)
      if (scan != p2 && scan != p3)
	TELLPLAYER(scan, "\n -=*> %s and %s are have gotten a divorce (awe)\n",
		   p2->name, p3->name);

}

/* decapitate =) change what users type into all caps all the time */
void decap_player(player * p, char *str)
{
  player *p2;

  if (!*str)
  {
    tell_player(p, " Format: decap <person>\n");
    return;
  }

  CHECK_DUTY(p);
  p2 = find_player_global(str);
  if (p2)
  {
    if (!check_privs(p->residency, p2->residency))
    {
      tell_player(p, " No way beavis!!\n");
      TELLPLAYER(p2, "%s tried to decap you...\n", p->name);
      return;
    }
    if (p2->system_flags & DECAPPED)
    {
      SUWALL(" -=*> %s regrants caps to %s.\n", p->name, p2->name);
      p2->system_flags &= ~DECAPPED;
      tell_player(p2, " -=> Your shift key is returned to you.\n");
    }
    else
    {
      SUWALL(" -=*> %s decides to break %s's caps lock key.\n", p->name, p2->name);
      LOGF("decap", " %s removes caps from %s.", p->name, p2->name);
      tell_player(p2, " -=*> Someone nukes your shift key!! oi!\n");
      p2->system_flags |= DECAPPED;
    }
  }
}

/* some more stuff for marrying -- net.propose */

void net_propose(player * p, char *str)
{

  player *q, *scan;
  list_ent *l;

  if (!*str)
  {
    tell_player(p, " Format: propose <player>\n");
    return;
  }

  if (p->system_flags & MARRIED)
  {
    tell_player(p, " But you are already married !!\n");
    return;
  }
  if (p->system_flags & ENGAGED)
  {
    tell_player(p, " But you are already engaged !!\n");
    return;
  }
  if (p->flags & WAITING_ENGAGE)
  {
    tell_player(p, " You already have an offer pending.\n");
    return;
  }
  if (p->tag_flags & NO_PROPOSE)
  {
    tell_player(p, " Not when you yourself are blocking proposals.\n");
    return;
  }
  q = find_player_global(str);
  if (!q)
    return;
  if (q == p)
  {
    tell_player(p, " That'd be SOME feat...\n");
    return;
  }
  if (q->system_flags & MARRIED)
  {
    tell_player(p, " Sorry, that person is already married.\n");
    return;
  }
  if (q->system_flags & ENGAGED)
  {
    tell_player(p, " Sorry, that person is already engaged.\n");
    return;
  }
  if (q->flags & WAITING_ENGAGE)
  {
    tell_player(p, " That person already has a standing offer...\n");
    return;
  }
  if (!(q->residency) || q->residency & NO_SYNC)
  {
    tell_player(p, " Sorry, you cannot be engaged to a non-resident.\n");
    return;
  }
  if (q->flags & NO_PROPOSE)
  {
    tell_player(p, " That person is blocking proposals...\n");
    return;
  }
  l = fle_from_save(q->saved, p->lower_name);
  /* since this is a ressie command, check that p isn't ignored by q */
  if (l && (l->flags & IGNORE || l->flags & BLOCK))
  {
    tell_player(p, " I don't think that person likes you...\n");
    return;
  }

  /* OK, we have two legal people.. lets do it! */

  TELLPLAYER(q, "\n"
	     "  %s has asked you to marry %s!\n"
	     "  Type \"accept %s\" to become engaged or \"reject %s\"\n"
	     "  to crush %s heart ...\n\n", p->name, get_gender_string(p), p->name, p->name, gstring_possessive(p));

  TELLPLAYER(p, " You get on your knees and propose to %s ...\n", q->name);
  strncpy(p->married_to, q->name, MAX_NAME - 3);
  q->flags |= WAITING_ENGAGE;
  p->flags |= WAITING_ENGAGE;

  /* Tell everyone else! */

  if (config_flags & cfWALLPROPOSE)
    for (scan = flatlist_start; scan; scan = scan->flat_next)
      if (scan != p && scan != q)
	TELLPLAYER(scan, "\n -=*> %s has just proposed to %s!\n",
		   p->name, q->name);

}

/* to agree to be engaged to a person */
void acc_engage(player * p, char *str)
{

  player *q, *scan;

  if (!*str)
  {
    tell_player(p, " Format: accept <player>\n");
    return;
  }

  if (!(p->flags & WAITING_ENGAGE))
  {
    tell_player(p, " No offer has been made.\n");
    return;
  }
  q = find_player_global(str);
  if (!q)
  {
    p->flags &= ~WAITING_ENGAGE;
    return;
  }
  if (strcasecmp(q->married_to, p->name))
  {
    tell_player(p, " Whoops, wrong person! Typo I assume.. try again\n");
    return;
  }
  else
  {
    if (!(q->flags & WAITING_ENGAGE))
    {
      tell_player(p, " Odd.. very odd...\n");
      return;
    }
    /* otherwise */
    p->flags &= ~WAITING_ENGAGE;
    q->flags &= ~WAITING_ENGAGE;
    p->system_flags |= ENGAGED;
    q->system_flags |= ENGAGED;
    strncpy(p->married_to, q->name, MAX_NAME - 3);
    TELLPLAYER(q, "\n %s whispers 'yes, I will marry you!'\n You are now net.engaged to %s.\n", p->name, p->name);
    TELLPLAYER(p, " You are now net.engaged to %s.\n", q->name);

    /* Tell everyone else! */

    if (config_flags & cfWALLACCEPT)
      for (scan = flatlist_start; scan; scan = scan->flat_next)
	if (scan != p && scan != q)
	  TELLPLAYER(scan, "\n -=*> %s says 'yes' to %s!\n",
		     p->name, q->name);

  }
}


/* to crush a person's heart */
void reject(player * p, char *str)
{

  player *q, *scan;

  if (!*str)
  {
    tell_player(p, " Format: reject <player>\n");
    return;
  }

  if (!(p->flags & WAITING_ENGAGE))
  {
    tell_player(p, " No offer has been made.\n");
    return;
  }
  q = find_player_global(str);
  if (!q)
  {
    p->flags &= ~WAITING_ENGAGE;
    return;
  }
  if (strcasecmp(q->married_to, p->name))
  {
    tell_player(p, " Whoops, wrong person! Typo I assume.. try again\n");
    return;
  }
  else
  {
    if (!(q->flags & WAITING_ENGAGE))
    {
      tell_player(p, " Odd.. very odd...\n");
      return;
    }
    /* otherwise */
    p->flags &= ~WAITING_ENGAGE;
    q->flags &= ~WAITING_ENGAGE;
    strncpy(p->married_to, "", MAX_NAME - 3);
    strncpy(q->married_to, "", MAX_NAME - 3);
    TELLPLAYER(q, " %s says \"No, I can't marry you.\"\n", p->name);
    TELLPLAYER(p, " You crush %s's heart.\n", q->name);

    /* Tell everyone else! */

    if (config_flags & cfWALLREJECT)
      for (scan = flatlist_start; scan; scan = scan->flat_next)
	if (scan != p && scan != q)
	  TELLPLAYER(scan, "\n -=*> Awww, %s turns down the proposal of %s ...\n",
		     p->name, q->name);


  }
}

/* and a commarejectnd to cancel the engagement */
void cancel_engage(player * p, char *str)
{

  player *z, *scan;		/* yes, just to be different */

  if (p->system_flags & MARRIED)
  {
    TELLPLAYER(p, " Isn't a little late to call off the engagement"
	       " since you're already married? Ask a minister"
	       " to grant a divorce, instead.\n");
    return;
  }
  if (!(p->system_flags & (ENGAGED | WAITING_ENGAGE)))
  {
    TELLPLAYER(p, " You have to be engaged to someone before you can cancel!\n");
    if (p->flags & WAITING_ENGAGE)
      TELLPLAYER(p, " Try \"reject\" instead...\n");
    return;
  }
  /* what other checks do I need here? *sigh* */
  z = find_player_global(p->married_to);
  if (z)
  {
    TELLPLAYER(z, " -=*> %s has just broken off the engagement.\n",
	       p->name);
    TELLPLAYER(p, " -=*> You cancel the engagement...\n");
    z->system_flags &= ~(MARRIED | ENGAGED);
    strncpy(z->married_to, "", MAX_NAME - 3);
    z->system_flags &= ~(MARRIED | ENGAGED);
    z->flags &= ~WAITING_ENGAGE;
    save_player(z);

    /* Tell everyone else! */

    if (config_flags & cfWALLREJECT)
      for (scan = flatlist_start; scan; scan = scan->flat_next)
	if (scan != p && (!z || scan != z))
	  TELLPLAYER(scan, "\n -=*> %s cancels the engagement to %s!\n",
		     p->name, z->name);

  }
  else				/* in case for some odd reason, we get here ... */
    TELLPLAYER(p, " Your ex-spouse-to-be isnt online atm ... canceling anyways.\n");

  p->system_flags &= ~(MARRIED | ENGAGED);
  p->flags &= ~WAITING_ENGAGE;

  strncpy(p->married_to, "", MAX_NAME - 3);
  save_player(p);

}

/* and an admin command to clear fargles... */

void net_anul_all(player * p, char *str)
{
  player *f;

  if (!*str)
  {
    tell_player(p, " Format: anul <player>\n");
    return;
  }

  f = find_player_global(str);
  if (!f)
    return;
  f->system_flags &= ~MARRIED;
  f->system_flags &= ~ENGAGED;
  f->flags &= ~WAITING_ENGAGE;
  strncpy(f->married_to, "", MAX_NAME - 3);
}

/* Original used d->name instead of sp->lower_name (in SUWALL) and forgot
   to save the player afterwards, oops :o) --Silver */

/* Oh AND forgot to do a check privs --Silver */

void no_msgs(player * p, char *str)
{
  player *d, dummy;

  CHECK_DUTY(p);

  if (!*str)
  {
    tell_player(p, " Format: nomsg <player>\n");
    return;
  }

  d = find_player_global(str);
  if (!d)
  {
    tell_player(p, " Checking saved files...\n");
    strcpy(dummy.lower_name, str);
    lower_case(dummy.lower_name);
    dummy.fd = p->fd;
    if (!load_player(&dummy))
    {
      tell_player(p, " No such person in saved files.\n");
      return;
    }
    d = &dummy;
  }

  if (d == p)
  {
    tell_player(p, " Idiot. Thats YOU!\n");
    return;
  }

  if (!check_privs(p->residency, d->residency))
  {
    tell_player(p, " You can't do that, sorry!\n");
    return;
  }

  if (d->system_flags & NO_MSGS)
  {
    d->system_flags &= ~NO_MSGS;
    if (find_player_global_quiet(d->name))
      TELLPLAYER(d, " -=*> %s restores your messages.\n", p->name);
    TELLPLAYER(p, " You restore messages to %s.\n", d->name);
    LOGF("msg", " -=*> %s restores messages to %s", p->name, d->name);
    SW_BUT(p, " -=*> %s restores messages to %s\n", p->name, d->name);
  }
  else
  {
    d->system_flags |= NO_MSGS;
    if (find_player_global_quiet(d->name))
      TELLPLAYER(d, " -=*> %s removes your messages.\n", p->name);
    TELLPLAYER(p, " You removes messages from %s.\n", d->name);
    LOGF("msg", " -=*> %s removes messages from %s", p->name, d->name);
    SW_BUT(p, " -=*> %s removes messages from %s\n", p->name, d->name);
  }

  if (!find_player_global_quiet(d->name))
  {
    d->script = 0;
    d->script_file[0] = 0;
    d->flags &= ~SCRIPTING;
    d->location = (room *) - 1;
  }
  save_player(d);
}

/* remove sanity completely from the player (and the coders too) */

void fake_nuke_player(player * p, char *str)
{
  player *p2;
  char nuked[MAX_NAME] = "";
  char naddr[MAX_INET_ADDR] = "";

  CHECK_DUTY(p);

  if (!str)
  {
    tell_player(p, " Format: scare <player>\n");
    return;
  }

  p2 = find_player_absolute_quiet(str);
  if (!p2)
  {
    tell_player(p, "No such person on the program.\n");
    return;
  }
  if (!check_privs(p->residency, p2->residency))
  {
    tell_player(p, " You can't scare them !\n");
    TELLPLAYER(p2, " -=*> %s tried to scare you.  Yipes!!.\n", p->name);
    return;
  }
  strcpy(nuked, p2->name);
  strcpy(naddr, p2->inet_addr);
  /* time for a new nuke screen */
  TELLPLAYER(p2, nuke_msg.where);
  p->num_ejected++;
  p2->eject_count++;
  quit(p2, 0);
  SUWALL(" -=*> %s scares the shit out of %s, TOASTY!!\n -=*> %s "
	 "was from %s\n", p->name, nuked, nuked, naddr);
}

#ifdef AUTOSHUTDOWN
/* Auto-shutdown code by Silver */

void set_auto_shutdown(player * p, char *str)
{
  if (auto_sd == 1)
  {
    tell_player(p, " Auto-Shutdown is already active.\n");
    return;
  }
  auto_sd = 1;

  tell_player(p, " Auto-Shutdown has now been activated. The code will "
	      "automatically shutdown when there are no people on.\n");
  SW_BUT(p, " -=*> %s has started Auto-Shutdown ...\n", p->name);
  LOGF("shutdown", "%s has started Auto-Shutdown.", p->name);
}

void cancel_as(player * p, char *str)
{
  if (auto_sd == 0)
  {
    tell_player(p, " Auto-Shudown is not active! *sigh*\n");
    return;
  }
  if (!*str)
  {
    tell_player(p, " Format: cancel_as <reason>\n");
    return;
  }
  auto_sd = 0;
  tell_player(p, " Auto-Shutdown cancelled.\n");
  LOGF("shutdown", "%s cancels the Auto-Shutdown (reason: %s)", p->name, str);
  SW_BUT(p, " -=*> %s has cancelled Auto-Shutdown (reason: %s)\n",
	 p->name, str);
}
#endif

/* On line command priv changing - by Phypor (again *grin*) */

void change_command_privs(player * p, char *str)
{
  struct command *comlist = coms[0];
  struct command *scan;
  char *privs, *privs2;
  int newprivs, i;

  if (!(config_flags & cfPRIVCHANGE))
  {
    tell_player(p, " Priv changing is unavailable at this time.\n");
    return;
  }

  privs = next_space(str);
  *privs++ = '\0';

  if (!*privs)
  {
    tell_player(p, " Format: change_command_privs <command> <privs>\n");
    return;
  }

  newprivs = get_case_flag(permission_list, privs);
  if (!newprivs && strcasecmp(privs, "newbie"))
  {
    TELLPLAYER(p, " Sorry but '%s' isn't a valid permission ...\n", privs);
    return;
  }
  privs2 = get_flag_name(permission_list, privs);

  if (isalpha(*str))
    comlist = coms[((int) (tolower(*str)) - (int) 'a' + 1)];

  while (comlist && comlist->text && comlist->text[0])
  {
    if (!strcasecmp(str, comlist->text))
    {
      for (i = 0; i <= ((int) 'z' - (int) 'a' + 1); i++)
	for (scan = coms[i]; scan && scan->text; scan++)
	  if (scan->function == comlist->function)
	  {
	    scan->level = newprivs;
	    TELLPLAYER(p, " You change the '%s' command to %s level\n", scan->text, privs);
	    SW_BUT(p, " -=*> %s changes %s to %s level\n", p->name, str, privs);
	  }
      LOGF("commands", "%s changes %s to %s level", p->name, str, privs);
      return;
    }
    comlist++;
  }
  TELLPLAYER(p, " That priv you gave is valid, but the command '%s' cannot "
	     "be found.\n", str);
}


/*
 * view the suhistory, by astyanax (note the poor coding techniques! ;-)
 * (don't worry, I'll clean it up for you *grin* --Silver)
 */

void see_suhistory(player * p, char *str)
{
  int i, flag = 0;
  char *oldstack = stack;


  if (p->flags & BLOCK_SU)
  {
    tell_player(p, " Try going back on duty first, biiach! =)\n");
    return;
  }

  pstack_mid("Starting SU History");
  for (i = SUH_NUM - 1; i >= 0; i--)
    if (p->suh_seen >= i)
    {
      sprintf(stack, "%s", suhistory[i]);
      stack = strchr(stack, 0);
      flag = 1;
    }

  pstack_mid("End of SU History");
  *stack++ = 0;

  if (flag)
    pager(p, oldstack);		/* Suppose we better page this */
  else
    tell_player(p, " No SU history to view.\n");
  stack = oldstack;
}

/* saved warnings - for naughty people :o) --Silver */

void saved_warning(player * p, char *str)
{
  player dummy, *p2;
  char *msg;

  if (!*str)
  {
    tell_player(p, " Format: swarn <player> <warning>\n");
    return;
  }

  msg = next_space(str);
  if (*msg)
    *msg++ = 0;
  if (!*msg)
  {
    tell_player(p, " Format: swarn <player> <warning>\n");
    return;
  }

  p2 = find_player_absolute_quiet(str);
  if (p2)
  {
    TELLPLAYER(p, " %s is on %s! Would it be better just to warn them?\n",
	       p2->name, get_config_msg("talker_name"));
    return;
  }

  strcpy(dummy.lower_name, str);
  lower_case(dummy.lower_name);
  dummy.fd = p->fd;
  if (!load_player(&dummy))
  {
    tell_player(p, " No such person in saved files.\n");
    return;
  }
  p2 = &dummy;

  if (p2->residency == BANISHED)
  {
    TELLPLAYER(p, " That person is banished.\n");
    return;
  }

  if (strlen(p2->swarn_sender) > 0)
  {
    TELLPLAYER(p, " That person already has a saved warning from %s which reads:\n"
	       "  \"%s\"\n", p2->swarn_sender, p2->swarn_message);
    return;
  }

  if (strlen(msg) > MAX_SWARN)
  {
    tell_player(p, " That warning is too long. Please shorten a bit.\n");
    return;
  }

  strncpy(p2->swarn_sender, p->name, MAX_NAME);
  strncpy(p2->swarn_message, msg, MAX_SWARN);
  p2->script = 0;
  p2->script_file[0] = 0;
  p2->flags &= ~SCRIPTING;
  p2->location = (room *) - 1;
  save_player(p2);

  TELLPLAYER(p, " Saved warning to %s sent.\n", p2->name);
  SW_BUT(p, " -=*> %s save-warns %s: %s\n", p->name, p2->name, msg);
  LOGF("warn", "%s save_warns %s: %s", p->name, p2->name, msg);
  p->num_warned++;

}

/* Kill the angel dead --Silver */

void kill_angel(player * p, char *str)
{
  FILE *fp;
  int pid;

  tell_player(p, " Attempting to kill guardian angel ... ");

  fp = fopen("junk/ANGEL_PID", "r");
  if (!fp)
  {
    tell_player(p, "No PID file! Unsucessful.\n");
    LOGF("boot", "%s unsucessfully kills the guardian angel", p->name);
    return;
  }

  fscanf(fp, "%d", &pid);
  fclose(fp);

  /* Signal 9 used (SIGKILL) to politely tell the talker to "DIE NOW!"
     (sometimes I get carried away --Silver) */

  if (!kill(pid, 9))
  {
    tell_player(p, "Sucessful.\n");
    unlink("junk/ANGEL_PID");
    LOGF("boot", "%s kills the guardian angel", p->name);
  }
  else
  {
    tell_player(p, " Unsucessful!\n");
    LOGF("boot", "%s unsucessfully kills the guardian angel", p->name);
  }
}

/* so your talker has changed its staff here is an easy way
   to let your ressies know whut ranks are whut
 */
void show_rank_equivs(player * p, char *str)
{
  char *oldstack = stack;

  pstack_mid("Rank Equvelants");
  stack += sprintf(stack,
		   "\n                 %s compared to standard EW-Too\n\n",
		   get_config_msg("talker_name"));
  stack += sprintf(stack, "    %-30s  HC Admin\n", get_config_msg("hc_name"));
  stack += sprintf(stack, "    %-30s  Coder\n", get_config_msg("coder_name"));
  stack += sprintf(stack, "    %-30s  Admin\n", get_config_msg("admin_name"));
  stack += sprintf(stack, "    %-30s  Lower Admin\n", get_config_msg("la_name"));
  stack += sprintf(stack, "    %-30s  Advanced SU\n", get_config_msg("asu_name"));
  stack += sprintf(stack, "    %-30s  Superuser\n", get_config_msg("su_name"));
  stack += sprintf(stack, "    %-30s  Pseudo SU\n", get_config_msg("psu_name"));

  ENDSTACK(LINE);
  tell_player(p, oldstack);
  stack = oldstack;
}


void script(player * p, char *str)
{
  char *oldstack = stack;
  time_t t;

  if (p->flags & SCRIPTING)
  {
    if (!*str)
    {
      tell_player(p, " You are ALREADY scripting! ('script off' to turn"
		  " current scripting off)\n");
    }
    if (!strcasecmp(str, "off"))
    {
      p->flags &= ~SCRIPTING;
      TELLPLAYER(p, " -=*> Scripting stopped at %s\n", convert_time(time(0)));
      *(p->script_file) = 0;
      SUWALL(" -=*> %s has stopped continuous scripting.\n", p->name);
    }
    return;
  }
  if (!*str || !strcasecmp(str, "on"))
  {
    tell_player(p, " You must give a reason for starting scripting.\n");
    return;
  }
  t = time(0);
  strftime(stack, 16, "%y%m%d%H%M%S", localtime(&t));
  stack = end_string(stack);
  sprintf(p->script_file, "%s%s.script", p->name, oldstack);
  stack = oldstack;

  p->flags |= SCRIPTING;

  TELLPLAYER(p, " -=*> Scripting started at %s, for reason '%s'\n",
	     convert_time(time(0)), str);

  SUWALL(" -=*> %s has started continuous scripting with reason '%s'\n",
	 p->name, str);
}


/* screening */

void screen_newbies(player * p, char *str)
{
  if (p->flags & BLOCK_SU)
  {
    tell_player(p, " Get yer arse back on duty and try it.\n");
    return;
  }
  if (*str)
  {
    if (!strcasecmp("off", str) &&
	sys_flags & SCREENING_NEWBIES)
    {
      sys_flags &= ~SCREENING_NEWBIES;
      LOGF("newbies", "Newbie screening turned off by %s", p->name);
      SUWALL("\n -=*> %s lets in newbies without screening\n\n",
	     p->name);
      return;
    }
    else if (!strcasecmp("on", str) &&
	     !(sys_flags & SCREENING_NEWBIES))
    {
      sys_flags |= SCREENING_NEWBIES;
      sys_flags &= ~CLOSED_TO_NEWBIES;
      LOGF("newbies", "Newbie screening turned on by %s", p->name);
      SUWALL("\n -=*> %s sifts the newbies through a screen\n\n",
	     p->name);
      return;
    }
  }
  if (sys_flags & SCREENING_NEWBIES)
    TELLPLAYER(p, " %s currently screening newbies.\n", talker_name);
  else
    TELLPLAYER(p, " %s currently not screening newbies.\n", talker_name);
}


void show_screen_queue(player * p)
{
  player *scan;
  char *oldstack = stack;
  int hit = 0;

  pstack_mid("Current screening queue");

  stack += sprintf(stack, "\n");
  for (scan = flatlist_start; scan; scan = scan->flat_next)
    if (scan->input_to_fn == newbie_dummy_fn)
    {
      stack += sprintf(stack, "%-20s    %d seconds left\n", scan->name,
		       scan->timer_count);
      hit++;
    }
  stack = end_string(stack);

  if (!hit)
    tell_player(p, " The screening queue is currently empty ...\n");
  else
    TELLPLAYER(p, "%s\n" LINE, oldstack);
  stack = oldstack;
}

void newbie_allow(player * p, char *str)
{
  player *p2;
  player *cp = current_player;

  if (!*str)
  {
    show_screen_queue(p);
    return;
  }
  if (p->flags & BLOCK_SU)
  {
    tell_player(p, " Get yer ass on duty, then allow someone in.\n");
    return;
  }
  p2 = find_screend_player(str);
  if (!p2)
  {
    TELLPLAYER(p, " '%s' not found in the screening queue...\n", str);
    show_screen_queue(p);
    return;
  }
  SUWALL(" -=*> %s allows the entry of %s\n", p->name, p2->name);
  LOGF("screen", "%s allows %s [%s]", p->name, p2->name, p2->inet_addr);

  current_player = p2;
  tell_player(p2, newpage1_msg.where);
  do_prompt(p2, "Hit return to continue:");
  p2->input_to_fn = newbie_start;
  p2->timer_fn = login_timeout;
  p2->timer_count = 1800;
  current_player = cp;
}

void newbie_deny(player * p, char *str)
{
  player *p2;
  player *cp = current_player;

  if (!*str)
  {
    show_screen_queue(p);
    return;
  }
  if (p->flags & BLOCK_SU)
  {
    tell_player(p, " Get yer ass on duty, then deny em.\n");
    return;
  }
  p2 = find_screend_player(str);
  if (!p2)
  {
    TELLPLAYER(p, " '%s' not found in the screening queue...\n", str);
    show_screen_queue(p);
    return;
  }
  SUWALL(" -=*> %s denies access to %s\n", p->name, p2->name);
  LOGF("screen", "%s denies %s [%s]", p->name, p2->name, p2->inet_addr);

  current_player = p2;
  newbie_was_screend(p2);
  current_player = cp;

}
