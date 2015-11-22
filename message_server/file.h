/*
 * file.h
 *
 * $Id: file.h,v 1.2 1996/03/28 20:02:07 athan Exp $
 *
 * $Log: file.h,v $
 * Revision 1.2  1996/03/28 20:02:07  athan
 * General cleanups
 *
 *
 */

#include <stdio.h>

struct afile {
   char *what;
   size_t len;
};
