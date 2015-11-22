/*
 * Playground+ - xstring.c
 * Xtra string related functions (c) phypor 1998
 * ---------------------------------------------------------------------------
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* this is the only thing we need from the talker, so instead of including
   all the files and shat, we just extern this
 */
extern void lower_case(char *);
extern char *end_string(char *);
extern char *stack;


/*
   like strstr, but case insenstive
 */
char *strcasestr(char *haystack, char *needle)
{
  char s[5], *nptr, *hptr;

  if (!needle || !*needle)
    return (char *) NULL;

  memset(s, 0, 5);
  strncpy(s, needle, 4);
  lower_case(s);

  while (*haystack)
  {
    if (tolower(*haystack) == s[0])
    {
      if (!s[1])
	return haystack;
      if (*(haystack + 1) && tolower(*(haystack + 1)) == s[1])
      {
	if (!s[2])
	  return haystack;
	if (*(haystack + 2) && tolower(*(haystack + 2)) == s[2])
	{
	  if (!s[3])
	    return haystack;
	  if (*(haystack + 3) && tolower(*(haystack + 3)) == s[3])
	  {
	    hptr = haystack + 4;
	    nptr = needle + 4;
	    for (; *nptr && *hptr; nptr++, hptr++)
	      if (tolower(*nptr) != tolower(*hptr))
		break;
	    if (!*nptr)
	      return haystack;
	  }
	}
      }
    }
    haystack++;
  }
  /* strstr returns NULL, not an actual char that is NULL...
     so we will follow its way
   */
  return (char *) NULL;
}


/*
   given a string to search in (haystack),
   a list of search strings new line terminated (needles),
   check each line seperately to see if it occurs,
   case insensitive
   returns a pointer to occurance in haystack
 */

char *strcaseline(char *haystack, char *needles)
{
  char *lf = needles, *re;
  char ln[160];
  int i = 0;

  if (!lf || !*lf)
    return (char *) NULL;

  for (memset(ln, 0, 160); *lf; memset(ln, 0, 160), i = 0)
  {
    while (*lf && *lf != '\n' && i < 158)
      ln[i++] = *lf++;
    if ((re = strcasestr(haystack, ln)))
      return re;
    if (*lf == '\n')
      lf++;
  }
  return (char *) NULL;
}

/*
   given a string to search in (haystack),
   a list of search strings new line terminated (needles),
   check each line seperately to see if it occurs,
   case sensitive
   returns a pointer to occurance in haystack
 */

char *strline(char *haystack, char *needles)
{
  char *lf = needles, *re;
  char ln[160];
  int i = 0;

  if (!lf || !*lf)
    return (char *) NULL;

  for (memset(ln, 0, 160); *lf; memset(ln, 0, 160), i = 0)
  {
    while (*lf && *lf != '\n' && i < 158)
      ln[i++] = *lf++;
    if ((re = strstr(haystack, ln)))
      return re;
    if (*lf == '\n')
      lf++;
  }
  return (char *) NULL;
}

/*
   the large static vars appear to be a kludge,
   however in this peticular instance they are
   the most effective means to get it done

   overflow checking could be better, but
   buffers are so large, there shouldnt be any
   way to overwrite...(watch out for possible
   recursive replaces tho, those are killers)
 */

char *single_replace(char *str, char *find, char *rep)
{
  static char buf[51200];
  char *ptr;
  int i = 0;
  int m = 0;
  int n = 0;
  int c = 0;

  if (strlen(str) > 38400)
    return str;

  if (!(ptr = strstr(str, find)))
    return str;

  memset(buf, 0, 51200);

  c = strlen(str) - strlen(ptr);

  for (i = 0; i < c; i++)
    buf[i] = str[i];

  for (n = 0, m = i; rep[n]; n++, m++)
    buf[m] = rep[n];

  for (n = 0; n < strlen(find); n++)
    i++;

  for (; str[i]; m++, i++)
    buf[m] = str[i];

  return buf;
}


char *replace(char *str, char *find, char *rep)
{
  static char returnstr[51200];

  if (strlen(str) > 38400)
    return str;

  if (strstr(str, find))
  {
    /* seed */
    strncpy(returnstr, single_replace(str, find, rep), 51100);

    /* recursion */
    while (strstr(returnstr, find))
      strncpy(returnstr, single_replace(returnstr, find, rep), 51100);

    return returnstr;
  }
  return str;

}



/* given a multiple strings to check the first of each line  (haystacks),
   a string for which to search (needle),
   return where needle is in the haystacks
 */

char *linestr(char *haystacks, char *needle)
{
  char *ptr;

  if (!haystacks || !*haystacks)
    return (char *) NULL;

  ptr = haystacks;
  while ((ptr = strcasestr(ptr, needle)))
    if (ptr == haystacks || *(ptr - 1) == '\n')
      return ptr;
    else
      ptr++;

  return (char *) NULL;
}


/*
   given a string of lines with
   identifier1:        value
   identifier2:        value
   identifiern:        value

   given an identifier ...

   return the value assoicated with the identifer
 */


char *lineval(char *where, char *identifier)
{
  char *oldstack = stack, *line;
  static char buf[1024];

  sprintf(stack, "%s:", identifier);
  stack = end_string(stack);
  line = linestr(where, oldstack);
  stack = oldstack;


  if (!line || !*line)
    return (char *) NULL;

  memset(buf, 0, 1024);

  while (*line && *line++ != ':');

  while (*line && (*line == ' ' || *line == '\t'))
    line++;

  strncpy(buf, line, 1023);
  line = buf;

  while (!NULL)
  {
    while (*line && *line != '\n')	/* dont do it all in one line so we */
      line++;			/* dont finish ahead of the \n      */

    if (*line && *(line + 1) == '\t')
    {
      line++;			/* past the \n */
      while (*line == '\t')	/* turn tabs into spaces */
	*line++ = ' ';
    }
    else
      break;
  }
  *line = '\0';

  if (*(line - 1) == ' ')	/* a blank line? */
  {
    line--;
    while (*line == ' ')
      line--;
    if (*line == '\n')
      *++line = '\0';
  }

  return buf;
}
