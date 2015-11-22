/*
 * Playground+ - glue.c
 * String returns, shutdown code, bootup and some other stuff
 * ---------------------------------------------------------------------------
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <memory.h>
#include <sys/socket.h>

#include "include/config.h"
#include "include/player.h"
#include "include/fix.h"
#include "include/proto.h"

#ifdef ANTICRASH
#include <setjmp.h>
#endif

#define ABS(x) (((x)>0)?(x):-(x))

/* interns */

int active_port = 0;
char active_talker_name[160];
int max_log_size = 5;

char *tens_words[] =
{"", "ten", "twenty", "thirty", "forty", "fifty",
 "sixty", "seventy", "eighty", "ninety"};

char *units_words[] =
{"none", "one", "two", "three", "four", "five",
 "six", "seven", "eight", "nine"};

char *teens[] =
{"ten", "eleven", "twelve", "thirteen", "fourteen",
 "fifteen", "sixteen", "seventeen", "eighteen",
 "nineteen"};


char *months[12] =
{"January", "February", "March", "April", "May",
 "June", "July", "August", "September", "October",
 "November", "December"};

/* these are the directories in which files maybe edited */
char *EditableDirs[] =
{
  "files",
  "files/whois",
/* "doc",    */
  ""
};

char *ConfigFlagSwitches[] =
{
  "",
  "Spam prevention",
  "Command privs alteration",
  "Swear filtering",
  "Finger from login",
  "Walling of proposals",
  "Walling of marriages",
  "Walling of rejections",
  "Walling of divorces",
  "Walling of accepts",
  "Vegas style slots",
  "Idling is bad",
  "Admin idle exemption",
  "Force command",
  "Multi removal infroming",
  "Newbies closed automatically",
  "Multi verbosity",
  "Newbie selfres",
  "Truespod time",
  "Reconnection look",
  "Online file editing",
  "Examine inform",
  "Welcoming email",
  "All capped names",
  "Sus can recap others",
  ""
};

char shutdown_reason[256] = "";

   /* formally handled in plists.c */
file motd_msg, connect_msg, newban_msg, banned_msg, nonewbies_msg, newbie_msg,
  newpage1_msg, newpage2_msg, disclaimer_msg, splat_msg, sumotd_msg, version_msg,
  hitells_msg, fingerpaint_msg, connect2_msg, connect3_msg, noressies_msg,
  quit_msg, nuke_msg, sneeze_msg, rude_msg;

   /* formally handled in socket.c */
file banish_file, banish_msg, full_msg, splat_msg;

   /* formally hadnled in softmsgs.c */
file config_msg, frogs_msg, admin_msg, shutdowns_msg, plists_msg, pdefaults_msg,
  session_msg, deflog_msg, rooms_msg;

struct message_file
{
  file *f;
  char *path;
  int update;
}
MessageFileArray[] =
{
  {
    &motd_msg, "files/motd.msg", 0
  }
  ,
  {
    &newban_msg, "files/newban.msg", 0
  }
  ,
  {
    &banned_msg, "files/banned.msg", 0
  }
  ,
  {
    &nonewbies_msg, "files/nonew.msg", 0
  }
  ,
  {
    &newbie_msg, "files/newbie.msg", 0
  }
  ,
  {
    &newpage1_msg, "files/newpage1.msg", 0
  }
  ,
  {
    &newpage2_msg, "files/newpage2.msg", 0
  }
  ,
  {
    &disclaimer_msg, "files/disclaimer.msg", 0
  }
  ,
  {
    &splat_msg, "files/splat.msg", 0
  }
  ,
  {
    &sumotd_msg, "files/sumotd.msg", 0
  }
  ,
  {
    &version_msg, "files/version.msg", 0
  }
  ,
  {
    &hitells_msg, "files/hitells.msg", 0
  }
  ,
  {
    &fingerpaint_msg, "files/color_test.msg", 0
  }
  ,
  {
    &connect_msg, "files/connect.msg", 0
  }
  ,
  {
    &connect2_msg, "files/connect2.msg", 0
  }
  ,
  {
    &connect3_msg, "files/connect3.msg", 0
  }
  ,
  {
    &noressies_msg, "files/nores.msg", 0
  }
  ,
  {
    &quit_msg, "files/quit.msg", 0
  }
  ,
  {
    &nuke_msg, "files/nuke.msg", 0
  }
  ,
  {
    &sneeze_msg, "files/sneeze.msg", 0
  }
  ,
  {
    &banish_file, "files/banish", 0
  }
  ,
  {
    &banish_msg, "files/banish.msg", 0
  }
  ,
  {
    &full_msg, "files/full.msg", 0
  }
  ,
  {
    &splat_msg, "files/splat.msg", 0
  }
  ,
  {
    &rude_msg, "files/rude.msg", 0
  }
  ,

  {
    &config_msg, "soft/config.msg", 0
  }
  ,
  {
    &frogs_msg, "soft/frogs.msg", 0
  }
  ,
  {
    &admin_msg, "soft/admin.msg", 0
  }
  ,
  {
    &shutdowns_msg, "soft/shutdowns.msg", 0
  }
  ,
  {
    &plists_msg, "soft/plists.msg", 0
  }
  ,
  {
    &pdefaults_msg, "soft/pdefaults.msg", 0
  }
  ,
  {
    &session_msg, "soft/session.msg", 0
  }
  ,
  {
    &deflog_msg, "soft/deflog.msg", 0
  }
  ,
  {
    &rooms_msg, "soft/rooms.msg", 0
  }
  ,

  {
    0, "", 0
  }
  ,
};

#ifdef ANTICRASH
#include "anti-crash.c"
#endif


/* print up birthday */

char *birthday_string(time_t bday)
{
  static char bday_string[50];
  struct tm *t;
  t = localtime(&bday);
  if ((t->tm_mday) > 10 && (t->tm_mday) < 20)
    sprintf(bday_string, "%dth of %s", t->tm_mday, months[t->tm_mon]);
  else
    switch ((t->tm_mday) % 10)
    {
      case 1:
	sprintf(bday_string, "%dst of %s", t->tm_mday, months[t->tm_mon]);
	break;
      case 2:
	sprintf(bday_string, "%dnd of %s", t->tm_mday, months[t->tm_mon]);
	break;
      case 3:
	sprintf(bday_string, "%drd of %s", t->tm_mday, months[t->tm_mon]);
	break;
      default:
	sprintf(bday_string, "%dth of %s", t->tm_mday, months[t->tm_mon]);
	break;
    }
  return bday_string;
}

/* return a string of the system time */

char *sys_time()
{
  time_t t;
  static char time_string[25];
  t = time(0);

  if (config_flags & cfUSUK)
    strftime(time_string, 25, "%H:%M:%S - %m/%d/%y", localtime(&t));
  else
    strftime(time_string, 25, "%H:%M:%S - %d/%m/%y", localtime(&t));

  return time_string;
}

/* returns converted user time */

char *convert_time(time_t t)
{
  static char time_string[50];
  strftime(time_string, 49, "%H:%M:%S - %a, %B %d", localtime(&t));
  return time_string;
}

/* get local time for all those americans :) */

char *time_diff(int diff)
{
  time_t t;
  static char time_string[50];

  t = time(0) + 3600 * diff;
  strftime(time_string, 49, "%H:%M:%S - %a, %B %d", localtime(&t));
  return time_string;
}

char *time_diff_sec(time_t last_on, int diff)
{
  static char time_string[50];
  time_t sec_diff;

  sec_diff = (3600 * diff) + last_on;
  strftime(time_string, 49, "%H.%M:%S - %a, %B %d", localtime(&sec_diff));
  return time_string;
}

/* Timeprompt code by Nightwolf */

char *sys_time_prompt(int diff)
{
  time_t t;
  static char time_string[12];

  t = time(0) + 3600 * diff;
  strftime(time_string, 12, "%H:%M:%S", localtime(&t));
  return time_string;
}

/* If the requester is lower than LOWER_ADMIN or the remote user has a connection
   refused then return just the addy.

   If the requester is LOWER_ADMIN or above and ident is installed then return
   the user name.

   Otherwise just return the addy.

   Robots get their address returned and nothing else. If Q is NULL then we
   ignore a persons level.

   P = person being looked up, Q = requester of information
 */

char *get_address(player * p, player * q)
{
  static char addy[200], *retstr;
  int can_get_addy = 0;
  retstr = addy;

  if (q == NULL)
    can_get_addy = 1;
  else if (q->residency & LOWER_ADMIN)
    can_get_addy = 1;

#ifndef IDENT
  if (can_get_addy)
    return (p->inet_addr);

  return 0;
}
#else

#ifdef ROBOTS
  if (!can_get_addy || !strcasecmp(p->remote_user, "<CONN REFUSED>") || p->residency & ROBOT_PRIV)
#else
  if (!can_get_addy || !strcasecmp(p->remote_user, "<CONN REFUSED>"))
#endif
  {
    sprintf(retstr, "%s", p->inet_addr);
    while (*retstr)
      retstr++;
  }
  else
  {
    sprintf(retstr, "%s@%s", p->remote_user, p->inet_addr);
    while (*retstr)
      retstr++;
  }
  *retstr++ = 0;

  return addy;
}
#endif

/* converts time into words */

char *word_time(int t)
{
  static char time_string[100], *fill;
  int neg = 0, days, hrs, mins, secs;
  if (!t)
    return "no time at all";

  if (t < 0)
  {
    neg = 1;
    t = 0 - t;
  }
  days = t / 86400;
  hrs = (t / 3600) % 24;
  mins = (t / 60) % 60;
  secs = t % 60;
  fill = time_string;
  if (neg)
  {
    sprintf(fill, "negative ");
    while (*fill)
      fill++;
  }
  if (days)
  {
    sprintf(fill, "%d day", days);
    while (*fill)
      fill++;
    if (days != 1)
      *fill++ = 's';
    if (hrs || mins || secs)
    {
      *fill++ = ',';
      *fill++ = ' ';
    }
  }
  if (hrs)
  {
    sprintf(fill, "%d hour", hrs);
    while (*fill)
      fill++;
    if (hrs != 1)
      *fill++ = 's';
    if (mins && secs)
    {
      *fill++ = ',';
      *fill++ = ' ';
    }
    if ((mins && !secs) || (!mins && secs))
    {
      strcpy(fill, " and ");
      while (*fill)
	fill++;
    }
  }
  if (mins)
  {
    sprintf(fill, "%d min", mins);
    while (*fill)
      fill++;
    if (mins != 1)
      *fill++ = 's';
    if (secs)
    {
      strcpy(fill, " and ");
      while (*fill)
	fill++;
    }
  }
  if (secs)
  {
    sprintf(fill, "%d sec", secs);
    while (*fill)
      fill++;
    if (secs != 1)
      *fill++ = 's';
  }
  *fill++ = 0;
  return time_string;
}

/* returns the time, in greatest time measurement */

char *significant_time(int t)
{
  static char time_string[100];
  int days, hrs, mins, secs;

  if (!t || t < 0)
    return "no time at all";

  days = t / 86400;
  hrs = (t / 3600) % 24;
  mins = (t / 60) % 60;
  secs = t % 60;

  if (days)
  {
    if (days > 1)
      sprintf(time_string, "%d days", days);
    else
      strcpy(time_string, "1 day");
  }
  else if (hrs)
  {
    if (hrs > 1)
      sprintf(time_string, "%d hours", hrs);
    else
      strcpy(time_string, "1 hour");
  }
  else if (mins)
  {
    if (mins > 1)
      sprintf(time_string, "%d mins", mins);
    else
      strcpy(time_string, "1 min");
  }
  else
  {
    if (secs > 1)
      sprintf(time_string, "%d secs", secs);
    else
      strcpy(time_string, "1 sec");
  }
  return time_string;
}

/* returns a number in words */

char *number2string(int n)
{
  int negative = 0, hundreds, tens, units;
  static char words[50];
  char *fill;
  if (n >= 1000)
  {
    sprintf(words, "%d", n);
    return words;
  }
  if (!n)
    return "none";
  if (n < 0)
  {
    n = 0 - n;
    negative = 1;
  }
  hundreds = n / 100;
  tens = (n / 10) % 10;
  units = n % 10;
  fill = words;
  if (negative)
  {
    sprintf(fill, "negative ");
    while (*fill)
      fill++;
  }
  if (hundreds)
  {
    sprintf(fill, "%s hundred", units_words[hundreds]);
    while (*fill)
      fill++;
  }
  if (hundreds && (units || tens))
  {
    strcpy(fill, " and ");
    while (*fill)
      fill++;
  }
  if (tens && tens != 1)
  {
    strcpy(fill, tens_words[tens]);
    while (*fill)
      fill++;
  }
  if (tens != 1 && tens && units)
    *fill++ = ' ';
  if (units && tens != 1)
  {
    strcpy(fill, units_words[units]);
    while (*fill)
      fill++;
  }
  if (tens == 1)
  {
    strcpy(fill, teens[(n % 100) - 10]);
    while (*fill)
      fill++;
  }
  *fill++ = 0;
  return words;
}

/* point to after a string */

char *end_string(char *str)
{
  str = strchr(str, 0);
  str++;
  return str;
}

/* get gender string function */

char *get_gender_string(player * p)
{
  switch (p->gender)
  {
    case MALE:
      return "him";
      break;
    case FEMALE:
      return "her";
      break;
    case OTHER:
      return "it";
      break;
    case VOID_GENDER:
      return "it";
      break;
  }
  return "(this is frogged)";
}

/* get gender string for possessives */

char *gstring_possessive(player * p)
{
  switch (p->gender)
  {
    case MALE:
      return "his";
      break;
    case FEMALE:
      return "her";
      break;
    case OTHER:
      return "its";
      break;
    case VOID_GENDER:
      return "its";
      break;
  }
  return "(this is frogged)";
}


/* more gender strings */

char *gstring(player * p)
{
  switch (p->gender)
  {
    case MALE:
      return "he";
      break;
    case FEMALE:
      return "she";
      break;
    case OTHER:
      return "it";
      break;
    case VOID_GENDER:
      return "it";
      break;
  }
  return "(this is frogged)";
}

char *gender_raw(player * p)
{
  switch (p->gender)
  {
    case MALE:
      return "male";
      break;
    case FEMALE:
      return "female";
      break;
    case OTHER:
      return "thing";
      break;
    case VOID_GENDER:
      return "thing";
      break;
  }
  return "(this is frogged)";
}


/* returns the 'full' name of someone, that is their pretitle and name */

char *full_name(player * p)
{
  static char fname[MAX_PRETITLE + MAX_NAME];
  if ((!(sys_flags & NO_PRETITLES)) && (p->residency & BASE) && p->pretitle[0])
  {
    sprintf(fname, "%s %s", p->pretitle, p->name);
    return fname;
  }
  return p->name;
}


void inform_config_flag_changes(int old, int new)
{
  int i;

  for (i = 1; ConfigFlagSwitches[i][0]; i++)
  {
    if ((old & (1 << i)) != (new & (1 << i)))
    {
      if (new & (1 << i))
	SUWALL(" -=*> %s switched ON\n", ConfigFlagSwitches[i]);
      else
	SUWALL(" -=*> %s switched OFF\n", ConfigFlagSwitches[i]);
    }
  }
}

void set_config_flag(char *type, int bit)
{
  char *ptr;

  ptr = get_config_msg(type);
  if (!strcasecmp(ptr, "on") || !strcasecmp(ptr, "yes"))
    config_flags |= bit;
  else
    config_flags &= ~bit;
}


void set_config_flags(void)
{
  int oldcf = config_flags;

  if (!strcasecmp(get_config_msg("time_format"), "us"))
    config_flags |= cfUSUK;
  else
    config_flags &= ~cfUSUK;

  if (!strcasecmp(get_config_msg("slot_style"), "vegas"))
    config_flags |= cfVEGASLOTS;
  else
    config_flags &= ~cfVEGASLOTS;

  set_config_flag("nospam", cfNOSPAM);
  set_config_flag("privs_change", cfPRIVCHANGE);
  set_config_flag("swear_filter", cfNOSWEAR);
  set_config_flag("login_finger", cfFINGERLOGIN);
  set_config_flag("wall_propose", cfWALLPROPOSE);
  set_config_flag("wall_marriage", cfWALLMARRIGE);
  set_config_flag("wall_reject", cfWALLREJECT);
  set_config_flag("wall_accept", cfWALLACCEPT);
  set_config_flag("wall_divorce", cfWALLDIVORCE);
  set_config_flag("idle_bad", cfIDLEBAD);
  set_config_flag("admin_ok_idle", cfADMINIDLE);
  set_config_flag("can_force", cfFORCEABLE);
  set_config_flag("multi_inform", cfMULTI_INFORM);
  set_config_flag("no_sus_no_new", cfNOSUNONEW);
  set_config_flag("verbose_multis", cfMULTIVERBOSE);
  set_config_flag("use_truespod", cfUSETRUESPOD);
  set_config_flag("allow_selfres", cfSELFRES);
  set_config_flag("reconnect_look", cfRECONNECTLOOK);
  set_config_flag("can_edit_files", cfCANEDITFILES);
  set_config_flag("show_when_xd", cfSHOWXED);
  set_config_flag("do_email_check", cfWELCOMEMAIL);
  set_config_flag("capped_names", cfCAPPEDNAMES);
  set_config_flag("sus_can_recap", cfSUSCANRECAP);


  set_talker_addy();
  strncpy(talker_name, get_config_msg("talker_name"), 79);
  strncpy(talker_email, get_config_msg("talker_email"), 79);
  backup_hour = atoi(get_config_msg("backup_hour"));
  social_index = atoi(get_config_msg("social_index"));
  max_log_size = atoi(get_config_msg("max_log_size"));
  if (max_log_size <= 0)	/* just in case */
    max_log_size = 5;

  if (oldcf)
    inform_config_flag_changes(oldcf, config_flags);
}



/* lil func to inform of big changes done in config.msg 
   (these only get done on a reload of config.msg, not on boot 
 */
void update_configs(void)
{
  int tp = atoi(get_config_msg("port"));
  char *atn;

  if (active_port != tp)	/* this aint no lil operation here */
  {
    SUWALL(" -=*> Port changing from %d to %d ...\n", active_port, tp);
    close_down_socket();
    init_socket(tp);
    active_port = tp;
#ifdef IDENT
    kill_ident_server();
    init_ident_server();
#endif /* IDENT */
#ifdef INTERCOM			/* im not up on intercom enough to know if this will actually
				   work or not....but if there is any chance of it, this
				   method is the mostly likely to sucseed id imagine */
    intercom_port = active_port - 1;
    kill_intercom();
    start_intercom();
#endif /* INTERCOM */
  }

  atn = get_config_msg("talker_name");
  if (strcmp(active_talker_name, atn))
  {
    SUWALL(" -=*> Talker name changed from '%s' to '%s'\n",
	   active_talker_name, atn);
    strncpy(active_talker_name, atn, 159);
  }

  set_config_flags();

}

void init_messages(void)
{
  int i;
  struct stat sbuf;
  file d;

  for (i = 0; MessageFileArray[i].path[0]; i++)
  {
    memset(&d, 0, sizeof(file));
    memset(&sbuf, 0, sizeof(struct stat));

    if (stat(MessageFileArray[i].path, &sbuf) < 0)
    {
      if (!strncmp(MessageFileArray[i].path, "soft/", 5))
      {
	LOGF("error", "\n"
	     " Omigawd !!!   Soft message file missing '%s'\n\n"
	   " We really can't boot without this file so I'm gonna go ahead\n"
	     " and exit... if you don't know where this file went and dont\n"
	     " have any backups, you'll need to get it from a PG+ dist\n\n",
	     MessageFileArray[i].path);
	exit(2);
      }

      LOGF("error", "Missing file '%s'!", MessageFileArray[i].path);
      continue;
    }

    if (!(MessageFileArray[i].update))	/* inits at boot */
    {
      if (MessageFileArray[i].f->where)		/* somehow */
	FREE(MessageFileArray[i].f->where);
      d = load_file(MessageFileArray[i].path);
      MessageFileArray[i].f->where = d.where;
      MessageFileArray[i].f->length = d.length;
      MessageFileArray[i].update = sbuf.st_mtime;
      continue;
    }

    /* it it was modified later than we have set */
    if (sbuf.st_mtime > MessageFileArray[i].update)
    {
      if (MessageFileArray[i].f->where)
	FREE(MessageFileArray[i].f->where);
      d = load_file(MessageFileArray[i].path);
      MessageFileArray[i].f->where = d.where;
      MessageFileArray[i].f->length = d.length;
      MessageFileArray[i].update = sbuf.st_mtime;
      SUWALL(" -=*> Reloaded '%s' ...\n", MessageFileArray[i].path);
      if (!strcasecmp(MessageFileArray[i].path, "soft/config.msg"))
	update_configs();
    }
  }
}


void log(char *filename, char *string)
{
  int fd;
  int length;
  char *oldstack = stack;
  char path[160];
  struct stat sbuf;

  SEND_TO_DEBUG("%s log: %s", filename, string);

  memset(path, 0, 160);
  sprintf(path, "logs/%s.log", filename);

  if (stat(path, &sbuf) < 0)	/* file no exists */
  {
    fd = creat(path, (S_IRUSR | S_IWUSR));
    length = priv_for_log_str(get_deflog_msg(filename));
    if (length > 0)
      fchmod(fd, S_IWUSR | length);
    length = 0;
  }
  else
  {
#ifdef BSDISH
    /* *BSD doesn't support O_SYNC?? */
    fd = open(path, (O_RDWR | O_CREAT | O_APPEND), (S_IRUSR | S_IWUSR));
#else
    fd = open(path, (O_RDWR | O_CREAT | O_SYNC | O_APPEND),
	      (S_IRUSR | S_IWUSR));
#endif
    length = sbuf.st_size;
  }

  /* Prevent bug and idea logs from being clipped */
  if (strcasecmp(string, "bug") && strcasecmp(string, "idea"))
    if ((length + strlen(string)) > (max_log_size * 1024))
    {
      lseek(fd, 0, SEEK_SET);	/* put file ptr back to beginning of file */
      read(fd, stack, length);	/* read it into stack */
      close(fd);		/* close it */
      fd = open(path, (O_WRONLY | O_CREAT | O_TRUNC),
		(S_IRUSR | S_IWUSR));
      /* re-open, truncating it */

      stack[length] = '\0';	/* make sure stack is NULL terminated */
      stack += strlen(string);	/* advance stack ptr length of string */

      while (*stack && *stack != '\n')
	stack++;		/* advance stack to next newline */

      if (*stack)
      {
	stack++;		/* advance past newline */
	write(fd, stack, strlen(stack));
      }
      stack = oldstack;
    }
  sprintf(stack, "%s - %s\n", sys_time(), string);
  if (!(sys_flags & NO_PRINT_LOG))
    printf(stack);
  write(fd, stack, strlen(stack));
  close(fd);

}



/* send things to a file (used for spodlist and mailing code) --Silver */

void sendtofile(char *f, char *string)
{
  FILE *fp;
  char *fname;

  fname = malloc(strlen(string)+15);
  if (!fname)
  {
    LOGF("error", "no memory to malloc in sendtofile");
    return;
  }

  sprintf(fname, "reports/%s.rpt", f);

  fp = fopen(fname, "a");
  if (!fp)
  {
    LOGF("error", "can't send text to '%s' in sendtofile", fname);
    free(fname);   
    return;
  }
 
  fprintf(fp, "%s\n", string);
  fclose(fp);
  free(fname);
}

void banlog(char *f, char *string)
{
  int fd, length;

  sprintf(stack, "files/%s", f);

#ifdef BSDISH
  fd = open(stack, O_CREAT | O_WRONLY | S_IRUSR | S_IWUSR);
#else
  fd = open(stack, O_CREAT | O_WRONLY | O_SYNC, S_IRUSR | S_IWUSR);
#endif

  length = lseek(fd, 0, SEEK_END);
  sprintf(stack, "%s\n", string);
  if (!(sys_flags & NO_PRINT_LOG))
    printf(stack);
  write(fd, stack, strlen(stack));
  close(fd);
}

/* what happens when *shriek* an error occurs */

void handle_error(char *error_msg)
{
  if (sys_flags & PANIC)
  {
    stack = stack_start;
    log("error", "Immediate PANIC shutdown.");
    exit(-1);
  }
  sys_flags |= PANIC;

  stack = stack_start;

  if (sys_flags & UPDATE)
    sys_flags &= ~NO_PRINT_LOG;

  log("error", error_msg);
  log("boot", "Abnormal exit from error handler");

  /* dump possible useful info */

  log("dump", "------------ Starting dump");

  sprintf(stack_start, "Errno set to %d, %s", errno, sys_errlist[errno]);
  stack = end_string(stack_start);
  log("dump", stack_start);

  if (current_player)
  {
    log("dump", current_player->name);
    if (current_player->location)
    {
      sprintf(stack_start, "player %s.%s",
	      current_player->location->owner->lower_name,
	      current_player->location->id);
      stack = end_string(stack_start);
      log("dump", stack_start);
    }
    else
      log("dump", "No room of current player");

    sprintf(stack_start, "flags %d sys %d tag %d cus %d mis %d res %d",
	    current_player->flags, current_player->system_flags,
	    current_player->tag_flags, current_player->custom_flags,
	    current_player->misc_flags, current_player->residency);
    stack = end_string(stack_start);
    log("dump", stack_start);
    log("dump", current_player->ibuffer);
  }
  else
    log("dump", "No current player !");
  if (current_room)
  {
    sprintf(stack_start, "current %s.%s", current_room->owner->lower_name,
	    current_room->id);
    stack = end_string(stack_start);
    log("dump", stack_start);
  }
  else
    log("dump", "No current room");

  sprintf(stack_start, "global flags %d, players %d", sys_flags, current_players);
  stack = end_string(stack_start);
  log("dump", stack_start);

  sprintf(stack_start, "action %s", action);
  stack = end_string(stack_start);
  log("dump", stack_start);

  log("dump", "---------- End of dump info");

#ifndef SEAMLESS_REBOOT
  raw_wall("\n\n"
	   "      -=> *WIBBLE* Something bad has happened. Trying to save files <=-\007\n\n\n");
  close_down();
  exit(-1);
#else
  su_wall("\n\n");
  su_wall(" -=*> *WIBBLE* Something bad has happened!\n\007");
  if (auto_sd)  /* don't reboot if autoshutdown is on */
  {
    su_wall(" -=*> Autoshutdown active! Reboot may corrupt pfiles.\n");
    su_wall(" -=*> Starting immediate panic shutdown ...\n");
    raw_wall("\n\n"
             "      -=> *WIBBLE* Something bad has happened. Trying to save files <=-\007\n\n\n");
    close_down();
    exit(-1);
  }
  do_reboot();
#endif
}


/* function to convert seamlessly to caps (ish) */

char *caps(char *str)
{
  static char buff[500];
  strncpy(buff, str, 498);
  buff[0] = toupper(buff[0]);
  return buff;
}


/* load a file into memory */

file load_file_verbose(char *filename, int verbose)
{
  file f =
  {0, 0};
  int d;
  char *oldstack;

  oldstack = stack;

  d = open(filename, O_RDONLY);
  if (d < 0)
  {
    sprintf(oldstack, "Can't find file:%s", filename);
    stack = end_string(oldstack);
    if (verbose)
      log("error", oldstack);
    f.where = (char *) MALLOC(1);
    *(char *) f.where = 0;
    f.length = 0;
    stack = oldstack;
    return f;
  }

  f.length = lseek(d, 0, SEEK_END);

  if (f.length < 0)
  {
    LOGF("error", "lseek, load_file_verbose() for %s, errno %d, %s",
	 filename, errno, strerror(errno));
    f.where = (char *) MALLOC(1);
    *(char *) f.where = 0;
    f.length = 0;
    return f;
  }
  lseek(d, 0, SEEK_SET);
  f.where = (char *) MALLOC(f.length + 1);
  memset(f.where, 0, f.length + 1);
  if (read(d, f.where, f.length) < 0)
  {
    sprintf(oldstack, "Error reading file:%s", filename);
    stack = end_string(oldstack);
    log("error", oldstack);
    f.where = (char *) MALLOC(1);
    *(char *) f.where = 0;
    f.length = 0;
    stack = oldstack;
    return f;
  }
  close(d);
  if (sys_flags & VERBOSE)
  {
    sprintf(oldstack, "Loaded file:%s", filename);
    stack = end_string(oldstack);
    log("boot", oldstack);
    stack = oldstack;
  }
  stack = oldstack;
  *(f.where + f.length) = 0;
  return f;
}

file load_file(char *filename)
{
  return load_file_verbose(filename, 1);
}

/* convert a string to lower case */

void lower_case(char *str)
{
  while (*str)
    *str++ = tolower(*str);
}

/* fns to block signals */

void sigpipe(int c)
{
  if (current_player)
  {
    LOGF("sigpipe", "Closing connection due to sigpipe. [%s]",
	 current_player->name);
    shutdown(current_player->fd, 2);
    close(current_player->fd);
  }
  else
  {
    LOGF("sigpipe", "SIGPIPE recv'd, last action : %s", action);
  }

#if !defined(hpux) && !defined(linux)
  signal(SIGPIPE, sigpipe);
#endif /* !hpux && !linux */
  return;
}
void sighup(int c)
{
  log("boot", "SIGHUP caught ... being snobbish towards it tho...");

#if !defined(hpux) && !defined(linux)
  signal(SIGHUP, sighup);
#endif /* !hpux && !linux */
}

void sigquit(int c)
{
  handle_error("Quit signal received.");
}

void sigill(int c)
{
  handle_error("Illegal instruction.");
}

void sigfpe(int c)
{
  handle_error("Floating Point Error.");
}

void sigbus(int c)
{
  handle_error("Bus Error.");
}

void sigsegv(int c)
{
  handle_error("Segmentation Violation.");
}

#if !defined(linux)
void sigsys(int c)
{
  handle_error("Bad system call.");
}

#endif
void sigterm(int c)
{
  handle_error("Terminate signal received.");
}

void sigxfsz(int c)
{
  handle_error("File descriptor limit exceeded.");
}

void sigusr1(int c)
{
  fork_the_thing_and_sync_the_playerfiles();
#if !defined(hpux) && !defined(linux)
  signal(SIGUSR1, sigusr1);
#endif /* hpux && linux */
}

void sigusr2(int c)
{
  do_backup(0, 0);
}

void sigchld(int c)
{
#if !defined(hpux) && !defined(linux)
  signal(SIGCHLD, sigchld);
#endif /* hpux && linux */
  return;
}

/* close down sequence */

void close_down(void)
{
  player *scan, *old_current;

  struct itimerval new, old;

  raw_wall("\007\n\n");
  command_type |= HIGHLIGHT;
  if (shutdown_count == 0)
  {
    raw_wall(shutdown_reason);
  }
  raw_wall("\n\n\n      ---====>>>> &t shutting down NOW <<<<====---"
	   "\n\n\n");
  command_type &= ~HIGHLIGHT;

  new.it_interval.tv_sec = 0;
  new.it_interval.tv_usec = 0;
  new.it_value.tv_sec = 0;
  new.it_value.tv_usec = new.it_interval.tv_usec;
  if (setitimer(ITIMER_REAL, &new, &old) < 0)
    handle_error("Can't set timer.");
  if (sys_flags & VERBOSE || sys_flags & PANIC)
    log("boot", "Timer Stopped");

  if (sys_flags & VERBOSE || sys_flags & PANIC)
    log("boot", "Saving all players.");
  for (scan = flatlist_start; scan; scan = scan->flat_next)
    save_player(scan);
  if (sys_flags & VERBOSE || sys_flags & PANIC)
    log("boot", "Syncing to disk.");
  sync_all();

  old_current = current_player;
  current_player = 0;
  sync_all_news_headers();
  sync_notes(0);
  sync_sitems(0);
  current_player = old_current;

  if (sys_flags & PANIC)
    raw_wall("\n\n              ---====>>>> Files sunc (phew !) <<<<====---"
	     "\007\n\n\n");

  for (scan = flatlist_start; scan; scan = scan->flat_next)
    close(scan->fd);

#ifdef INTERCOM
  kill_intercom();
#endif

  close_down_socket();
#ifdef ALLOW_MULTIS
  destroy_all_multis();
#endif

  if (!(sys_flags & PANIC))
  {
    unlink("junk/PID");
    log("boot", "Program exited normally.");
    exit(0);
  }
}


/* the boot sequence */

void boot(int port)
{
  char *oldstack = stack;
  int i;
#ifndef PC
  struct rlimit rlp;
  struct itimerval new, old;
#endif
#if defined(hpux) | defined(linux)
  struct sigaction sa;
#endif /* hpux | linux */

  sprintf(stack, "Starting to boot up %s ...", get_config_msg("talker_name"));
  stack = end_string(stack);
  log("boot", oldstack);
  stack = oldstack;

  up_date = time(0);
  reboot_date = time(0);

#ifndef PC
#ifndef ULTRIX

#if !defined(linux)
  getrlimit(RLIMIT_NOFILE, &rlp);
  rlp.rlim_cur = rlp.rlim_max;
  setrlimit(RLIMIT_NOFILE, &rlp);
#endif /* LINUX */
/*
   max_players = (rlp.rlim_cur) - 20;
 */
  max_players = 210;

  if (sys_flags & VERBOSE)
  {
    sprintf(oldstack, "Got %d file descriptors, Allocated %d for players",
	    (int) rlp.rlim_cur, max_players);
    stack = end_string(oldstack);
    log("boot", oldstack);
    stack = oldstack;
  }
#else
  max_players = ULTRIX_PLAYER_LIM;

  if (sys_flags & VERBOSE)
  {
    sprintf(oldstack, "Set max players to %d.", max_players);
    stack = end_string(oldstack);
    log("boot", oldstack);
    stack = oldstack;
  }
#endif /* ULTRIX */

#ifndef SOLARIS
  getrlimit(RLIMIT_RSS, &rlp);
  rlp.rlim_cur = MAX_RES;
  setrlimit(RLIMIT_RSS, &rlp);
#endif

#else
  max_players = 10;
#endif

  flatlist_start = 0;
  for (i = 0; i < 27; i++)
    hashlist[i] = 0;

  stdout_player = (player *) MALLOC(sizeof(player));
  memset(stdout_player, 0, sizeof(player));

  srand(time(0));

  /* Initialise ... */

  init_plist();			/* player lists */
  init_parser();		/* commands */
  init_rooms();			/* all rooms */
  init_notes();			/* player notes */
  init_help();			/* help system */
  init_sitems();		/* items */
  init_news();			/* nunews */
#ifdef ALLOW_MULTIS
  init_multis();		/* multis/chains */
#endif
  init_socials();		/* kRad socials */
  init_global_histories();	/* history logging */
#ifdef ROBOTS
  init_robots();		/* robots code */
#endif
#ifdef IDENT
  init_ident_server();		/* ident server */
#endif
  get_hardware_info();		/* for netstat */

#ifndef PC
  if (!(sys_flags & SHUTDOWN))
  {
    new.it_interval.tv_sec = 0;
    new.it_interval.tv_usec = (1000000 / TIMER_CLICK);
    new.it_value.tv_sec = 0;
    new.it_value.tv_usec = new.it_interval.tv_usec;
#if defined(hpux) | defined(linux)
    sa.sa_handler = actual_timer;
#ifndef REDHAT5
    sa.sa_mask = 0;
#endif
    sa.sa_flags = 0;
    if ((int) sigaction(SIGALRM, &sa, 0) < 0)
#else
    if ((int) signal(SIGALRM, actual_timer) < 0)
#endif /* hpux | linux */
      handle_error("Can't set timer signal.");
    if (setitimer(ITIMER_REAL, &new, &old) < 0)
      handle_error("Can't set timer.");
    if (sys_flags & VERBOSE)
      log("boot", "Timer started.");
  }
#if defined(hpux) | defined(linux)
  sa.sa_handler = sigpipe;

#ifndef REDHAT5
  sa.sa_mask = 0;
#endif
  sa.sa_flags = 0;
  sigaction(SIGPIPE, &sa, 0);
  sa.sa_handler = sighup;
  sigaction(SIGHUP, &sa, 0);
  sa.sa_handler = sigquit;
  sigaction(SIGQUIT, &sa, 0);
  sa.sa_handler = sigill;
  sigaction(SIGILL, &sa, 0);
  sa.sa_handler = sigfpe;
  sigaction(SIGFPE, &sa, 0);
  sa.sa_handler = sigbus;
  sigaction(SIGBUS, &sa, 0);
  sa.sa_handler = sigsegv;
  sigaction(SIGSEGV, &sa, 0);
#if !defined(linux)
  sa.sa_handler = sigsys;
  sigaction(SIGSYS, &sa, 0);
#endif /* linux */

#ifdef REDHAT5
  sigemptyset(&sa.sa_mask);
#else
  sa.sa_mask = 0;
#endif

 sa.sa_handler = SIG_IGN;
 sa.sa_flags = 0;
 sigaction(SIGPIPE, &sa, NULL);
  
#ifdef REDHAT5
  sigemptyset(&sa.sa_mask);
#else
  sa.sa_mask = 0;
#endif

  sa.sa_handler = sigterm;
  sigaction(SIGTERM, &sa, 0);
  sa.sa_handler = sigxfsz;
  sigaction(SIGXFSZ, &sa, 0);
  sa.sa_handler = sigusr1;
  sigaction(SIGUSR1, &sa, 0);
  sa.sa_handler = sigusr2;
  sigaction(SIGUSR2, &sa, 0);
  sa.sa_handler = sigchld;
  sigaction(SIGCHLD, &sa, 0);
#else
  signal(SIGPIPE, sigpipe);
  signal(SIGHUP, sighup);
  signal(SIGQUIT, sigquit);
  signal(SIGILL, sigill);
  signal(SIGFPE, sigfpe);
  signal(SIGBUS, sigbus);
  signal(SIGSEGV, sigsegv);
  signal(SIGSYS, sigsys);
  signal(SIGTERM, sigterm);
  signal(SIGXFSZ, sigxfsz);
  signal(SIGUSR1, sigusr1);
  signal(SIGUSR2, sigusr2);
  signal(SIGCHLD, sigchld);
#endif /* hpux | linux */
#endif /* PC */

#ifdef INTERCOM
  intercom_port = port - 1;
#endif

#ifndef SEAMLESS_REBOOT
  if (!(sys_flags & SHUTDOWN))
  {
#else
  if (!(sys_flags & SHUTDOWN) && !possibly_reboot())
  {
#endif
    init_socket(port);
    alive_connect();
  }
  current_players = 0;

  stack = oldstack;
}


/* Log the Process ID of this process to the file junk/PID */

void log_pid(void)
{
  FILE *f;

  f = fopen("junk/PID", "w");
  if (!f)
  {
    fprintf(stderr, "Log_Pid: Couldn't open junk/PID for writing!!!\n");
    exit(-1);
  }
  fprintf(f, "%d", getpid());
  fflush(f);
  fclose(f);
}


/* got to have a main to control everything */

int main(int argc, char *argv[])
{
  int port = 0;
  char *ptr;

  action = "boot";

  /* lets go head and do this now */
  if (chdir(ROOT))
  {
    printf("Can't change to root directory.\n");
    exit(1);
  }

  /* then get our stack */
  backup = 0;
  stack_start = (char *) MALLOC(STACK_SIZE);
  memset(stack_start, 0, STACK_SIZE);
  stack = stack_start;

  /* now we need some of these right away, so get them in */
  init_messages();
  set_config_flags();
  strncpy(active_talker_name, get_config_msg("talker_name"), 159);


  /* any start up configs here */
  ptr = get_config_msg("screening");
  if (!strcasecmp(ptr, "on") || !strcasecmp(ptr, "yes"))
    sys_flags |= SCREENING_NEWBIES;

  strncpy(session, get_session_msg("default_session"), MAX_SESSION - 3);
  strncpy(sess_name, get_session_msg("default_setter"), MAX_NAME - 1);

  /* are we running in verbose debugging mode? */
#ifdef DEBUG_VERBOSE
  sys_flags |= VERBOSE;
#endif

  if (argc == 3)
  {
    if (!strcasecmp("update", argv[1]))
    {
      if (!strcasecmp("spods", argv[2]))
      {
	log("boot", "Program booted to update spodlists.");
	sys_flags |= SHUTDOWN | UPDATE_SPODLIST;
      }
      else if (!strcasecmp("url", argv[2]))
      {
	log("boot", "Program booted for URL update.");
	sys_flags |= SHUTDOWN | UPDATE_URLS;
      }
      else if (!strcasecmp("data", argv[2]))
      {
	log("boot", "Program booted for data output.");
	sys_flags |= SHUTDOWN | UPDATE_INT_DATA;
      }
      else if (!strcasecmp("rooms", argv[2]))
      {
	log("boot", "Program booted for file rooms update.");
	sys_flags |= SHUTDOWN | UPDATEROOMS;
      }
      else if (!strcasecmp("flags", argv[2]))
      {
	log("boot", "Program booted for flags update");
	sys_flags |= SHUTDOWN | UPDATEFLAGS;
      }
      else
      {
	log("boot", "Program booted for file players update.");
	sys_flags |= SHUTDOWN | UPDATE;
      }
    }
  }
  if (argc == 2)
    port = atoi(argv[1]);

  if (!port)
    port = atoi(get_config_msg("port"));

  if (port < 1024 || port > 32000)
  {
    log("boot", "Check your port configuration, setting to default 7777");
    port = 7777;
  }
  active_port = port;
  boot(port);

#ifdef INTERCOM
  start_intercom();
#endif

  if (sys_flags & UPDATE_URLS)
    do_update(3);
  else if (sys_flags & UPDATE_INT_DATA)
    do_update(4);
  else if (sys_flags & UPDATE_SPODLIST)
    do_update(2);
  else if (sys_flags & UPDATE)
    do_update(0);
  else if (sys_flags & UPDATEFLAGS)
    do_update(0);
  else if (sys_flags & UPDATEROOMS)
    do_update(1);
  sys_flags |= NO_PRINT_LOG;

  log_pid();			/* Log the Process ID of of the talker */

  log("boot", "Ready and listening for connections");	/* Another formatting line ... -grin- */

/* print a warning if running in debugging mode */

#ifdef DEBUGGING
  printf("\n\n -=*> WARNING! YOU ARE CURRENTLY RUNNING IN ");
#ifdef DEBUG_VERBOSE
  printf("VERBOSE ");
#endif
  printf("DEBUGGING MODE!\n"
      " -=*> This mode should not be used in live usage due to code size!\n"
	 " -=*> To use as 'live' You should either undefine USE_LIBG or comment out\n"
    " -=*> 'DEBUGGING = 1' within the Makefile, recompile and reboot.\n\n");
#endif

#ifdef ANTICRASH
  setjmp(recover_jmp_env);
  signal(SIGSEGV, mid_program_error);
  signal(SIGBUS, mid_program_error);
#endif

  while (!(sys_flags & SHUTDOWN))
  {
    errno = 0;

    if (backup)
      do_backup(0, 0);

    if (stack != stack_start)
    {
      sprintf(stack_start, "Lost stack reclaimed %d bytes\n",
	      (int) stack - (int) stack_start);
      stack = end_string(stack_start);
      log("stack", stack_start);
      stack = stack_start;
    }
    action = "scan sockets";
    scan_sockets();
    action = "processing players";
    process_players();
    current_player = (player *) NULL;
    current_room = (room *) NULL;

#ifdef ROBOTS
    action = "processing robots";
    process_robots();
    current_player = (player *) NULL;
    current_room = (room *) NULL;
#endif

    action = "timer_function";
    timer_function();
    sigpause(0);

    action = "do_alive_ping";
    do_alive_ping();

    action = "";

  }

  close_down();
  exit(0);
}

int save_file(file * f, char *filename)
{
  char *oldstack;
  FILE *fp;

  if (NULL == (fp = fopen(filename, "w")))
  {
    return 0;
  }
  if (-1 == fwrite(f->where, f->length, 1, fp))
  {
    oldstack = stack;
    sprintf(stack, "Unable to save %s\n", filename);
    log("error", stack);
    stack = oldstack;
  }
  fclose(fp);
  return 1;
}

void do_backup(player * p, char *str)
{
  char *oldstack = stack;

  backup_time = (int) time(0);
  backup = 0;

  raw_wall("\n"
	   " -=*> Starting Daily Backup Protocol\n"
	   " -=*> The system will resume shortly!\n\n"
	   " -=*> Validating all rooms\n");
  dynamic_validate_rooms((player *) 0, (char *) 0);

  raw_wall(" -=*> Defragmenting rooms\n");
  dynamic_defrag_rooms((player *) 0, (char *) 0);

  raw_wall(" -=*> Handling mail and news\n");
  sync_notes(0);

  raw_wall(" -=*> Syncing objects\n");
  sync_sitems(0);

  raw_wall(" -=*> Saving Residents\n");
  sync_all();

  strcpy(stack, ROOT "bin/backup.code");
  stack = end_string(stack);
  system(oldstack);
  stack = oldstack;

  raw_wall(" -=*> Daily Backups Complete!\n\n");
}


/* you want online editing of files...where here ya go..;)   ~phy */

void end_file_edit(player * p)
{
  FILE *f;
  char *oldstack = stack;
  struct stat sbuf;
  int new_file = 0;

  if (!p->current_file[0] || !p->edit_info)
  {
    tell_player(p, " Now howd you get here ???\n");
    return;
  }

  if (strlen(p->edit_info->buffer) < 2)
  {
    tell_player(p, " Not saving as the file would be blanked...\n"
		" If you wanna delete a file, do it from shell\n");
    return;
  }

  if (stat(p->current_file, &sbuf) < 0)
    new_file++;

  /* lets keep a backup copy, shall we? */
  if (!new_file)
  {
    sprintf(stack, "%s~", p->current_file);
    stack = end_string(stack);
    rename(p->current_file, oldstack);
    stack = oldstack;
  }

  f = fopen(p->current_file, "w");
  if (!f)
  {
    TELLPLAYER(p, " Unable to open the file '%s for writing\n",
	       p->current_file);
    return;
  }
  fputs(p->edit_info->buffer, f);
  fclose(f);
  if (new_file)
  {
    TELLPLAYER(p, " File %s successfully created and saved ...\n", p->current_file);
    SW_BUT(p, " -=*> %s creates %s ...\n", p->name, p->current_file);
    LOGF("edit", "%s creates '%s'", p->name, p->current_file);
  }
  else
  {
    TELLPLAYER(p, " File %s successfully saved ...\n", p->current_file);
    SW_BUT(p, " -=*> %s edits %s ...\n", p->name, p->current_file);
    LOGF("edit", "%s edits '%s'", p->name, p->current_file);
  }
  memset(p->current_file, 0, MAX_DESC);

  /* whee, let auto reload handle the rest */
}

void quit_file_edit(player * p)
{
  TELLPLAYER(p, " Aborted editing of %s ...\n", p->current_file);
  memset(p->current_file, 0, MAX_DESC);
}

void edit_file(player * p, char *str)
{
  struct stat sbuf;
  char *oldstack = stack, *wf;
  file gf;
  int i;

  if (!(config_flags & cfCANEDITFILES))
  {
    tell_player(p, " This command is unimplemented.\n");
    return;
  }

  wf = next_space(str);
  if (!*wf)
  {
    tell_player(p, " Format : edit_file <directory> <file>\n");
    return;
  }
  *wf++ = '\0';

  for (i = 0; EditableDirs[i][0]; i++)
    if (!strcmp(EditableDirs[i], str))
      break;

  if (!EditableDirs[i][0])
  {
    tell_player(p, " Cannot access that directory.\n");
    return;
  }
  if (*wf == '.' || strchr(wf, '/'))
  {
    tell_player(p, " Cannot access another directory... (bunghole)\n");
    return;
  }
  sprintf(stack, "%s/%s", str, wf);
  stack = end_string(stack);

  if (stat(oldstack, &sbuf) < 0)
  {
    /* un comment this out if you want to be able to only edit
       existant files */
/*
   tell_player (p, " You may only edit existant files ...\n");
   stack = oldstack;
   return;
 */

  }
  else if (!(S_ISREG(sbuf.st_mode)))
  {
    TELLPLAYER(p, " The file '%s' is not a regular one,\n"
	       " so there isnt any use trying to edit it\n", oldstack);
    stack = oldstack;
    return;
  }

  gf = load_file_verbose(oldstack, 0);

  if (!gf.where)
  {
    tell_player(p, " Funky error with load_file ?\n");
    return;
  }
  if (!*(gf.where))
    tell_player(p, " File is empty... starting fresh\n");

  start_edit(p, 10240, end_file_edit, quit_file_edit, gf.where);
  FREE(gf.where);
  if (!p->edit_info)
    tell_player(p, " Sorry jack, failed to get into the editor\n");
  else
    strncpy(p->current_file, oldstack, MAX_DESC - 1);
  stack = oldstack;
}


void get_ps(player * p, char *str)
{
  FILE *f;
  char psc[80], red[240], *oldstack = stack, psp[80];
  int my_pid = getpid();

  strncpy(psp, get_config_msg("ps_options"), 79);
  if (!strcasecmp(psp, "off"))
  {
    tell_player(p, " This command is unimplemented.\n");
    return;
  }
  sprintf(psc, "ps %s", psp);
  if (!(f = popen(psc, "r")))
  {
    TELLPLAYER(p, "Couldnt popen ... errno %d, %s\n", errno, strerror(errno));
    return;
  }
  sprintf(psc, " %d ", my_pid);
  while (fgets(red, 239, f))
  {
    if (strstr(red, psc))
      stack += sprintf(stack, "^H%s^N", red);
    else
      stack += sprintf(stack, "%s", red);
  }
  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
  pclose(f);
}

/* for checking for a reserved name */

int reserved_name (char *name)
{
    int i;
    
   for (i = 0; reserved_names[i][0]; i++)
      if (!strcasecmp (reserved_names[i], name))
         return 1;
   return 0;
}

/* for checking for a site that shouldn't be logged */

int unlogged_site(char *site)
{
   int i;
   
   for (i = 0; unlogged_sites[i][0]; i++)
      if (!strcasecmp (unlogged_sites[i], site))
         return 1;
   return 0;
}

