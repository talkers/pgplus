/*
 * Playground+ - dsc.c
 * Simple little version file so we have with compile time         ~ phypor
 * ---------------------------------------------------------------------------
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "include/config.h"
#include "include/player.h"
#include "include/proto.h"
#include "include/version.h"

struct hardware_info
{
  int allgood;
  int processors;
  int ram;
  float bogomips;
  char cpu[40];
  char model[40];
  char vendor[40];
}
CPU;

/* --------------------------------------------------------------------------

   This is the Playground+ version command which may only be modified
   to list additional modules installed by yourself. The credits must
   be kept the same!

   This pg_version has been modified with permission.

   -------------------------------------------------------------------------- */

void pg_version(player * p, char *str)
{
  char *oldstack = stack;
  int ot = 0;

  if (!strcmp(str, "-"))
  {
    TELLPLAYER(p, "\n %s is running Playground Plus v%s\n"
	       " (compilation date: %s)\n", get_config_msg("talker_name"), PG_VERSION, COMPILE_TIME);
    return;
  }

  pstack_mid("Playground+ Version Information");

  sprintf(stack, "\nThis talker is based on Playground Plus by Silver (Richard Lawrence),\nblimey (Geoffrey Swift) and phypor (J. Bradley Christian), a stable\nbug"
	  "fixed and improved version of Playground 96 by traP (Mike Bourdaa),\nastyanax (Chris Allegretta), Nogard (Hans Peterson) and "
	  "vallie (Valerie Kelly)\nwhich is itself based on Summink by Athanasius (Neil Peter Charley)\nwhich itself was"
	  " based on EW-Too by Burble (Simon Marsh).\n\n");
  stack = strchr(stack, 0);

  sprintf(stack, " -=*> Using Playground+ v%s base code.\n", PG_VERSION);
  stack = strchr(stack, 0);

#ifdef LINUX
#ifdef REDHAT5
  sprintf(stack, " -=*> Playground+ running on Linux (glibc2).\n");
#else
  sprintf(stack, " -=*> Playground+ running on Linux (glibc1).\n");
#endif
#elif SOLARIS
  sprintf(stack, " -=*> Playground+ running on Solaris.\n");
#elif SUNOS
  sprintf(stack, " -=*> Playground+ running on SunOS.\n");
#elif HPUX
  sprintf(stack, " -=*> Playground+ running on HP-UX.\n");
#elif ULTRIX
  sprintf(stack, " -=*> Playground+ running on Ultrix.\n");
#elif BSDISH
  sprintf(stack, " -=*> Playground+ running on *BSD.\n");
#else
  sprintf(stack, " -=*> Playground+ running on Unknown O/S.\n");
#endif
  stack = strchr(stack, 0);

  sprintf(stack, " -=*> Executable compiled %s\n", COMPILE_TIME);
  stack = strchr(stack, 0);

  ot = atoi(get_config_msg("player_timeout"));
  if (!ot)
    stack += sprintf(stack, " -=*> Player timeouts disabled.\n");
  else
    stack += sprintf(stack, " -=*> Player timeouts enabled (%d weeks).\n", ot);


#ifdef INTERCOM
  pg_intercom_version();
#endif

  dynatext_version();

  nunews_version();

#ifdef ROBOTS
  robot_version();
#endif

  sprintf(stack, " -=*> Auto reloading v1.0 (by phypor) enabled.\n");
  stack = strchr(stack, 0);


#ifdef ANTICRASH
  crashrec_version();
#endif

  if (config_flags & cfNOSPAM)
  {
    sprintf(stack, " -=*> Anti-spam code v3.0 (by Silver) enabled.\n");
    stack = strchr(stack, 0);
  }

  channel_version();

#ifdef COMMAND_STATS
  command_stats_version();
#endif

  slots_version();

#ifdef AUTOSHUTDOWN
  sprintf(stack, " -=*> Auto-shutdown code v1.0 (by Silver) enabled.\n");
  stack = strchr(stack, 0);
#endif

#ifdef LAST
  last_version();
#endif

#ifdef NEWPAGER
  sprintf(stack, " -=*> New pager code v1.1 (by Nightwolf) enabled.\n");
  stack = strchr(stack, 0);
#endif

  spodlist_version();

#ifdef SEAMLESS_REBOOT
  reboot_version();
#endif

  socials_version();

#ifdef IDENT
  ident_version();
#endif

  if (config_flags & cfNOSWEAR)
    swear_version();

  stack += sprintf(stack, " -=*> ESE, expandable search engine v1.3 (by phypor) enabled\n");

#ifdef ALLOW_MULTIS
  multis_version();
#endif

  softmsg_version();

/* A warning that people are using debugging mode. This means sysops can
   slap silly people who use this mode in live usage -- Silver */

#ifdef DEBUGGING
#ifdef DEBUG_VERBOSE
  stack += sprintf(stack, "\n    -=*> WARNING! Talk server running in verbose ");
#else
  stack += sprintf(stack, "\n        -=*> WARNING! Talk server running in ");
#endif
  stack += sprintf(stack, "debugging mode! <*=-\n\n");
#endif

  stack = strchr(stack, 0);
  pstack_mid("Playground+ Homepage: http://pgplus.ewtoo.org");
  *stack++ = 0;

  if (p->location)
    pager(p, oldstack);
  else
    tell_player(p, oldstack);

  stack = oldstack;
}

/* net stats */

void netstat(player * p, char *str)
{
  char *oldstack = stack;
  FILE *fp;
  int pid1 = -1, pid2 = -1;

  pstack_mid("Network and server statistics");

  stack += sprintf(stack, "                           In       Out\n");

  stack += sprintf(stack, "Total bytes        : %8d  %8d\n"
		   "Average bytes      : %8d  %8d\n"
		   "Bytes per second   : %8d  %8d\n"
		   "Total packets      : %8d  %8d\n"
		   "Average packets    : %8d  %8d\n"
		   "Packets per second : %8d  %8d\n\n",
	      in_total, out_total, in_average, out_average, in_bps, out_bps,
		   in_pack_total, out_pack_total, in_pack_average,
		   out_pack_average, in_pps, out_pps);

  stack += sprintf(stack, "Talker name        : %s\n", get_config_msg("talker_name"));
  stack += sprintf(stack, "Site address       : %s (%s)\n", talker_alpha, talker_ip);
#ifdef INTERCOM
  stack += sprintf(stack, "Port number        : %d (talk server), %d (intercom server)\n", active_port, active_port - 1);
#else
  stack += sprintf(stack, "Port number        : %d\n", active_port);
#endif
  stack += sprintf(stack, "Base code revision : v%s\n", PG_VERSION);
  stack += sprintf(stack, "Talker root dir    : %s\n", ROOT);
  stack += sprintf(stack, "Compilation date   : %s\n", COMPILE_TIME);
  stack += sprintf(stack, "Process ID's       : ");

  /* Now we need to get the PIDS */

  fp = fopen("junk/PID", "r");
  if (fp)
  {
    fscanf(fp, "%d", &pid1);
    fclose(fp);
    stack += sprintf(stack, "%d (talk server)", pid1);
  }
  else
    stack += sprintf(stack, "unknown talk server PID");


  fp = fopen("junk/ANGEL_PID", "r");
  if (fp)
  {
    fscanf(fp, "%d", &pid2);
    fclose(fp);
    if (pid1 != -1)
      stack += sprintf(stack, ", ");
    stack += sprintf(stack, "%d (guardian angel)", pid2);
  }
  else
  {
    if (pid1 != -1)
      stack += sprintf(stack, ", ");
    stack += sprintf(stack, "unknown guardian angel PID");
  }
  stack += sprintf(stack, "\n");
  stack += sprintf(stack, "Machine `uname`    : %s\n", UNAME);

  if (CPU.allgood)
    stack += sprintf(stack, "CPU Stats          : %s %s %s %s Processor%s %dM RAM\n"
		     "Bogomips           : %.2f\n" LINE,
	      number2string(CPU.processors), CPU.vendor, CPU.cpu, CPU.model,
		     (CPU.processors > 1) ? "s" : "", CPU.ram, CPU.bogomips);
  else
    stack += sprintf(stack, "CPU Stats          : Unavailable\n" LINE);

  stack = end_string(stack);
  tell_player(p, oldstack);
  stack = oldstack;
}

/* 
   the idea for this was taken from linux_logo, 
   however the codes been totally rewritten
   ~phy

   (only claimed to work on linux boxen)
 */

void get_hardware_info(void)
{
  FILE *f;
  char red[40], dummy[40];
  float bmc;
  struct stat sbuf;

  if (!(f = fopen("/proc/cpuinfo", "r")))
  {
    CPU.allgood = 0;
    return;
  }
  while (fgets(red, 39, f))
  {
    if (!(strncasecmp(red, "bogomips", 8)))
    {
      CPU.processors++;
      sscanf(red, "%s : %f", dummy, &bmc);
      CPU.bogomips += bmc;
      continue;
    }
    if (CPU.processors)
      continue;

    if (!(strncasecmp(red, "cpu", 3)) && !strstr(red, "cpuid"))
    {
      sscanf(red, "%s : %s", dummy, CPU.cpu);
      continue;
    }
    if (!(strncasecmp(red, "model", 5)))
    {
      sscanf(red, "%s : %s", dummy, CPU.model);
      continue;
    }
    if (!(strncasecmp(red, "vendor_id", 9)))
    {
      sscanf(red, "%s : %s", dummy, CPU.vendor);
      if (!strcasecmp(CPU.vendor, "AuthenticAMD"))
	strcpy(CPU.vendor, "AMD");
      else if (!strcasecmp(CPU.vendor, "GenuineIntel"))
	strcpy(CPU.vendor, "Intel");
      else if (!strcasecmp(CPU.vendor, "CyrixInstead"))
	strcpy(CPU.vendor, "Cyrix");
      continue;
    }
  }
  fclose(f);

  stat("/proc/kcore", &sbuf);
  CPU.ram = sbuf.st_size / 1024;
  CPU.ram /= 1024;

  CPU.allgood++;
}
