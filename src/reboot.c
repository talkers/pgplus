/*
 * Playground+ - reboot.c
 * Code to perform a seamless reboot (c) phypor 1998
 * ---------------------------------------------------------------------------
 *
 *  Changes to original:
 *    - Change of #define's to point to correct path
 *    - Replacement of ez_wall, ez_tellplayer and ez_log to PG+ formats
 *    - All errors are sent to SU channel
 *    - Code will wait till people out of editor before rebooting
 *    - Slight changes to error messages
 *    - Some uninformative and badly spelt comments removed
 *    - Reset several "time" stats on reboot
 *    - Change of include file locations
 *    - Code tidely indented
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "include/config.h"
#include "include/player.h"
#include "include/proto.h"


/* Only add all this is we want seamless rebooting */
#ifdef SEAMLESS_REBOOT


#define REBOOTING_DIR		"junk/rebooting"
#define PLAYER_LIST_FILE	"junk/rebooting/_plist"
#define TALKER_SYSINFO_FILE	"junk/rebooting/_sysinfo"
#define CHILDS_PID_FILE		"junk/rebooting/_child_pid"


extern int main_descriptor;
extern int alive_descriptor;
#ifdef IDENT
extern void kill_ident_server(void);
#endif /* IDENT */


char rebooter[MAX_NAME];	/* use a global for nicer looking code */


typedef struct talker_system_info
{
  int
    main_fd, angel_fd, up_date, logins, sys_flags, session_reset;

  char
    session[MAX_SESSION], sess_name[MAX_NAME], rebooter[MAX_NAME];
}
talker_system_info;


int build_sysinfo(void)
{
  talker_system_info tsi;
  FILE *f;

  memset(&tsi, 0, sizeof(talker_system_info));

  tsi.main_fd = main_descriptor;
  tsi.angel_fd = alive_descriptor;
  tsi.up_date = up_date;
  tsi.logins = logins;
  tsi.sys_flags = sys_flags;
  tsi.session_reset = session_reset;
  strncpy(tsi.session, session, MAX_SESSION - 1);
  strncpy(tsi.sess_name, sess_name, MAX_NAME - 1);
  if (current_player)
    strncpy(tsi.rebooter, current_player->lower_name, MAX_NAME - 1);

  f = fopen(TALKER_SYSINFO_FILE, "w");
  if (!f)
  {
    SUWALL(" -=*> Failed to open reboot system info file [%s]\n", strerror(errno));
    LOGF("error", "Failed to open reboot system info file [%s]", strerror(errno));
    return -1;
  }
  fwrite((void *) &tsi, sizeof(talker_system_info), 1, f);
  fclose(f);
  return 0;
}

int build_loggedin_players_list(void)
{
  player *scan;
  char *oldstack = stack;
  int fd;

  /* build players list */

  /* get to the end of the flatlist */
  for (scan = flatlist_start; scan->flat_next; scan = scan->flat_next);

  /* work backwards through it, adding names to stack */
  for (; scan; scan = scan->flat_previous)
    if (scan->location)
      stack += sprintf(stack, "%s\n", scan->lower_name);

  stack = end_string(stack);

  fd = open(PLAYER_LIST_FILE, (O_WRONLY | O_CREAT |
			       O_NONBLOCK | O_TRUNC), (S_IRUSR | S_IWUSR));

  if (fd < 0)
  {
    SUWALL(" -=*> Failed to open reboot player list file [%s]\n", strerror(errno));
    LOGF("error", "Failed to open reboot player list file [%s]", strerror(errno));
    stack = oldstack;
    return -1;
  }
  write(fd, oldstack, strlen(oldstack));
  close(fd);
  stack = oldstack;
  return 0;
}

int build_loggedin_players_info(void)
{
  player *scan, *oldcp;
  char *oldstack = stack;
  char location_string[MAX_NAME + MAX_ID];
  FILE *f;

  for (scan = flatlist_start; scan; scan = scan->flat_next)
  {
    /* if the player doesnt have a location, no telling whut point
       they are in the login process, (or maybe they arent even logging in)
       we wont even attempt to keep up with them
     */
    if (!scan->location)
    {
      oldcp = current_player;	/* so the tell_player goes through */
      current_player = scan;
      tell_player(scan, "\n\n   Reboot in progress, losing connection.\n"
		  "   Please relogin.\n\n\n");
      current_player = oldcp;
      shutdown(scan->fd, 2);
      close(scan->fd);
      continue;
    }

    /* do this roundabout so we cant possibly overwrite field length */
    memset(location_string, 0, MAX_NAME + MAX_ID);
    sprintf(location_string, "%s.%s", scan->location->owner->lower_name,
	    scan->location->id);
    strncpy(scan->location_string, location_string, MAX_NAME + MAX_ID - 1);

    stack += sprintf(stack, "%s/%s", REBOOTING_DIR, scan->lower_name);
    stack = end_string(stack);
    f = fopen(oldstack, "w");
    if (!f)
    {
      SUWALL(" -=*> Failed to open reboot player info file (%s) [%s]\n", scan->lower_name, strerror(errno));
      LOGF("error", "Failed to open reboot player info (%s) [%s]", scan->lower_name, strerror(errno));
      stack = oldstack;
      return -1;
    }
    fwrite((void *) scan, sizeof(player), 1, f);
    fclose(f);
    stack = oldstack;
  }
  return 0;
}

void close_fds(void)
{
  int i, d = 0;
  player *scan;

  for (i = 4; i < (1 << 12); i++)	/* start at 4, so we dont kill stdout, etc */
  {
    if (i == main_descriptor ||
	i == alive_descriptor)
      continue;

    for (scan = flatlist_start; scan; scan = scan->flat_next)
      if (scan->fd == i)
	break;

    if (scan)
      continue;

    if (close(i) == 0)
      d++;
  }

}


void do_reboot(void)
{
  player *scan;
  int cpid;
  FILE *f;
  char server_name[256];
  struct itimerval itimer;

  sprintf(server_name, "-=> %s <=- Talk Server on port %d",
	  get_config_msg("talker_name"), atoi(get_config_msg("port")));

  SUWALL(" -=*> Starting reboot ...\n");

  if (build_sysinfo() < 0)
  {
    SUWALL(" -=*> Reboot failed (build_sysinfo) ...\n");
    return;
  }

  if (build_loggedin_players_list() < 0)
  {
    SUWALL(" -=*> Reboot failed (build_loggedin_players_list) ...\n");
    return;
  }
  if (build_loggedin_players_info() < 0)
  {
    SUWALL(" -=*> Reboot failed (build_loggedin_players_info) ...\n");
    return;
  }

  /* kill the timer */
  itimer.it_interval.tv_sec = itimer.it_interval.tv_usec = 0;
  itimer.it_value.tv_sec = itimer.it_value.tv_usec = 0;
  setitimer(ITIMER_REAL, &itimer, 0);


  for (scan = flatlist_start; scan; scan = scan->flat_next)
    save_player(scan);

  sys_flags |= SHUTDOWN;

  sync_all();
  sync_notes(0);
  sync_sitems(0);

#ifdef IDENT
  kill_ident_server();
#endif /* IDENT */

#ifdef INTERCOM
  kill_intercom();
#endif

  sync_all_news_headers();

#ifdef ALLOW_MULTIS
  reboot_save_multis();
  destroy_all_multis();
#endif

  /*
     here is where to add for any other syncing that needs to be done
     when your talker shutsdown... notes, etc
   */


  close_fds();

  cpid = getpid();
  f = fopen(CHILDS_PID_FILE, "w");
  if (f)
  {
    fprintf(f, "%d\n", cpid);
    fclose(f);
  }
  else
    LOGF("error", "Failed to fopen [%s] coz %s ... reboot fails.", CHILDS_PID_FILE, strerror(errno));


  execlp("bin/" TALKER_EXEC, server_name, (char *) NULL);

  sys_flags &= ~SHUTDOWN;

  log("error", "Failed to execlp!");
  SUWALL(" -=*> Failed to reboot ... !!!\n");

}
/* Modified by Silver to take into account that people may be in the
   editor when the code reboots */

void reboot_command(player * p, char *str)
{
  player *scan;
  char *oldstack = stack;

  CHECK_DUTY(p);

  if (auto_sd)
  {
    tell_player(p, "\n Autoshutdown is active. Rebooting the talker may cause pfile corruption.\n"
                   " You are advised to perform a proper shutdown to avoid problems.\n");
    return;
  }

  if (!strcasecmp(str, "-f"))
  {
    tell_player(p, " Forcing reboot ...\n");
    LOGF("reboot", "%s forces a reboot ...", p->name);
    do_reboot();
  }

  if (awaiting_reboot && strcasecmp(str, "-g"))
  {
    tell_player(p, " Already awaiting opportunity to reboot ...\n");
    return;
  }

  for (scan = flatlist_start; scan; scan = scan->flat_next)
    if (scan->location && (scan->flags & IN_EDITOR || scan->mode & (MAILEDIT | ROOMEDIT | NEWSEDIT)))
    {
      awaiting_reboot = 1;
      stack += sprintf(stack, "%s  ", scan->name);
    }

  if (!awaiting_reboot)
  {
    LOGF("reboot", "%s calls for a reboot ... and gets it.", p->name);
    do_reboot();
  }
  stack = end_string(stack);

  TELLPLAYER(p,
	     " Someone is currently in the editor (or in a mode).\n"
	     " The code will reboot when they have finished ...\n"
	     " (ppl holding up the show are: %s)\n", oldstack);

  stack = oldstack;

  SW_BUT(p, " -=*> %s requests a reboot of %s\n",
	 p->name, get_config_msg("talker_name"));
  LOGF("reboot", "%s calls for a reboot ... but has to wait.", p->name);

}



/**                                                                        ** 
        these are for the other side of a reboot, that is the child
        awakening and loading in (if needed) the reboot info 
 **                                                                        **/


   /* a coupla specilized functions as its easier to spoon them and include
      here than try to including modification procs */

void trans_to_quiet(player * p, char *str)
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
      log("error", "Bad Location list 2");
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
}

player *create_player_template(player * t)
{
  player *p;

  p = (player *) MALLOC(sizeof(player));
  memcpy(p, t, sizeof(player));

  /* reset some parseing varibles */
  memset(p->ibuffer, 0, IBUFFER_LENGTH);
  p->column = p->ibuff_pointer = 0;

  /* just in case, NULL all possible pointers */
  p->hash_next = p->flat_next = p->flat_previous =
    p->room_next = 0;
  p->saved = 0;
  p->location = 0;
  p->edit_info = 0;
  p->input_to_fn = 0;
  p->timer_fn = 0;
  p->ttt_opponent = 0;
  p->gag_top = 0;
  p->command_used = 0;
  p->social = 0;


  if (flatlist_start)
    flatlist_start->flat_previous = p;
  p->flat_next = flatlist_start;
  flatlist_start = p;

  p->hash_next = hashlist[0];
  hashlist[0] = p;
  p->hash_top = 0;
  p->timer_fn = 0;
  p->timer_count = -1;
  p->edit_info = 0;

  return p;
}

int retrieve_sysinfo(void)
{
  talker_system_info tsi;
  FILE *f;


  memset(&tsi, 0, sizeof(talker_system_info));

  f = fopen(TALKER_SYSINFO_FILE, "r");
  if (!f)
  {
    LOGF("error", "Failed to open reboot system info file for read [%s]",
	 strerror(errno));
    return -1;
  }
  fread((void *) &tsi, sizeof(talker_system_info), 1, f);
  fclose(f);

  main_descriptor = tsi.main_fd;
  alive_descriptor = tsi.angel_fd;
  up_date = tsi.up_date;
  logins = tsi.logins;
  sys_flags = tsi.sys_flags;
  session_reset = tsi.session_reset;
  strncpy(session, tsi.session, MAX_SESSION - 1);
  strncpy(sess_name, tsi.sess_name, MAX_NAME - 1);
  strncpy(rebooter, tsi.rebooter, MAX_NAME - 1);

  return 0;
}



void reattach_player(player * info)
{
  player *p, *previous, *scan;
  saved_player *sp;
  int hash;

  p = create_player_template(info);
  load_player(p);

  /* based on code from plists.c : link_to_program() */
  previous = 0;
  scan = hashlist[0];
  while (scan && scan != p)
  {
    previous = scan;
    scan = scan->hash_next;
  }
  if (!scan)
    log("error", "Bad non-name hash list");
  else if (!previous)
    hashlist[0] = p->hash_next;
  else
    previous->hash_next = p->hash_next;


  hash = (int) (p->lower_name[0]) - (int) 'a' + 1;
  p->hash_next = hashlist[hash];
  hashlist[hash] = p;
  p->hash_top = hash;

  p->flags |= PROMPT;
  p->timer_fn = 0;
  p->timer_count = -1;


  if (p->residency != NON_RESIDENT)
  {
    p->logged_in = 1;
    sp = p->saved;
  }
  current_players++;

  trans_to_quiet(p, p->location_string);
  if (!(p->location))
    trans_to_quiet(p, sys_room_id(ENTRANCE_ROOM));

  if (p->saved)
  {
    decompress_list(p->saved);
    decompress_alias(p->saved);
    decompress_item(p->saved);
    p->system_flags = p->saved->system_flags;
    p->tag_flags = p->saved->tag_flags;
    p->custom_flags = p->saved->custom_flags;
    p->misc_flags = p->saved->misc_flags;
    if (!(p->pennies))
      p->pennies = p->saved->pennies;
  }

}

void retrieve_players(void)
{
  FILE *f, *pf;
  char namein[160];
  char *oldstack = stack;
  int ret;
  player spanky;

  f = fopen(PLAYER_LIST_FILE, "r");
  if (!f)
  {
    log("error", "When rebooting, in retrieve_players, fopen failed.");
    return;
  }
  memset(namein, 0, 160);
  ret = fscanf(f, "%s", namein);
  while (ret && ret != EOF)
  {
    stack += sprintf(stack, "%s/%s", REBOOTING_DIR, namein);
    stack = end_string(stack);
    pf = fopen(oldstack, "r");
    stack = oldstack;

    if (!pf)
      LOGF("error", "Failed to open reboot player info read (%s) [%s]",
	   namein, strerror(errno));
    else
    {
      fread((void *) &spanky, sizeof(player), 1, pf);
      fclose(pf);
      reattach_player(&spanky);
    }
    memset(namein, 0, 160);
    ret = fscanf(f, "%s", namein);
  }
}


int possibly_reboot(void)
{
  player *p;
  FILE *f;
  char r[16];
  char reboot_msg[256] = "";	/* Set this to your reboot message (if any) */

  f = fopen(CHILDS_PID_FILE, "r");
  if (!f)
    return 0;			/* no child pid file, fuggit */

  memset(r, 0, 16);
  fgets(r, 15, f);
  if (getpid() != atoi(r))
    return 0;

  SUWALL(" -=*> Starting reboot ...\n");
  log("boot", "Rebooting ...");

  if (reboot_msg[0])
    raw_wall(reboot_msg);

  retrieve_sysinfo();
  retrieve_players();


#ifdef ALLOW_MULTIS
  reboot_load_multis();
#endif

  system("rm -f " REBOOTING_DIR "/*");

  if (sys_flags & PANIC)
  {
    sys_flags &= ~PANIC;	/* no need to panic any more */
    if (rebooter[0])
    {
      p = find_player_absolute_quiet(rebooter);
      if (p)
      {
	SW_BUT(p, " -=*> %s managed to crash the talk server.\n", p->name);
	SW_BUT(p, " -=*> %s rebooted and back on track.\n\n",
	       get_config_msg("talker_name"));
	tell_player(p, "\n\n"
		    LINE
		    "\nThis command has caused a system error to occur.\n"
	"Your connection has been terminated to prevent the code crashing.\n"
		    "Please avoid usage of this command until we get it fixed. Sorry!\n\n"
		    LINE
		    "\n\007\n");
	quit(p, 0);
      }
      else
	SUWALL(" -=*> Just crashed without a current_player.\n"
	       " -=*> %s rebooted and back on track.\n\n",
	       get_config_msg("talker_name"));
    }
  }

  SUWALL(" -=*> Reboot of %s successful.\n", get_config_msg("talker_name"));


  reboot_date = time(0);

  return 1;
}

void reboot_version(void)
{
  sprintf(stack, " -=*> Seamless Reboot v0.5b (by phypor) enabled.\n");
  stack = strchr(stack, 0);
}

#ifdef ALLOW_MULTIS
int reboot_save_multis(void)
{
  multi *mscan = all_multis;
  multiplayer *pscan;
  int multis_total;
  FILE *reboot_multi_file = fopen("junk/rebooting/multi", "w");

  if (!reboot_multi_file)
  {
    log("reboot", "Couldn't open junk/rebooting/multi for writing");
    return 1;
  }

  multis_total = multi_count();

  /* amount of multis */
  fprintf(reboot_multi_file, "%d\n", multis_total);

  while (mscan)
  {
    fprintf(reboot_multi_file, "%d,%d,%d,%d\n", mscan->number,
	    mscan->multi_flags, mscan->multi_idle, players_on_multi(mscan));
    pscan = mscan->players_list;
    while (pscan)
    {
      if (pscan->the_player)
	fprintf(reboot_multi_file, "%s\n", pscan->the_player->lower_name);
      else
	fprintf(reboot_multi_file, "%s\n", "<unknown>");

      pscan = pscan->next_player;
    }

    mscan = mscan->next_multi;
  }

  /* actually save it */
  fflush(reboot_multi_file);
  fclose(reboot_multi_file);

  return 0;
}

void reboot_load_multis(void)
{
  multi *new = NULL, *old = NULL;
  multiplayer *newp = NULL, *oldp = NULL;
  char *currpos, *nextpos;
  player *p;
  int num_multis = 0, m_number = 0, m_flags = 0, m_players = 0;
  int m_idle = 0;
  char m_playername[MAX_NAME];
  file reboot_multi_file = load_file("junk/rebooting/multi");

  if (!(reboot_multi_file.where))
  {
    log("reboot", "Couldn't open junk/rebooting/multi for reading");
    return;
  }

  /* get first line - amount of multis */
  currpos = reboot_multi_file.where;
  nextpos = strchr(currpos, '\n');
  *nextpos += 0;
  num_multis = atoi(currpos);

  while (num_multis)
  {
    old = new;
    new = (multi *) MALLOC(sizeof(multi));
    if (old)
      old->next_multi = new;
    else
      all_multis = new;

    /* get first line of multi definition */
    currpos = nextpos;
    nextpos = strchr(currpos, ',');
    *nextpos++ = 0;
    m_number = atoi(currpos);
    currpos = nextpos;
    nextpos = strchr(currpos, ',');
    *nextpos++ = 0;
    m_flags = atoi(currpos);
    currpos = nextpos;
    nextpos = strchr(currpos, ',');
    *nextpos++ = 0;
    m_idle = atoi(currpos);
    currpos = nextpos;
    nextpos = strchr(currpos, '\n');
    *nextpos++ = 0;
    m_players = atoi(currpos);

    new->number = m_number;
    new->multi_flags = m_flags;
    new->multi_idle = m_idle;
    new->players_list = NULL;
    new->next_multi = NULL;

    oldp = NULL;
    newp = NULL;

    while (m_players)
    {
      oldp = newp;
      newp = (multiplayer *) MALLOC(sizeof(multiplayer));
      if (oldp)
	oldp->next_player = newp;
      else
	new->players_list = newp;

      /* get player name */
      currpos = nextpos;
      nextpos = strchr(currpos, '\n');
      *nextpos++ = 0;
      strcpy(m_playername, currpos);

      p = find_player_global_quiet(m_playername);
      newp->the_player = p;
      newp->next_player = NULL;
      m_players--;
    }
    num_multis--;
  }
}
#endif /* ALLOW_MULTIS */

#endif /* SEAMLESS_REBOOT */
