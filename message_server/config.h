/*
 * config.h
 *
 * $Id: config.h,v 1.3 1996/03/29 16:51:01 athan Exp $
 *
 * $Log: config.h,v $
 * Revision 1.3  1996/03/29 16:51:01  athan
 * All defaults are now in here
 *
 * Revision 1.2  1996/03/28 20:02:07  athan
 * General cleanups
 *
 *
 */

/* status of program */
#define	INIT		0
#define	RUNNING		1
#define	QUIT		2
#define	SHUTDOWN	3

#define DEFAULT_MSG_FILE	"MESSAGE"
#define DEFAULT_PORT	 	3232
#define DEFAULT_TIMEOUT		10
#define MAX_CONNECTIONS		64
