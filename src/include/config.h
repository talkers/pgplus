/* 
 * Playground+ - config.h
 * Holds configuration defines
 * -----------------------------------------------------------------------
 */

/* Include the ROOT define - set up automatically everytime 'make' is called */

#include "root.h"

/* Include all the autoconfig stuff (from "make config") */

#include "autoconfig.h"

/* the system equivalent of the timelocal command */

#ifndef OPENBSD
   #if !defined(linux)
      #define TIMELOCAL(x) timelocal(x)
   #else
      #define TIMELOCAL(x) mktime(x)
   #endif 
#else
      #define TIMELOCAL(x) mktime(x)
#endif


/* this for ULTRIX and Solaris */

#ifdef ULTRIX 
 #define TIMELOCAL(x) mktime(x)
#endif 
#ifdef SOLARIS
 #define TIMELOCAL(x) mktime(x)
#endif 

/* Timings */

#define ONE_SECOND 1	/* how long a second is */
#define ONE_MINUTE 60	/* how long a minute is */
#define ONE_HOUR 3600 /* ... and an hour */
#define ONE_DAY 86400	/* ... and a day */
#define ONE_WEEK 604800	/* ... and a week */
#define ONE_MONTH 2419200 /* this is really 28 days */
#define ONE_YEAR 31536000 /* one year */

/* which malloc routines to use */

#define MALLOC(x) malloc(x)
#define FREE(x) free(x)

/* These defines shouldn't need to be modified - however if you do be
   _very_ careful! */

#define SOCKET_PATH "junk/alive_socket"
#define STACK_SIZE 500001	/* how big the stack can be */
#define HASH_SIZE 64			
#define NOTE_HASH_SIZE 40
#define TIMER_CLICK 5
#define VIRTUAL_CLICK 10000
#define NAME_MAX_IN_PIPE 10
#define YOU_MAX_IN_PIPE 3
#define MAX_RES 1048576
#define SYNC_TIME 60
#define NOTE_SYNC_TIME 1800
#define PING_INTERVAL 10	/* time in secs a users site is pinged */
#define ULTRIX_PLAYER_LIM 200
#define TERM_LINES 16


#define SYS_ROOM_OWNER "main"

 /* if you change these, make sure to have rooms for them in files/main.rooms */
#define ENTRANCE_ROOM "room"
#define RELAXED_ROOM "potty"

#define NEWS_SYNC_INTERVAL (60 * 10) /* how often news has a chance to sync */

/* Additional defines not covered in autoconfig.h or above */

#define LINE "---------------------------------------------------------------------------\n"

#if INTERCOM
#define INTERCOM_SOCKET "junk/intercom_socket"
#define INTERCOM_NAME get_config_msg("talker_name")
#endif

#define WHO_IS_THAT        " No such person in saved files...\n"
#define NOT_HERE_ATM       " That person isn't on right now...\n"

/* Playground Plus version - This value should NOT be changed unless you
   are specifically instructed to do so by an OFFICAL update patch file
   from http://pgplus.ewtoo.org
*/

#define PG_VERSION "1.0.9"

