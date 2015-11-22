/*
 * ident_client.c
 * Ident client for rfc1413 lookups
 * ---------------------------------------------------------------------------
 *
 * The majority of the code contained herein is Copyright 1995/1996 by
 * Neil Peter Charley.
 * Portions of the code (notably that for Solaris 2.x and BSD compatibility)
 * are Copyright 1996 James William Hawtin. Thanks also goes to James
 * for pointing out ways to make the code more robust.
 * ALL code contained herein is covered by one or the other of the above
 * Copyright's.
 *
 *     Distributed as part of Playground+ package with permission.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
/*#include <sys/filio.h> */
#include <unistd.h>
#include <sys/wait.h>

#include "include/config.h"
#include "include/player.h"
#include "include/fix.h"
#include "include/proto.h"
#ifdef LINUX
  #include "include/un.h"
#else
  #include <un.h>
#endif

#if defined(HAVE_UNISTDH)
#include <unistd.h>
#endif
#if defined(HAVE_FILIOH)
#include <sys/filio.h>
#endif
#if !defined(OSF)
#include <sys/wait.h>
#endif

#ifdef IDENT

#include "include/ident.h"

/* Local functions */

void kill_ident_server(void);
void process_reply(int msg_size);
void read_ident_reply(void);
void send_ident_request(player * p, struct sockaddr_in *sadd);

/* Local Variables */

#define BUFFER_SIZE 2048	/* Significantly bigger than the
				   * equivalent in ident_server.c
				 */
int ident_toclient_fds[2];
int ident_toserver_fds[2];
int ident_server_pid = 0;
ident_identifier ident_id = 0;
char ident_buf_input[BUFFER_SIZE];
char ident_buf_output[BUFFER_SIZE];
char reply_buf[BUFFER_SIZE];
short int local_port;


/*
 * Start up the ident server
 */

void init_ident_server(void)
{
  int ret;
#if defined(NONBLOCKING)
  int dummy;
#endif /* NONBLOCKING */
  fd_set fds;
  struct timeval timeout;
  char name_buffer[256];

  sprintf(name_buffer, "-=> %s <=- Ident server",
	  get_config_msg("talker_name"));

  local_port = (short int) atoi(get_config_msg("port"));
  if (-1 == pipe(ident_toclient_fds))
  {
    switch (errno)
    {
      case EMFILE:
	log("boot", "init_ident_server: Too many fd's in use by"
	    " process\n");
	exit(1);
      case ENFILE:
	log("boot", "init_ident_server: Too many fd's in use in"
	    " system\n");
	exit(1);
      case EFAULT:
	log("boot", "init_ident_server: ident_toclient_fds invalid!\n");
	exit(1);
    }
  }
  if (-1 == pipe(ident_toserver_fds))
  {
    switch (errno)
    {
      case EMFILE:
	log("boot", "init_ident_server: Too many fd's in use by"
	    " process\n");
	exit(1);
      case ENFILE:
	log("boot", "init_ident_server: Too many fd's in use in"
	    " system\n");
	exit(1);
      case EFAULT:
	log("boot", "init_ident_server: ident_toserver_fds invalid!\n");
	exit(1);
    }
  }

#if defined(SOLARIS)
  /* Ack never use VFORK on SOLARIS 2 it can have very nasty effects */
  ret = fork();
#else /* !SOLARIS */
  ret = vfork();
#endif /* SOLARIS */
  switch (ret)
  {
    case -1:			/* Error */
      log("boot", "init_ident_server couldn't fork!\n");
      exit(1);
    case 0:			/* Child */
      close(IDENT_CLIENT_READ);
      close(IDENT_CLIENT_WRITE);
      close(0);
      dup(IDENT_SERVER_READ);
      close(IDENT_SERVER_READ);
      close(1);
      dup(IDENT_SERVER_WRITE);
      close(IDENT_SERVER_WRITE);
      execlp("bin/ident_server", name_buffer, 0);
      log("boot", "init_ident_server failed to exec ident\n");
      exit(1);
    default:			/* Parent */
      ident_server_pid = ret;
      close(IDENT_SERVER_READ);
      close(IDENT_SERVER_WRITE);
      IDENT_SERVER_READ = IDENT_SERVER_WRITE = -1;
#if defined(NONBLOCKING)
      if (ioctl(IDENT_CLIENT_READ, FIONBIO, &dummy) < 0)
      {
	log("error", "Ack! Can't set non-blocking to ident read.");
      }
      if (ioctl(IDENT_CLIENT_WRITE, FIONBIO, &dummy) < 0)
      {
	log("error", "Ack! Can't set non-blocking to ident write.");
      }
#endif /* NONBLOCKING */
  }

  FD_ZERO(&fds);
  FD_SET(IDENT_CLIENT_READ, &fds);
  timeout.tv_sec = 5;
  timeout.tv_usec = 0;
  if (0 >= (select(FD_SETSIZE, &fds, 0, 0, &timeout)))
  {
    log("boot", "init_ident_server: Timed out waiting for server"
	" connect\n");
    kill_ident_server();
    return;
  }

  ioctl(IDENT_CLIENT_READ, FIONREAD, &ret);
  while (ret != strlen(SERVER_CONNECT_MSG))
  {
    sleep(1);
    ioctl(IDENT_CLIENT_READ, FIONREAD, &ret);
  }
  ret = read(IDENT_CLIENT_READ, ident_buf_input, ret);
  ident_buf_input[ret] = '\0';
  if (strcmp(ident_buf_input, SERVER_CONNECT_MSG))
  {
    fprintf(stderr, "From Ident: '%s'\n", ident_buf_input);
    log("boot", "init_ident_server: Bad connect from server, killing\n");
    kill_ident_server();
    return;
  }
  log("boot", "Ident Server Up and Running");

  return;
}

/* Shutdown the ident server */

void kill_ident_server(void)
{
  int status;

  close(IDENT_CLIENT_READ);
  close(IDENT_CLIENT_WRITE);
  IDENT_CLIENT_READ = -1;
  IDENT_CLIENT_WRITE = -1;
  kill(ident_server_pid, SIGTERM);
  waitpid(-1, &status, WNOHANG);
}

void send_ident_request(player * p, struct sockaddr_in *sadd)
{
  char *s;
  int bwritten;

  s = ident_buf_output;
  *s++ = CLIENT_SEND_REQUEST;
  memcpy(s, &ident_id, sizeof(ident_id));
  s += sizeof(ident_id);
  memcpy(s, &(local_port), sizeof(local_port));
  s += sizeof(local_port);
  memcpy(s, &(sadd->sin_family), sizeof(sadd->sin_family));
  s += sizeof(sadd->sin_family);
  memcpy(s, &(sadd->sin_addr.s_addr), sizeof(sadd->sin_addr.s_addr));
  s += sizeof(sadd->sin_addr.s_addr);
  memcpy(s, &(sadd->sin_port), sizeof(sadd->sin_port));
  s += sizeof(sadd->sin_port);
  bwritten = write(IDENT_CLIENT_WRITE, ident_buf_output,
		   (s - ident_buf_output));

  p->ident_id = ident_id++;
  if (bwritten < (s - ident_buf_output))
  {
    log("ident", "Client failed to write request, killing and restarting"
	" Server\n");
    kill_ident_server();
    sleep(3);
    init_ident_server();
    bwritten = write(IDENT_CLIENT_WRITE, ident_buf_output,
		     (s - ident_buf_output));
    if (bwritten < (s - ident_buf_output))
    {
      log("ident", "Restart failed\n");
    }
  }

}

void read_ident_reply(void)
{
  int bread;
  int toread;
  int i;
  static int bufpos = 0;

  ioctl(IDENT_CLIENT_READ, FIONREAD, &toread);
  if (toread <= 0)
  {
    return;
  }
  bread = read(IDENT_CLIENT_READ, ident_buf_input, BUFFER_SIZE - 20);
  ident_buf_input[bread] = '\0';

  for (i = 0; i < bread;)
  {
    reply_buf[bufpos++] = ident_buf_input[i++];
    if ((bufpos > (sizeof(char) + sizeof(ident_identifier)))
	&& (reply_buf[bufpos - 1] == '\n'))
    {
      process_reply(bufpos);
      bufpos = 0;
#if defined(HAVE_BZERO)
      bzero(reply_buf, BUFFER_SIZE);
#else
      memset(reply_buf, 0, BUFFER_SIZE);
#endif /* HAVE_BZERO */
    }
  }
}

void process_reply(int msg_size)
{
  char *s;
  int i;
  ident_identifier id;
  player *scan;

  for (i = 0; i < msg_size;)
  {
    switch (reply_buf[i++])
    {
      case SERVER_SEND_REPLY:
	memcpy(&id, &reply_buf[i], sizeof(ident_identifier));
	i += sizeof(ident_identifier);
	for (scan = flatlist_start; scan; scan = scan->flat_next)
	{
	  if (scan->ident_id == id)
	  {
	    break;
	  }
	}
	s = strchr(&reply_buf[i], '\n');
	if (s)
	{
	  *s++ = '\0';
	}
	else
	{
	  s = strchr(reply_buf, '\0');
	  *s++ = '\n';
	  *s = '\0';
	}
	if (scan)
	{
	  strncpy(scan->remote_user, &reply_buf[i],
		  (MAX_REMOTE_USER < (s - &reply_buf[i]) ?
		   MAX_REMOTE_USER : (s - &reply_buf[i])));
	}
	else
	{
	  /* Can only assume connection dropped from here and we still
	   * somehow got a reply, throw it away
	   */
	}
	while (reply_buf[i] != '\n')
	{
	  i++;
	}
	break;
      default:
	i++;
    }
  }
}


/* ident version */
void ident_version(void)
{
  sprintf(stack, " -=*> Ident server v1.01 (by Athanasius and Oolon) enabled.\n");
  stack = strchr(stack, 0);
}


#endif
