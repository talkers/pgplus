/*
 * comms.c
 *
 * $Id: comms.c,v 1.3 1996/03/29 16:51:01 athan Exp $
 *
 * $Log: comms.c,v $
 * Revision 1.3  1996/03/29 16:51:01  athan
 * Reformatting
 * Moved #define's to config.h
 * Now use MAX_CO[3~NNECTIONS not hard-coded 64
 *
 * Revision 1.2  1996/03/28 20:02:07  athan
 * General cleanups
 *
 *
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <malloc.h>
#include <sys/socket.h>
#include <unistd.h>
#if defined(SOLARIS)
#include <sys/filio.h>
#endif /* SOLARIS */

#include "comms.h"
#include "config.h"
#include "file.h"


/* Extern Function Prototypes */

/* Extern Variables */

extern struct afile thefile;
extern int timeout;

/* Local Function Prototypes */

int check_timeout(long thistime, int *count);
void closec(struct connection *c);
void do_login(int newfd, long ltime);
void init_comms(void);
void write_all(char *s);

/* Local Variables */

int mainsock;
struct connection *firstc;
int cons = 0;
char buffer[4096];

const char fullmsg[] = "Connections full, sorry!\n";

void init_comms(void)
{
  firstc = NULL;
}

void do_login(int newfd, long ltime)
{
  struct connection *newc, *tc = firstc;

  if (cons == MAX_CONNECTIONS)
  {
    write(newfd, fullmsg, strlen(fullmsg));
    shutdown(newfd, 2);
    close(newfd);
  }
  if (tc != NULL)
  {
    while (tc->next != NULL)
    {
      tc = tc->next;
    }
  }
  if (NULL == (newc = (struct connection *) malloc(sizeof(struct connection))))
  {
    write(newfd, fullmsg, strlen(fullmsg));
    shutdown(newfd, 2);
    close(newfd);
  }
  if (firstc != NULL)
  {
    tc->next = newc;
  }
  else
  {
    firstc = newc;
  }
  if (firstc != tc)
  {
    newc->prev = tc;
  }
  else
  {
    newc->prev = NULL;
  }
  newc->next = NULL;
  newc->fd = newfd;
  newc->login_time = ltime;
  newc->count = ltime;
  cons++;

  write(newfd, thefile.what, thefile.len);
}

/* Check all current connections for timeout and act upon it if
   necessary */

int check_timeout(long thistime, int *count)
{
  struct connection *i, *n;
  char cnt[64];
  long toread;

  if (firstc != NULL)
  {
    for (i = firstc; i != NULL; i = n)
    {
      n = i->next;
      if (-1 == ioctl(i->fd, FIONREAD, (caddr_t) & toread))
      {
	closec(i);
	count--;
      }
      else if ((i->login_time + timeout + 1) <= thistime)
      {
	closec(i);
	count--;
      }
      else if ((thistime - i->count) > 0)
      {
	sprintf(cnt, "%2d ", (int) (timeout - (i->count - i->login_time)));
	write(i->fd, &cnt, strlen(cnt));
	i->count = thistime;
      }
    }
  }
  return 0;
}

void closec(struct connection *c)
{
  if (c->next != NULL)
  {
    c->next->prev = c->prev;
  }
  if (c->prev != NULL)
  {
    c->prev->next = c->next;
  }
  if (c == firstc)
  {
    firstc = firstc->next;
  }
  write(c->fd, "  Bye!\n", 7);
  shutdown(c->fd, 2);
  close(c->fd);
  free(c);
  cons--;
}
