/*
 * Playground+ - globals.c
 * All global non player specific variables
 * ---------------------------------------------------------------------------
 */

#define GLOBAL_FILE

#include "include/config.h"
#include "include/player.h"

/* boot thangs */

int up_date;			/* date talker went up */
int reboot_date;		/* how long ago since last reboot */
int logins = 0;			/* number of logins */
int backup = 0;			/* should we do a backup now? */
int backup_time = -1;		/* time of last backup */

/* Intercom stuff */

#if INTERCOM
int intercom_fd = -1, intercom_pid, intercom_port, intercom_last;
room *intercom_room;
#endif

/* sizes */

int max_players, current_players = 0;
int max_ppl_on_so_far = 0;
int in_total = 0, out_total = 0, in_current = 0, out_current = 0, in_average = 0,
  out_average = 0, net_count = 10, in_bps = 0, out_bps = 0, in_pack_total = 0,
  out_pack_total = 0, in_pack_current = 0, out_pack_current = 0, in_pps = 0,
  out_pps = 0, in_pack_average = 0, out_pack_average = 0;


/* One char for splat sites */

int splat1, splat2;
int splat_timeout;
int soft_splat1, soft_splat2, soft_timeout = 0;

/* sessions!  */

char session[MAX_SESSION];	/* session name */
int session_reset = 0;		/* how long before it can be reset */
player *p_sess = 0;		/* player who set the session */
char sess_name[MAX_NAME] = "";	/* name of player who set session */

/* flags */

int sys_flags = 0;
int command_type = 0;
int config_flags = 0;
int sys_color_atm = 8;

/* suh stuff */
char suhistory[SUH_NUM][MAX_SUH_LEN];

/* pointers */

char *action;
char *stack, *stack_start;
player *flatlist_start;
player *hashlist[27];
player *current_player;
room *current_room;
player *stdout_player;
player *otherfriend;

/* Playground+ stuff - longest spod */
char talker_alpha[160];
char talker_ip[40];
int longest_time = 1;		/* longgest log in time */
char longest_player[20] = "No-one";	/* who set it */
#if AUTOSHUTDOWN
int auto_sd = 0;		/* do we want to shutdown and reboot? */
#endif

int awaiting_reboot = 0;	/* are we waiting to reboot */
int pot;
int social_index;

player **pipe_list;
int pipe_matches;

room *entrance_room, *prison, *relaxed, *comfy, *boot_room;

/*
 * lists for use with idle times its here for want of a better place to put it
 */

file idle_string_list[] =
{
  {"has just hit return.\n", 0},
  {"is typing away at the keyboard.\n", 10},
  {"has hesitated slightly.\n", 15},
  {"is thinking about what to type next.\n", 25},
  {"appears to be stuck for words.\n", 40},
  {"ponders thoughtfully about what to say.\n", 60},
  {"stares oblivious into space.\n", 200},
  {"is on the road to idledom.\n", 300},
  {"is off for a cup of coffee?\n", 600},
  {"appears to be doing something else.\n", 900},
  {"is slipping into a coma.\n", 1200},
  {"is hospitalized and is comatose.\n", 1800},
  {"is probably not going to recover\n", 2400},
  {"has had the plug pulled.\n", 3000},
  {"is dead Jim.\n", 3600},
  {"has been six feet under for some time\n", 7200},
  {0, 0}
};

/* reserved login names -- ie. name that people cannot use */

const char *reserved_names[] =
{
   "me",
   "newbies",
   "sus",
   "staff",
   "finger",
   "who",
   ""
};

/* for sites that you don't want logged as a non-connection
   (ie. sites containing talker listings which periodically connect to the
   talker) 

   NOTE: IP address equivilants are NOT necessary here but have been
included
         just in case a nameserver lookup fails

*/

const char *unlogged_sites[] =
{
   "ewtoo.org", "195.153.247.85",
   "realm.progsoc.uts.edu.au", "138.25.7.1",
   ""
};


