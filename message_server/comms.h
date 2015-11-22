/*
 * comms.h
 *
 * $Id: comms.h,v 1.2 1996/03/28 20:02:07 athan Exp $
 *
 * $Log: comms.h,v $
 * Revision 1.2  1996/03/28 20:02:07  athan
 * General cleanups
 *
 *
 */

struct connection {
   int fd;
   time_t login_time;
   long count;
   struct connection *prev;
   struct connection *next;
};
