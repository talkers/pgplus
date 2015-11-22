/*
 * socket.c
 *
 * $Id: socket.c,v 1.3 1996/03/29 16:51:01 athan Exp $
 *
 * $Log: socket.c,v $
 * Revision 1.3  1996/03/29 16:51:01  athan
 * General cleanup
 *
 * Revision 1.2  1996/03/28 20:02:07  athan
 * General cleanups
 *
 *
 */

#include <malloc.h>
#include <sys/types.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#if defined(SOLARIS)
#include <sys/filio.h>
#endif /* SOLARIS */

#include "config.h"

#define IPROTO   0
#define MAX_QUEUE   5

/* Extern Function Prototypes */

#if defined(SOLARIS)
extern int gethostname(char *name, int namelen);
#endif

/* Extern Variables */

extern int status;
extern int verbose;

/* Local Function Prototypes */

int accept_connection(void);
int open_main_socket(int port);
void shut_down(void);

/* Local Variables */

int mainsock;

int open_main_socket(int port)
{
  struct sockaddr_in sa;
  char *hostname;
  struct hostent *hp;
  int dummy = 1;

  hostname = (char *) malloc(101);
  memset((char *) &sa, 0, sizeof(struct sockaddr_in));
  gethostname(hostname, 100);

  hp = gethostbyname(hostname);
  if (hp == NULL)
  {
    fprintf(stderr, "Error: Host machine does not exist!\n");
    return -1;
  }

  sa.sin_family = hp->h_addrtype;
  sa.sin_port = htons(port);

  if (-1 == (mainsock = socket(AF_INET, SOCK_STREAM, 0)))
  {
    fprintf(stderr, "Couldn't open main socket!\n");
    return -1;
  }

  if (-1 == setsockopt(mainsock, SOL_SOCKET, SO_REUSEADDR, (char *) &dummy,
		       sizeof(dummy)))
  {
    fprintf(stderr, "Failed setsockopt\n");
    return -1;
  }

  if (-1 == ioctl(mainsock, (int) FIONBIO, (caddr_t) & dummy))
  {
    fprintf(stderr, "Couldn't set to non-blocking\n");
    return -1;
  }

  if (-1 == (bind(mainsock, (struct sockaddr *) &sa, sizeof(sa))))
  {
    close(mainsock);
    return -1;
  }

  if (listen(mainsock, MAX_QUEUE) < 0)
  {
    fprintf(stderr, "Listen refused\n");
    return -1;
  }

  return 0;
}

/* Accept a new connection */

int accept_connection(void)
{
  struct sockaddr_in add;
  int lengthadd;
  int dummy;
  int this_sock;

  lengthadd = sizeof(add);

  getsockname(mainsock, (struct sockaddr *) &add, &lengthadd);

  if (-1 == (this_sock = accept(mainsock, (struct sockaddr *) &add,
				&lengthadd)))
  {
    return -1;
  }

  if (-1 == ioctl(this_sock, (int) FIONBIO, (caddr_t) & dummy))
  {
    close(this_sock);
    return -1;
  }

  return this_sock;
}


void shut_down(void)
{
  if (verbose)
  {
    fprintf(stderr, "Shutting down NOW!\n");
  }
  shutdown(mainsock, 2);
  close(mainsock);
}
