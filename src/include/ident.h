/*
 * ident.h
 *
 * Header file for ident server code.
 *
 */

/*
 The majority of the code contained herein is Copyright 1995/1996 by
 Neil Peter Charley.
 Portions of the code (notably that for Solaris 2.x and BSD compatibility)
 are Copyright 1996 James William Hawtin. Thanks also goes to James
 for pointing out ways to make the code more robust.
 ALL code contained herein is covered by one or the other of the above
 Copyright's.

 Permission to use this code authored by the above persons is given
 provided:

   i) Full credit is given to the author(s).

  ii) The code is not re-distributed in ANY form to persons other than
     yourself without the express permission of the author(s).
     (i.e. you can transfer it from account to account as long as it is
      to yourself)
       NB: Special permission has been given to the maintainer of the package
          known as 'SensiSummink' AND to the maintainer of the package
	  known as 'GPA' (later version of PG96) to make use of this code
	  in that.  Anyone using this package automatically gets the right
	  to USE this code.
          You are *NOT*, however, given permission to redistribute it in
          other forms other than passing on the 'SensiSummink' or
	  'GPA' package that you yourself recieved.

 iii) You make EVERY effort to forward bugfixes to the author(s)
     NB: Neil is co-ordinating any bugfixes that go into this program, so
         please try and send them to him first.
 */

/* $Revision: 1.1 $ */

#if !defined(IDENT_SERVERH)
 #define IDENT_SERVERH

#include <time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#if defined(sun)
 #if defined(SVR4)
  #if !defined(SOLARIS)
   #define SOLARIS
   #define NOALARM
   #define NONBLOCKING
   /* Signal configation */
   #define USE_SIGACTION
   #define USE_SIGEMPTYSET
   #define SIGTYPE int sigtype
  #endif /* !SOLARIS */
 #else /* sun && !SVR4 (SunOS 4.x presumeably) */
  #define BSD
  #define SIGTYPE int sigtype, int code,  struct sigcontext *scp, char *addr
  #define HAVE_BCOPY
  #define HAVE_BZERO
 #endif /* SVR4 */
#endif /* sun */

#ifdef LINUX
 #define SIGTYPE int it
 #define USE_SIGACTION
#endif

/* The various fd's for the two pipes between client and server */

#define IDENT_SERVER_READ ident_toserver_fds[0]
#define IDENT_SERVER_WRITE ident_toclient_fds[1]
#define IDENT_CLIENT_READ ident_toclient_fds[0]
#define IDENT_CLIENT_WRITE ident_toserver_fds[1]

/* Messages between client and server */

#define SERVER_CONNECT_MSG "Server Connect:"
#define CLIENT_CONNECT_MSG "Client Connect:"
#define CLIENT_SEND_REQUEST 'A'
#define CLIENT_CANCEL_REQUEST 'B'
#define SERVER_SEND_REPLY ':'

/* Misc #define's */

#ifdef RESORT
 #define MAX_IDENTS_IN_PROGRESS 70
#else
 #define MAX_IDENTS_IN_PROGRESS 20
#endif
#define IDENT_TIMEOUT 60
#define IDENT_NOTYET "<NOT YET>"
#define IDENT_PORT 113
#define AWAITING_IDENT -1

/* Typedef's */

typedef int ident_identifier;

/* Struct for an ident request */

struct s_ident_request {
	ident_identifier ident_id;
	int local_port;
	struct sockaddr_in sock_addr;
	struct sockaddr_in local_sockaddr;
	int fd;
	int flags;
	time_t request_time;
	struct s_ident_request *next;
};
typedef struct s_ident_request ident_request;

/* Flags on an ident request */

#define IDENT_CONNECTED   (1 << 0)   /* Connection established */
#define IDENT_WRITTEN     (1 << 1)   /* Request written */
#define IDENT_REPLY_READY (1 << 2)   /* Reply available */
#define IDENT_CONNREFUSED (1 << 3)   /* Connection was refused */
#define IDENT_TIMEDOUT    (1 << 4)   /* Timed out before reply */

#endif /* IDENT_SERVERH */
