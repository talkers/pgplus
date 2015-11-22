/*
 * Playground+ - softmsg.c
 * The high level access points to the soft message system (c) phypor 1998
 * ---------------------------------------------------------------------------
 *
 * You may extract it and use it in a running talker
 * if you place credit for it either in help credits
 *     Soft Messages by phypor
 * or in the version command output
 *    -=> Soft Messages v1.2 (by phypor)
 *
 * This code is licenced for distribution in PG+ ONLY
 * You may not distribute this code in any form other than
 * within the code release PG+, you may only distribute
 * it in a successor of PG+ with express consent in writing
 * from phypor (j. bradley christian) <phypor@benland.muc.edu>
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "include/config.h"
#include "include/player.h"
#include "include/proto.h"

char smbuf[1024];		/* lil chunk of global memory we use */

char *get_config_msg(char *type)
{
  char *got;

  memset(smbuf, 0, 1024);
  if (!config_msg.where || !*config_msg.where)
  {
    if (!strcasecmp(type, "max_log_size"))	/* ouch */
      return "5000";
    log("error", "Softmsg file for config_msg aint loaded!");
    return "error";
  }
  got = lineval(config_msg.where, type);
  if (!got || !*got)
  {
    if (!strcasecmp(type, "logon_prefix") ||
	!strcasecmp(type, "logon_suffix") ||
	!strcasecmp(type, "logoff_prefix") ||
	!strcasecmp(type, "logoff_suffix") ||
	!strcasecmp(type, "site_alias") ||
	!strcasecmp(type, "welcome_msg"))
      return "";

    LOGF("error", "Softmsg in config_msg for '%s' isnt there!", type);
    return "error";
  }

  strncpy(smbuf, got, 1023);

  return smbuf;
}

char *get_admin_msg(char *type)
{
  char *got;

  memset(smbuf, 0, 1024);

  if (!admin_msg.where || !*admin_msg.where)	/* file aint loaded */
  {
    log("error", "Softmsg file for admin_msg aint loaded!");
    return "-=*> Erg erg erg";
  }
  got = lineval(admin_msg.where, type);
  if (!got || !*got)
  {
    LOGF("error", "Softmsg in admin_msg for '%s' isnt there!", type);
    return "-=*> Erg erg erg";
  }
  strncpy(smbuf, lineval(admin_msg.where, type), 1023);

  return smbuf;
}

char *get_shutdowns_msg(char *type)
{
  char *got;

  memset(smbuf, 0, 1024);

  if (!shutdowns_msg.where || !*shutdowns_msg.where)	/* file aint loaded */
  {
    log("error", "Softmsg file for shutdowns_msg aint loaded!");
    return "";
  }
  got = lineval(shutdowns_msg.where, type);

  if (got && *got)
    strncpy(smbuf, got, 1023);

  return smbuf;			/* its ok to have empty cases here */
}


char *get_frog_msg(char *type)
{
  char *got;

  memset(smbuf, 0, 1024);

  if (!frogs_msg.where || !*frogs_msg.where)
  {
    log("error", "Softmsg file for frogs_msg aint loaded!");
    return "I want my rattle";
  }
  got = lineval(frogs_msg.where, type);
  if (!got || !*got)
  {
    LOGF("error", "Softmsg in frogs_msg for '%s' isnt there!", type);
    return "I want my rattle";
  }

  strncpy(smbuf, lineval(frogs_msg.where, type), 1023);

  return smbuf;
}


char *get_plists_msg(char *type)
{
  char *got;

  memset(smbuf, 0, 1024);
  if (!plists_msg.where || !*plists_msg.where)
  {
    log("error", "Softmsg file for plists_msg aint loaded!");
    return "Erg Erg Erg";
  }
  got = lineval(plists_msg.where, type);

  if (!got || !*got)
  {
    LOGF("error", "Softmsg in plists_msg for '%s' isnt there!", type);
    return "Erg Erg Erg";
  }
  strncpy(smbuf, got, 1023);

  return smbuf;
}

char *get_pdefaults_msg(char *type)
{
  char *got;

  memset(smbuf, 0, 1024);
  if (!pdefaults_msg.where || !*pdefaults_msg.where)
  {
    log("error", "Softmsg file for pdefaults_msg aint loaded!");
    return "Erg Erg Erg";
  }
  got = lineval(pdefaults_msg.where, type);
  if (!got || !*got)
  {
    LOGF("error", "Softmsg in pdefaults_msg for '%s' isnt there!", type);
    return "Erg Erg Erg";
  }

  strncpy(smbuf, got, 1023);

  return smbuf;
}

char *get_session_msg(char *type)
{
  char *got;

  memset(smbuf, 0, 1024);
  if (!session_msg.where || !*session_msg.where)
  {
    log("error", "Softmsg file for session_msg aint loaded!");
    return "not set";
  }
  got = lineval(session_msg.where, type);
  if (!got || !*got)
  {
    LOGF("error", "Softmsg in session_msg for '%s' isnt there!", type);
    return "not set";
  }

  strncpy(smbuf, got, 1023);

  return smbuf;
}

char *get_deflog_msg(char *type)
{
  char *got;

  memset(smbuf, 0, 1024);
  if (!deflog_msg.where || !*deflog_msg.where)
    return "non";

  got = lineval(deflog_msg.where, type);
  if (!got || !*got)
    return "non";

  strncpy(smbuf, got, 1023);

  return smbuf;
}

char *get_rooms_msg(char *type)
{
  char *got;

  memset(smbuf, 0, 1024);
  if (!rooms_msg.where || !*rooms_msg.where)
  {
    log("error", "Softmsg file for rooms_msg aint loaded!");
    return "erg erg erg";
  }
  got = lineval(rooms_msg.where, type);
  if (!got || !*got)
  {
    LOGF("error", "Softmsg in rooms_msg for '%s' isnt there!", type);
    return "erg erg erg";
  }

  strncpy(smbuf, got, 1023);

  return smbuf;
}

void softmsg_version(void)
{
  stack += sprintf(stack, " -=*> Soft Messages v1.2 (by phypor) enabled.\n");
}
