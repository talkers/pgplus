/*
 * Playground+ - angel.c v1.6
 * Watches over the talker and reboots it when it goes down
 * ---------------------------------------------------------------------------
 *
 *  This is a major update to the PG96 angel including better
 *  memory usage, angel PID storing, soft messages inclusion and the ability
 *  to clean compile --Silver
 *
 */

#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/socket.h>
#ifdef LINUX
#include "include/un.h"
#else
#include <sys/un.h>
#endif
#include <sys/ioctl.h>
#if !defined(linux)
#include <sys/filio.h>
#endif /* LINUX */

#include "include/config.h"
#include "xstring.c"

/* a trade off, we have to define these here cause angel 
   doesnt access softmessages */
typedef struct file
{
  char *where;
  int length;
}
file;

file config_msg;
file load_file(char *);
char *get_config_msg(char *);

char angel_name[256];
char server_name[256];

char *stack, *stack_start;
int fh = 0, die = 0, crashes = 0, syncing = 0;
long int time_out = 0, t = 0;
int no_tty = 0;

/* we need to have these here for xstring stuffs */
char *end_string(char *str)
{
  str = strchr(str, 0);
  str++;
  return str;
}

void lower_case(char *str)
{
  while (*str)
    *str++ = tolower(*str);
}



/* return a string of the system time */

char *sys_time(void)
{
  time_t tt;
  static char time_string[25];

  tt = time(0);
  strftime(time_string, 25, "%H:%M:%S - %d/%m/%y", localtime(&tt));
  return time_string;
}


/* log errors and things to file */

void log(char *filename, char *string)
{
  int fd, length;

  sprintf(stack, "logs/%s.log", filename);
#ifdef BSDISH
  fd = open(stack, O_CREAT | O_WRONLY | S_IRUSR | S_IWUSR);
#else
  fd = open(stack, O_CREAT | O_WRONLY | O_SYNC, S_IRUSR | S_IWUSR);
#endif
  length = lseek(fd, 0, SEEK_END);
  if (length > (atoi(get_config_msg("max_log_size")) * 1024))
  {
    close(fd);
#ifdef BSDISH
    fd = open(stack, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
#else
    fd = open(stack, O_CREAT | O_WRONLY | O_SYNC | O_TRUNC, S_IRUSR | S_IWUSR);
#endif
  }
  sprintf(stack, "%s - %s\n", sys_time(), string);
  if (!no_tty)
    printf(stack);
  write(fd, stack, strlen(stack));
  close(fd);
}


void error(char *str)
{
  log("angel", str);
  exit(-1);
}

void sigpipe(int c)
{
  error("Sigpipe received.");
}

void sighup(int c)
{
  kill(fh, SIGHUP);
  die = 1;
}

void sigquit(int c)
{
  error("Quit signal received.");
}

void sigill(int c)
{
  error("Illegal instruction.");
}

void sigfpe(int c)
{
  error("Floating Point Error.");
}

void sigbus(int c)
{
  error("Bus Error.");
}

void sigsegv(int c)
{
  error("Segmentation Violation.");
}

void sigsys(int c)
{
  error("Bad system call.");
}

void sigterm(int c)
{
  error("Terminate signal received.");
}

void sigxfsz(int c)
{
  error("File descriptor limit exceeded.");
}

void sigchld(int c)
{
  log("angel", "Received SIGCHLD");
  return;
}


/* Woo woo, the main function thang */

int main(int argc, char *argv[])
{
#ifdef REDHAT5
  unsigned int length;
#else /* REDHAT5 */
  int length;
#endif /* !REDHAT5 */
  int status, alive_fd = 0, sock_fd, dieing;
  FILE *angel_pid_fd;
  struct sockaddr_un sa;
  char dummy;
  fd_set fds;
  struct timeval timeout;
#if defined(hpux) | defined(linux)
  struct sigaction siga;
#endif /* hpux | linux */

  stack_start = (char *) malloc(1000);
  stack = stack_start;


  if (chdir(ROOT) < 0)
  {
    fprintf(stderr, "\n\nCant change to '" ROOT "'!!\n\n");
    exit(-1);
  }

  config_msg = load_file("soft/config.msg");	/* get it loaded in */

  /* process names */
  sprintf(angel_name, "-=> %s <=- Guardian Angel watching port",
	  get_config_msg("talker_name"));

  /* Store angel_pid */

  unlink("junk/ANGEL_PID");
  if (!(angel_pid_fd = fopen("junk/ANGEL_PID", "w")))
  {
    fprintf(stderr, "Unable to create pid log file!\n");
    exit(1);
  }
  fprintf(angel_pid_fd, "%d\n", (int) getpid());
  fclose(angel_pid_fd);

  if (strcmp(angel_name, argv[0]))
  {
    if (!argv[1])
    {
      sprintf(stack, "%d", atoi(get_config_msg("port")));
      execlp("bin/angel", angel_name, stack, 0);
    }
    else
    {
      argv[0] = angel_name;
      execvp("bin/angel", argv);
    }
    error("exec failed");
  }
  if (nice(5) < 0)
    error("Failed to renice");

  t = time(0);
  time_out = t + 60;

#ifdef REDHAT5
  sigemptyset(&siga.sa_mask);
#else
  siga.sa_mask = 0;
#endif

#if defined(hpux) | defined(linux)
  siga.sa_handler = sigpipe;
#ifndef REDHAT5
  siga.sa_mask = 0;
#endif
  siga.sa_flags = 0;
  sigaction(SIGPIPE, &siga, 0);
  siga.sa_handler = sighup;
  sigaction(SIGHUP, &siga, 0);
  siga.sa_handler = sigquit;
  sigaction(SIGQUIT, &siga, 0);
  siga.sa_handler = sigill;
  sigaction(SIGILL, &siga, 0);
  siga.sa_handler = sigfpe;
  sigaction(SIGFPE, &siga, 0);
  siga.sa_handler = sigbus;
  sigaction(SIGBUS, &siga, 0);
  siga.sa_handler = sigsegv;
  sigaction(SIGSEGV, &siga, 0);
#if !defined(linux)
  siga.sa_handler = sigsys;
  sigaction(SIGSYS, &siga, 0);
#endif /* LINUX */
  siga.sa_handler = sigterm;
  sigaction(SIGTERM, &siga, 0);
  siga.sa_handler = sigxfsz;
  sigaction(SIGXFSZ, &siga, 0);
  siga.sa_handler = sigchld;
  sigaction(SIGCHLD, &siga, 0);
#else /* hpux | linux */
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
  signal(SIGCHLD, sigchld);
#endif /* hpux | linux */

  while (!die)
  {
    if (config_msg.where)	/* reload it every time the talker reboots */
      FREE(config_msg.where);
    config_msg = load_file("soft/config.msg");

    t = time(0);
    if (crashes >= 4 && time_out >= t)
    {
      log("error", "Crashing lots!! - Giving up.");
      exit(-1);
    }
    else if (time_out < t)
    {
      time_out = t + 30;
      crashes = 0;
    }
    crashes++;
    printf("\n\n");		/* Silly formatting reason :o) */
    log("angel", "Playground+ guardian angel bootloader v1.6 (07.06.98)");
    dieing = 0;
    fh = fork();
    switch (fh)
    {
      case 0:
	setsid();
	sprintf(server_name, "-=> %s <=- Talk Server on port",
		get_config_msg("talker_name"));
	argv[0] = server_name;
	argv[1] = get_config_msg("port");
/*      execvp("bin/talker", argv); */
	execvp("bin/" TALKER_EXEC, argv);
	error("failed to exec talk server!");
	break;
      case -1:
	error("Failed to fork()");
	break;
      default:
	no_tty = 1;
	unlink(SOCKET_PATH);
	sock_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if (sock_fd < 0)
	  error("failed to create socket!");
	sa.sun_family = AF_UNIX;
	strcpy(sa.sun_path, SOCKET_PATH);
	if (bind(sock_fd, (struct sockaddr *) &sa, sizeof(sa)) < 0)
	  error("failed to bind!");
	if (listen(sock_fd, 1) < 0)
	  error("failed to listen!");
	timeout.tv_sec = 120;
	timeout.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(sock_fd, &fds);
	if (select(FD_SETSIZE, &fds, 0, 0, &timeout) <= 0)
	{
	  kill(fh, SIGKILL);
	  log("angel", "Killed server before connect");
	  waitpid(fh, &status, 0);
	}
	else
	{
	  length = sizeof(sa);
	  alive_fd = accept(sock_fd, (struct sockaddr *) &sa, &length);
	  if (alive_fd < 0)
	    error("bad accept!?");
	  close(sock_fd);

	  while (waitpid(fh, &status, WNOHANG) <= 0)
	  {
	    timeout.tv_sec = 300;
	    timeout.tv_usec = 0;
	    FD_ZERO(&fds);
	    FD_SET(alive_fd, &fds);
	    if (select(FD_SETSIZE, &fds, 0, 0, &timeout) <= 0)
	    {
	      if (errno != EINTR)
	      {
		if (dieing)
		{
		  kill(fh, SIGKILL);
		  log("angel", "Server KILLED");
		}
		else
		{
		  kill(fh, SIGTERM);
		  log("angel", "Server TERMINATED");
		  dieing = 1;
		}
	      }
	    }
	    else
	    {
	      if (ioctl(alive_fd, FIONREAD, &length) < 0)
		error("bad FIONREAD");
	      if (!length)
	      {
		kill(fh, SIGKILL);
		log("angel", "Server disconnected");
		dieing = 1;
	      }
	      else
	      {
		for (; length; length--)
		{
		  read(alive_fd, &dummy, 1);
		}
	      }
	    }
	  }
	}

	close(alive_fd);
	switch ((status & 255))
	{
	  case 0:
	    log("angel", "Server exited safely");
	    break;
	  case 127:
	    sprintf(stack, "Server stopped due to signal %d.",
		    (status >> 8) & 255);
	    while (*stack)
	      stack++;
	    stack++;
	    log("angel", stack_start);
	    stack = stack_start;
	    break;
	  default:
	    sprintf(stack, "Server terminated due to signal %d.",
		    status & 127);
	    while (*stack)
	      stack++;
	    stack++;
	    log("angel", stack_start);
	    stack = stack_start;
	    if (status & 128)
	      log("angel", "Core dump produced - oops!?");
	    break;
	}
	break;
    }
  }
  exit(0);
}

/* we need this here as well... */

file load_file(char *filename)
{
  file f;
  int d;
  char *oldstack;

  oldstack = stack;

  d = open(filename, O_RDONLY);
  if (d < 0)
  {
    sprintf(oldstack, "Angel can't find file:%s", filename);
    stack = end_string(oldstack);
    log("error", oldstack);
    f.where = (char *) MALLOC(1);
    *(char *) f.where = 0;
    f.length = 0;
    stack = oldstack;
    return f;
  }
  f.length = lseek(d, 0, SEEK_END);
  lseek(d, 0, SEEK_SET);
  f.where = (char *) MALLOC(f.length + 1);
  memset(f.where, 0, f.length + 1);
  if (read(d, f.where, f.length) < 0)
  {
    sprintf(oldstack, "Angel error reading file:%s", filename);
    stack = end_string(oldstack);
    log("error", oldstack);
    f.where = (char *) MALLOC(1);
    *(char *) f.where = 0;
    f.length = 0;
    stack = oldstack;
    return f;
  }
  close(d);
  stack = oldstack;
  *(f.where + f.length) = 0;
  return f;
}

char *get_config_msg(char *type)
{
  char *got;
  static char smbuf[1024];

  memset(smbuf, 0, 1024);
  if (!config_msg.where || !*config_msg.where)
  {
    log("error", "Angel : Softmsg file for config_msg aint loaded!");
    return "error";
  }
  got = lineval(config_msg.where, type);
  if (!got || !*got)
  {
    log("error", "Angel : Softmsg in config_msg isnt there!");
    return "error";
  }

  strncpy(smbuf, got, 1023);

  return smbuf;
}
