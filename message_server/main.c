/*
 * main.c
 * The Main line of the Message Server
 *
 * $Id: main.c,v 1.3 1996/03/29 16:51:01 athan Exp $
 *
 * $Log: main.c,v $
 * Revision 1.3  1996/03/29 16:51:01  athan
 * Proper cli arguments now
 * Added code for restart, background, verbose, SIGUSR1 support
 *
 * Revision 1.2  1996/03/28 20:02:07  athan
 * General cleanups
 *
 *
 */

#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#if defined(SUNOS)
#include <memory.h>
#endif
#include <sys/wait.h>

#include "config.h"
#include "file.h"

/* Extern Function Prototypes */

extern int accept_connection(void);
extern int check_timeout(long thistime, int *count);
extern void do_login(int newfd, long ltime);
extern void init_comms(void);
extern void load_file(char *fn, struct afile *it);
extern int open_main_socket(int port);
extern void shut_down(void);

/* Extern Variables */

extern char *optarg;
extern int optind, opterr, optopt;
extern struct afile thefile;

/* Local Function Prototypes */

void catch_hup(int i);
void catch_int(int i);
void cleanupanddie(int i);
void catch_alrm(int i);
void do_help(void);
void set_read_msg_file(int i);
void do_server(void);
void sendusr1_to_child(int i);

/* Local Variables */

int status = INIT;
int read_msg_file = 0;
int port = DEFAULT_PORT;
char *msg_file = DEFAULT_MSG_FILE;
int loop = 0;
time_t loopdelay = 1;
int verbose = 0;
struct afile thefile =
{NULL, 0};
int timeout = DEFAULT_TIMEOUT;
int childpid;


int main(int argc, char *argv[])
{
  int optret;
  int restart = 0;
  int background = 0;
  int childstatus;

  /* Process command line arguments */
  while (-1 != (optret = getopt(argc, argv, "p:m:l:t:vrbh")))
  {
    switch (optret)
    {
    case 'p':			/* Port number */
      port = atoi(optarg);
      if (getuid() && port < 1024)
      {
	fprintf(stderr, "%s: Must be uid 0 to use ports below 1024!\n",
		argv[0]);
	exit(-1);
      }
      break;
    case 'm':			/* Message file */
      msg_file = strdup(optarg);
      if (!msg_file)
      {
	fprintf(stderr, "%s: Error duplicating file name!\n",
		argv[0]);
	exit(-1);
      }
      break;
    case 'l':			/* Loop until can bind port */
      loop = 1;
      loopdelay = atoi(optarg);
      if (loopdelay < 1)
      {
	fprintf(stderr, "%s: Loop delay must be at least 1 second\n",
		argv[0]);
	exit(-1);
      }
      break;
    case 't':			/* Connection timeout */
      timeout = atoi(optarg);
      if (timeout < 0)
      {
	fprintf(stderr, "%s: Timeout must be positive\n", argv[0]);
	exit(-1);
      }
    case 'v':			/* Print (verbose) errors */
      verbose++;
      break;
    case 'r':			/* Restart if crash */
      restart++;
      /* Implies background */
      background++;
      break;
    case 'b':			/* Run in background */
      background++;
      break;
    case 'h':			/* Help */
      printf("%s: Help...\n", argv[0]);
      do_help();
      exit(0);
      break;
    case ':':			/* Missing argument */
      fprintf(stderr, "%s: Missing argument\n", argv[0]);
      exit(-1);
      break;
    case '?':			/* Unknown option */
      break;
    default:			/* Erk ? */
      printf("%s: Erk, option processing caused an error!\n", argv[0]);
    }
  }

  if (background)
  {
    childpid = fork();
    switch (childpid)
    {
    case -1:			/* Error */
      fprintf(stderr, "%s: Failed to fork!\n", argv[0]);
      exit(-1);
    case 0:			/* Child */
      setpgrp();
      break;
    default:			/* Parent */
      exit(0);
    }
  }
  if (restart)
  {
    /* Server should restart if it dies for any reason */
    while (1)
    {
      signal(SIGHUP, SIG_IGN);
      signal(SIGINT, cleanupanddie);
      signal(SIGQUIT, cleanupanddie);
      signal(SIGTERM, cleanupanddie);
      signal(SIGUSR1, sendusr1_to_child);
      childpid = fork();
      switch (childpid)
      {
      case -1:			/* Error */
	exit(-1);
      case 0:			/* Child */
	do_server();
	break;
      default:			/* Parent */
	waitpid(childpid, &childstatus, 0);
	if (WEXITSTATUS(childstatus) == 42)
	{
	  /* Child exitted normally */
	  exit(0);
	}
      }
      sleep(10);
    }
  }
  else
  {
    do_server();
  }

  return 0;
}

void do_server(void)
{
  int newfd;
  int count = 0;
  long mytime = 0;
  struct itimerval oldtimer, newtimer;
  int ret = 0;

  signal(SIGTERM, catch_int);
  signal(SIGINT, catch_int);
  signal(SIGHUP, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGALRM, catch_alrm);
  signal(SIGUSR1, set_read_msg_file);

  newtimer.it_interval.tv_sec = loopdelay;
  newtimer.it_interval.tv_usec = 0;
  newtimer.it_value.tv_sec = loopdelay;
  newtimer.it_value.tv_usec = 0;
  setitimer(ITIMER_REAL, &newtimer, &oldtimer);

  init_comms();
  /* Attempt to open a socket and bind it to the port */
  do
  {
    if ((ret = open_main_socket(port)) < 0)
    {
      if (verbose || !loop)
      {
	fprintf(stderr, "Error: Failed to open main socket.\n");
      }
      if (!loop)
      {
	exit(-1);
      }
      sigpause(SIGALRM);
    }
  }
  while (loop && (ret < 0));
  if (verbose)
  {
    fprintf(stderr, "Socket open and bound to port %d\n", port);
  }
  status = RUNNING;

  /* Load the message */

  load_file(msg_file, &thefile);
  /* Accept new connections */
  while (status != SHUTDOWN)
  {
    if (read_msg_file)
    {
      load_file(msg_file, &thefile);
      read_msg_file = 0;
    }
    mytime = (long) time(NULL);
    /* Check for new connections */
    if (-1 != (newfd = accept_connection()))
    {
      do_login(newfd, mytime);
      count++;
    }
    /* Check for timeout on the logins */
    while (check_timeout(mytime, &count) == -1)
    {
    }
    sigpause(SIGALRM);
  }

  shut_down();
  exit(42);
}

void catch_alrm(int i)
{
  signal(SIGALRM, catch_alrm);
}


void do_help(void)
{
  printf(" Possible command line arguments are:\n"
	 "\n"
	 "  -p <port>               - Set port to listen on\n"
	 "  -m <file>               - Set message file to use\n"
	 "  -l <delay (seconds)>    - Loop until port is bound\n"
	 "  -t <timeout (seconds)>  - Time before connection close\n"
	 "  -v                      - Verbose messages\n"
	 "  -r                      - Run with 'angel'\n"
	 "  -b                      - Run in background (implied by -r)\n"
	 "  -h                      - This help message\n");
}


void set_read_msg_file(int i)
{
  read_msg_file = 1;
  signal(SIGUSR1, set_read_msg_file);
}

void catch_int(int i)
{
  status = SHUTDOWN;
}

void cleanupanddie(int i)
{
  kill(childpid, SIGKILL);
  exit(0);
}

void sendusr1_to_child(int i)
{
  kill(childpid, SIGUSR1);
  signal(SIGUSR1, sendusr1_to_child);
}
