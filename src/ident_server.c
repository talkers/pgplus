/*
 * ident_server.c
 * Ident server for rfc1413 lookups
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
 *       Distributed as part of Playground+ package with permission.
 */

#include <unistd.h>
#if defined(HAVE_FILIOH)
#include <sys/filio.h>
#endif /* HAVE_FILIOH */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/resource.h>

#include "include/config.h"
#include "include/player.h"
#include "include/fix.h"
#include "include/proto.h"

#ifdef BSD
#undef BSD
#endif

#ifdef IDENT

#include "include/ident.h"


/* Extern Function Prototypes */

/* Extern Variables */

/*extern int errno; */

#if !defined(OSF)
#if defined(BSD)
  /* Missing BSD PROTOTYPES */
/*  extern int  setrlimit(int, struct rlimit *); */
extern int getrlimit(int, struct rlimit *);
/*  extern int  setitimer(int, struct itimerval *, struct itimerval *); */
extern int fclose();
extern int select();
extern time_t time(time_t *);
extern int connect();
extern int shutdown(int, int);
extern int socket(int, int, int);
/*  extern int  strftime(char *, int, char *, struct tm *);
   extern void  perror(char *); */
extern int fflush();
extern int ioctl();
extern int fprintf();
/*  extern void bzero(void *, int);
   extern void  bcopy(char *, char *, int);
   extern int      vfprintf(FILE *stream, char *format, ...); */
extern int sigpause(int);
#endif /* BSD */
#else /* OSF */
extern int ioctl(int d, unsigned long request, void *arg);
#endif /* !OSF */

/* Local Function Prototypes */

void catch_sigterm(SIGTYPE);
void catch_sigalrm(SIGTYPE);
void check_connections(void);
void check_requests(void);
void closedown_request(int slot);
void do_request(ident_request * id_req);
void idlog(char *,...);
void process_request(int slot);
void process_result(int slot);
void queue_request(ident_identifier id, short int local_port,
		   struct sockaddr_in *sockadd);
void take_off_queue(int freeslot);
void write_request(ident_request * id_req);

/* Local Variables */

#define BUFFER_SIZE 1024
char scratch[4096];
char req_buf[BUFFER_SIZE];
ident_request idents_in_progress[MAX_IDENTS_IN_PROGRESS];
ident_request *first_inq = NULL;
ident_request *last_inq = NULL;
int debug = 0;
int status = 0;
int beats_per_second;
int req_size;

#define STATUS_RUNNING 1
#define STATUS_SHUTDOWN 2

int main(int argc, char *argv[])
{
#if !defined(NOALARM)
  struct itimerval new_timer, old_timer;
#endif /* !NOALARM */
  struct rlimit rlp;
#if defined(USE_SIGACTION)
  struct sigaction sigact;
#endif
  struct sockaddr_in sadd;

  getrlimit(RLIMIT_NOFILE, &rlp);
  rlp.rlim_cur = rlp.rlim_max;
  setrlimit(RLIMIT_NOFILE, &rlp);

  /* First things first, let the client know we're alive */
  write(1, SERVER_CONNECT_MSG, strlen(SERVER_CONNECT_MSG));

  /* Need this to keep in sync with the client side

   * <start><ident_id><local_port><sin_family><s_addr><sin_port>
   */
  req_size = sizeof(char) + sizeof(ident_identifier) + sizeof(short int)
  + sizeof(sadd.sin_family) + sizeof(sadd.sin_addr.s_addr)
  + sizeof(sadd.sin_port);

  /* Set up signal handling */
#if defined(USE_SIGACTION)
#if defined(USE_SIGEMPTYSET)
  sigemptyset(&(sigact.sa_mask));
  sigact.sa_sigaction = (void *) 0;
#else
#ifdef REDHAT5
  sigemptyset(&sigact.sa_mask);
#else
  sigact.sa_mask = 0;
#endif
#if defined(LINUX)
  sigact.sa_restorer = (void *) 0;
#endif
#endif /* USE_SIGEMPTYSET */
  sigact.sa_handler = catch_sigterm;
  sigaction(SIGTERM, &sigact, (struct sigaction *) 0);
#if !defined(NOALARM)
  sigact.sa_handler = catch_sigalrm;
  sigaction(SIGALRM, &sigact, (struct sigaction *) 0);
#endif /* !NOALARM */
  sigact.sa_handler = SIG_IGN;
  sigaction(SIGPIPE, &sigact, (struct sigaction *) 0);
#else /* !USE_SIGACTION */
  signal(SIGTERM, catch_sigterm);
#if !defined(NOALARM)
  signal(SIGALRM, catch_sigalrm);
#endif /* !NOALARM */
  signal(SIGPIPE, SIG_IGN);
#endif /* USE_SIGACTION */

#if !defined(NOALARM)
  beats_per_second = 5;
  /* Set up a timer to wake us up now and again */
  new_timer.it_interval.tv_sec = 0;
  new_timer.it_interval.tv_usec = 1000000 / beats_per_second;
  new_timer.it_value.tv_sec = 0;
  new_timer.it_value.tv_usec = new_timer.it_interval.tv_usec;
  if (0 > setitimer(ITIMER_REAL, &new_timer, &old_timer))
  {
    perror("ident");
  }
#endif /* !NOALARM */
#if defined(HAVE_BZERO)
  bzero(&idents_in_progress[0], MAX_IDENTS_IN_PROGRESS
	* sizeof(ident_request));
#else /* !HAVE_BZERO */
  memset(&idents_in_progress[0], 0,
	 MAX_IDENTS_IN_PROGRESS * sizeof(ident_request));
#endif /* HAVE_BZERO */
  /* Now enter the main loop */
  status = STATUS_RUNNING;
  while (status != STATUS_SHUTDOWN)
  {
    if (1 == getppid())
    {
      /* If our parent is now PID 1 (init) the talker must have died without
       * killing us, so we have no business still being here
       *
       * time to die...
       */
      exit(0);
    }
    check_requests();
    check_connections();
#if !defined(NOALARM)
    sigpause(0);
#endif /* !NOALARM */
  }

  return 0;
}

/* Catch a SIGTERM, this means to shutdown */

void catch_sigterm(SIGTYPE)
{
  status = STATUS_SHUTDOWN;
}

#if !defined(NOALARM)
/* Catch a SIGALRM, we do nothing on this */

void catch_sigalrm(SIGTYPE)
{
}
#endif /* !NOALARM */


/* Check our actual connections out for activity */

void check_connections(void)
{
  fd_set fds_write;
  fd_set fds_read;
  int i;
  struct timeval timeout;
  time_t now;

  FD_ZERO(&fds_write);		/* These are for connection being established */
  FD_ZERO(&fds_read);		/* These are for a reply being ready */

  /*   FD_SET(0, &fds_read); */
  for (i = 0; i < MAX_IDENTS_IN_PROGRESS; i++)
  {
    if (idents_in_progress[i].local_port)
    {
      if (idents_in_progress[i].flags & IDENT_CONNREFUSED)
      {
	process_result(i);
      }
      else if (!(idents_in_progress[i].flags & IDENT_CONNECTED))
      {
	FD_SET(idents_in_progress[i].fd, &fds_write);
      }
      else
      {
	FD_SET(idents_in_progress[i].fd, &fds_read);
      }
    }
    else
    {
      /* Free slot, so lets try to fill it */
      take_off_queue(i);
    }
  }

#if defined(NOALARM)
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
#else /* !NOALARM */
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
#endif /* NOALARM */
  i = select(FD_SETSIZE, &fds_read, &fds_write, 0, &timeout);
  switch (i)
  {
    case -1:
      break;
    case 0:
      break;
    default:
      for (i = 0; i < MAX_IDENTS_IN_PROGRESS; i++)
      {
	if (FD_ISSET(idents_in_progress[i].fd, &fds_write))
	{
	  /* Has now connected, so send request */
	  idents_in_progress[i].flags |= IDENT_CONNECTED;
	  write_request(&idents_in_progress[i]);
	}
	else if (FD_ISSET(idents_in_progress[i].fd, &fds_read))
	{
	  /* Reply is ready, so process it */
	  idents_in_progress[i].flags |= IDENT_REPLY_READY;
	  process_result(i);
	}
      }
  }

  now = time(NULL);
  for (i = 0; i < MAX_IDENTS_IN_PROGRESS; i++)
  {
    if (idents_in_progress[i].local_port)
    {
      if (now > (idents_in_progress[i].request_time + IDENT_TIMEOUT))
      {
	/* Request has timed out, whether on connect or reply */
	idents_in_progress[i].flags |= IDENT_TIMEDOUT;
	process_result(i);
      }
    }
  }
}


/* Check for requests from the client */

void check_requests(void)
{
  char msgbuf[129];
  int toread;
  int bread;
  static int bufpos = 0;
  int i;

  ioctl(0, FIONREAD, &toread);
  if (toread <= 0)
  {
    return;
  }
  if (toread > 128)
  {
    toread = 128;
  }

  bread = read(0, msgbuf, toread);
  for (i = 0; i < bread;)
  {
    req_buf[bufpos++] = msgbuf[i++];
    if (bufpos == req_size)
    {
      process_request(bufpos);
      bufpos = 0;
    }
  }
}

void process_request(int toread)
{
  int i;
  struct sockaddr_in sockadd;
  short int local_port;
  ident_identifier ident_id;

  for (i = 0; i < toread;)
  {
    switch (req_buf[i++])
    {
      case CLIENT_SEND_REQUEST:
	memcpy(&ident_id, &req_buf[i], sizeof(ident_id));
	i += sizeof(ident_id);
	memcpy(&local_port, &req_buf[i], sizeof(local_port));
	i += sizeof(local_port);
	memcpy(&(sockadd.sin_family), &req_buf[i],
	       sizeof(sockadd.sin_family));
	i += sizeof(sockadd.sin_family);
	memcpy(&(sockadd.sin_addr.s_addr), &req_buf[i],
	       sizeof(sockadd.sin_addr.s_addr));
	i += sizeof(sockadd.sin_addr.s_addr);
	memcpy(&(sockadd.sin_port), &req_buf[i],
	       sizeof(sockadd.sin_port));
	i += sizeof(sockadd.sin_port);

	queue_request(ident_id, local_port, &sockadd);
	break;
      case CLIENT_CANCEL_REQUEST:
	memcpy(&ident_id, &req_buf[i], sizeof(ident_id));
	i += sizeof(ident_id);
	break;
      default:
	break;
    }
  }
}


/* Queue up a request from the client and/or send it off */

void queue_request(ident_identifier id, short int local_port,
		   struct sockaddr_in *sockadd)
{
  ident_request *new_id_req;

  new_id_req = (ident_request *) MALLOC(sizeof(ident_request));
  new_id_req->ident_id = id;
  new_id_req->local_port = local_port;
  new_id_req->next = NULL;
  memcpy(&(new_id_req->sock_addr), sockadd, sizeof(struct sockaddr_in));
  if (!first_inq)
  {
    first_inq = new_id_req;
  }
  else if (!last_inq)
  {
    last_inq = new_id_req;
    first_inq->next = last_inq;
  }
  else
  {
    last_inq->next = new_id_req;
    last_inq = new_id_req;
  }
}

void take_off_queue(int freeslot)
{
  ident_request *old;

  if (first_inq)
  {
    memcpy(&idents_in_progress[freeslot], first_inq, sizeof(ident_request));
    old = first_inq;
    first_inq = first_inq->next;
    FREE(old);
    do_request(&idents_in_progress[freeslot]);
  }
}

void do_request(ident_request * id_req)
{
  int dummy;
  int ret;
  struct sockaddr_in sa;

  if (0 > (id_req->fd = socket(PF_INET, SOCK_STREAM, 0)))
  {
    /* Erk, put on pending queue */
    queue_request(id_req->ident_id, id_req->local_port,
		  &(id_req->sock_addr));
    id_req->local_port = 0;
    return;
  }
  if (ioctl(id_req->fd, FIONBIO, (caddr_t) & dummy) < 0)
  {
    /* Do without? */
  }
  id_req->request_time = time(NULL);
  sa.sin_family = id_req->sock_addr.sin_family;
  sa.sin_addr.s_addr = id_req->sock_addr.sin_addr.s_addr;
  sa.sin_port = htons(IDENT_PORT);
  ret = connect(id_req->fd, (struct sockaddr *) &sa, sizeof(sa));
  if (ret != 0
#if defined(EINPROGRESS)
      && errno != EINPROGRESS
#endif
    )
  {
    if (errno == ECONNREFUSED)
    {
      id_req->flags |= IDENT_CONNREFUSED;
    }
    else
    {
    }
#if defined(EINPROGRESS)
  }
  else if (errno != EINPROGRESS)
#else /* !EINPROGRESS */
  }
  else
#endif
  {
    id_req->flags |= IDENT_CONNECTED;
    write_request(id_req);
  }
}

void write_request(ident_request * id_req)
{
  sprintf(scratch, "%d,%d\n", ntohs(id_req->sock_addr.sin_port),
	  id_req->local_port);
  if (0 > write(id_req->fd, scratch, strlen(scratch)))
  {
    id_req->flags |= IDENT_CONNREFUSED;
  }
  else
  {
    id_req->flags |= IDENT_WRITTEN;
  }

}

/* Get a result from an identd and send it to the client in the
 * form:
 *
 * <SERVER_SEND_REPLY><ident_id><reply text>'\n'
 */

void process_result(int slot)
{
  char reply[BUFFER_SIZE];
  int bread;
  char *s;
  char *t = NULL;
  char *reply_text = NULL;

  s = scratch;
  *s++ = SERVER_SEND_REPLY;
  memcpy(s, &(idents_in_progress[slot].ident_id),
	 sizeof(ident_identifier));
  s += sizeof(ident_identifier);
  strcpy(s, "<ERROR>");
  reply_text = s;
  if (idents_in_progress[slot].flags & IDENT_CONNREFUSED)
  {
    /* Connection was refused */
    strcpy(s, "<CONN REFUSED>");
    t = strchr(s, '\0');
  }
  else if (!(idents_in_progress[slot].flags & IDENT_CONNECTED))
  {
    /* No connection was established */
    strcpy(s, "<CONN FAILED>");
    t = strchr(s, '\0');
  }
  else if (!(idents_in_progress[slot].flags & IDENT_WRITTEN))
  {
    /* Connection made but no message written */
    strcpy(s, "<DIDN'T SEND>");
    t = strchr(s, '\0');
  }
  else if (!(idents_in_progress[slot].flags & IDENT_REPLY_READY))
  {
    /* Request written but didn't get reply before timeout */
    strcpy(s, "<TIMED OUT>");
    t = strchr(s, '\0');
  }
  else
  {
    /* Got a reply, phew! */
    /* BUFFER_SIZE - 20 (== 1004 atm) should be plenty as RFC1413
     * specifies that a USERID reply should not have a user id field
     * of more than 512 octets.
     * Additionally RFC1413 specifies that:
     * "Clients should feel free to abort the connection if they
     *  receive 1000 characters without receiving an <EOL>"
     *
     * NB: <EOL> ::= "015 012"  ; CR-LF End of Line Indicator
     * I assume that under C this will be converted to '\n'
     */
#if defined(HAVE_BZERO)
    bzero(reply, BUFFER_SIZE);
#else /* !HAVE_BZERO */
    memset(reply, 0, BUFFER_SIZE);
#endif /* HAVE_BZERO */
    bread = read(idents_in_progress[slot].fd, reply, BUFFER_SIZE - 20);
    reply[bread] = '\0';

    /* Make sure the reply is '\n' terminated */
    t = strchr(reply, '\n');
    if (t)
    {
      t++;
#if defined(HAVE_BZERO)
      bzero(t, &reply[BUFFER_SIZE] - t - 1);
#else
      memset(t, 0, &reply[BUFFER_SIZE] - t - 1);
#endif /* HAVE_BZERO */
    }
    else
    {
      reply[1001] = '\n';
#if defined(HAVE_BZERO)
      bzero(&reply[1002], BUFFER_SIZE - 1002 - 1);
#else
      memset(&reply[1002], 0, BUFFER_SIZE - 1002 - 1);
#endif /* HAVE_BZERO */
    }

    if (!(t = strstr(reply, "USERID")))
    {
      /* In this case the reply MUST be in the form:

       * <port-pair> : ERROR : <error-type>
       *
       * (see RFC1413, if a later one exists for the ident protocol
       *  then this code should be updated)
       *
       * We will reply to our client with the '<error-type>' text
       *
       */
      if (!(t = strstr(reply, "ERROR")))
      {
	/* Reply doesn't contain 'USERID' *or* 'ERROR' */
	strcpy(s, "<ERROR>");
      }
      else
      {
	t = strchr(t, ':');
	/*
	 * <port-pair> : ERROR : <error-type>
	 *                     ^
	 *             t-------|
	 */
	if (!t)
	{
	  /* Couldn't find the colon after 'ERROR' */
	  strcpy(s, "<ERROR>");
	}
	else
	{
	  *s++ = '<';
	}
      }
    }
    else
    {
      /* Got a reply of the form:

       * <port-pair> : USERID : <opsys-field> : <user-id>
       *               ^
       *         t-----|
       *
       * We will reply to our client with the '<user-id>' text
       *
       */

      t = strchr(t, ':');
      if (!t)
      {
	/* Couldn't find the : after USERID 

	 * <port-pair> : USERID : <opsys-field> : <user-id>
	 *                      ^
	 *           t----------|
	 */
	strcpy(s, "<ERROR>");
      }
      else
      {
	t++;
	t = strchr(t, ':');
	if (!t)
	{
	  /* Couldn't find the : after <opsys-field>

	   * <port-pair> : USERID : <opsys-field> : <user-id>
	   *                                      ^
	   *                           t----------|
	   */
	  strcpy(s, "<ERROR>");
	}
      }
    }
    if (t)
    {
      /* t is now at the : before the field we want to return */
      /* Skip the : */
      t++;
      /* Skip any white space */
      while ((*t == ' ' || *t == '\t') && *t != '\0')
      {
	t++;
      }
      if (*t != '\0')
      {
	/* Found start of the desired text */
	sprintf(s, "%s", t);
	t = strchr(s, '\0');
	t--;
	/* The reply SHOULD be '\n' terminated (RFC1413) */
	while (*t == ' ' || *t == '\t' || *t == '\n' || *t == '\r')
	{
	  t--;
	}
	/* t now at end of text we want */
	/* Move forward to next char */
	t++;
	/* If char before s is a '<' we put it there ourselves to wrap
	 * an 'ERROR' return in <>. So we need to put the > on the
	 * end
	 */
	if (*(s - 1) == '<')
	{
	  *t++ = '>';
	}
      }
      else
      {
	strcpy(s, "<ERROR>");
	t = 0;
      }
    }
  }
  /* Make sure t is pointing to the NULL terminator of the string */
  if (!t)
  {
    t = strchr(s, '\0');
  }
  /* A newline terminates the message */
  *t++ = '\n';
  *t = '\0';
  /* Now actually send the reply to the client */
  write(1, scratch, (t - scratch));
  closedown_request(slot);
}

void closedown_request(int slot)
{
  idents_in_progress[slot].local_port = 0;
  shutdown(idents_in_progress[slot].fd, 2);
  close(idents_in_progress[slot].fd);
}


/* log using stdarg variable length arguments */

void idlog(char *str,...)
{
  va_list args;
  FILE *fp;
  char *fmt;
  struct tm *tim;
  char the_time[] = "xx/xx/xx - xx:xx.xx : ";
  char fname[21];
  time_t current_time;

  va_start(args, str);

  current_time = time(0);
  tim = localtime(&current_time);
  strftime((char *) &the_time, strlen(the_time), "%d/%m/%y - %T : ", tim);
  sprintf(fname, "logs/%s.log", str);

  fp = fopen(fname, "a");
  if (!fp)
  {
    fprintf(stderr, "Eek! Can't open file to log: %s\n", fname);
    return;
  }
  fprintf(fp, "%s", the_time);
  fmt = va_arg(args, char *);
  vfprintf(fp, fmt, args);
  va_end(args);
  fclose(fp);
}

#else
int main(void)
{

  printf("\n Playground+ Ident Server\n"
	 " ------------------------\n\n"
	 "This code is currently disabled. If you wish to have the ident "
	 "included\nyou need to re-run 'make config' and select it.\n\n"
	 "Once selected, the ident is run as part of the talker and should "
	 "not be\nrun on its own.\n\n");
  exit(0);
}
#endif
