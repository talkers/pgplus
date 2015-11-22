/*
 * file.c
 *
 * $Id: file.c,v 1.3 1996/03/29 16:51:01 athan Exp $
 *
 * $Log: file.c,v $
 * Revision 1.3  1996/03/29 16:51:01  athan
 * Cleanups
 * Fixed bug with free[A()'ing wrong variable
 *
 * Revision 1.2  1996/03/28 20:02:07  athan
 * General cleanups
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <fcntl.h>

#include "file.h"

/* Extern Function Prototypes */

/* Extern Variables */

extern int verbose;
extern struct afile thefile;

/* Local Function Prototypes */

void load_file(char *fn, struct afile *it);

/* Local Variables */


/* Load a file. Not quite as simple as it seems, seeing as we convert
 * any '\n' we find to '\n\r'. To achieve this we load it into one buffer,
 * which is the size of the file read, then copy it into a second buffer
 * a character at a time. This second buffer is twice the length of the
 * first to  allow for the worse case scenario of the file being completely
 * full of '\n' characters.
 * Once this is achieved we allocate memory for the ACTUAL size of the
 * processed file, and copy it into this buffer, pointed to by the 'what'
 * part of the it struct passed to us.
 */

void load_file(char *fn, struct afile *it)
{
  int d;
  int len;
  char *buf, *startbuf;
  char *processed, *startprocessed;

  d = open(fn, O_RDONLY);
  if (d < 0)
  {
    fprintf(stderr, "Couldn't open file \'%s\'\n", fn);
    exit(-1);
  }
  len = lseek(d, 0, SEEK_END);
  lseek(d, 0, SEEK_SET);
  if (NULL == (startbuf = buf = (char *) malloc(len + 1)))
  {
    fprintf(stderr, "Failed to malloc(%d)\n", len + 1);
    exit(-1);
  }
  memset(buf, 0, len + 1);
  if (NULL == (startprocessed = processed = (char *) malloc((2 * len) + 1)))
  {
    fprintf(stderr, "Failed to malloc(%d)\n", (2 * len) + 1);
    exit(-1);
  }
  memset(processed, 0, (2 * len) + 1);
  if (-1 == (read(d, buf, len)))
  {
    fprintf(stderr, "Error reading file \'%s\'\n", fn);
    exit(-1);
  }
  close(d);
  while (*buf)
  {
    switch (*buf)
    {
    case '\n':
      *processed++ = *buf++;
      *processed++ = '\r';
      break;
    default:
      *processed++ = *buf++;
    }
  }
  free(startbuf);
  if (it->what)
  {
    free(it->what);
  }
  it->len = (size_t) (processed - startprocessed);
  if (NULL == (it->what = (char *) malloc(it->len + 1)))
  {
    fprintf(stderr, "Failed to malloc(%d)\n", it->len + 1);
    exit(-1);
  }
  memcpy(it->what, startprocessed, it->len);
  it->what[it->len] = '\0';
  free(startprocessed);
}
