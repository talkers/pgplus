/*
 * Playground+ - compaction.c
 * Nasty compaction tables and stuff - keep well away!
 * ---------------------------------------------------------------------------
 */

#include <sys/types.h>
#include <netinet/in.h>

char uncompact_table[7][16] =
{
  {0, ' ', '\n', 'a', 'e', 'h', 'i', 'n', 'o', 's', 't', 1, 2, 3, 4, 5},
  {'b', 'c', 'd', 'f', 'g', 'j', 'k', 'l', 'm', 'p', 'q', 'r', 'u', 'v', 'w', 'x'},
  {'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N'},
  {'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '!', '"', '#', '@'},
  {'~', '&', 39, '(', ')', '*', '+', ',', '-', '.', '/', '0', '1', '2', '3', '4'},
  {'5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?', '[', '\\', ']', '^', 6},
  {'`', '_', '{', '|', '}', '%', '$', -1, 0, 0, 0, 0, 0, 0, 0, 0}};

char compact_table[128] =
{
  1, 60, 61, 62, 102, 101, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
  81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 63, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46,
  47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 91, 92, 93, 94, 97, 96, 3, 16, 17, 18, 4,
  19, 20, 5, 6, 21, 22, 23, 24, 7, 8, 25, 26, 27, 9, 10, 28, 29, 30, 31, 32, 33, 98, 99, 100, 64
};


/* stores a nibble */

char *store_nibble(char *dest, int n)
{
  static int toggle = 0;
  if (toggle)
  {
    *dest++ |= (char) n;
    toggle = 0;
  }
  else
  {
    if (!n)
    {
      *dest = 0;
      dest++;
      return dest;
    }
    n <<= 4;
    *dest = (char) n;
    toggle = 1;
  }
  return dest;
}

/* retrieves a nibble */

char *get_nibble(int *n, char *source)
{
  static int toggle = 0;
  if (toggle)
  {
    *n = ((int) *source++) & 15;
    toggle = 0;
  }
  else
  {
    *n = (((int) *source) >> 4) & 15;
    if (!*n)
      source++;
    else
      toggle = 1;
  }
  return source;
}

/* returns new source */

char *get_string(char *dest, char *source)
{
  char c = 1;
  int n = 0, table = 0;

  for (; c; table = 0)
  {
    c = 1;
    while (c && c < 7)
    {
      source = get_nibble(&n, source);
      c = uncompact_table[table][n];
      if (c && c < 7)
	table = (int) c;
    }
    *dest++ = c;
  }
  return source;
}

/* returns new destination */

char *store_string(dest, source)
     char *dest, *source;
{
  int n, row, tmp = 1;
  for (; tmp; source++)
  {
    tmp = (int) *source;
    switch (tmp)
    {
      case 0:
	row = 0;
	n = 0;
	break;
      case '\n':
	row = 0;
	n = 2;
	break;
      case -1:
	row = 6;
	n = 7;
	break;
      default:
	row = (compact_table[tmp - 32]) >> 4;
	n = (compact_table[tmp - 32]) & 15;
	break;
    }
    switch (row)
    {
      case 0:
	dest = store_nibble(dest, n);
	break;
      case 1:
	dest = store_nibble(dest, 11);
	dest = store_nibble(dest, n);
	break;
      case 2:
	dest = store_nibble(dest, 12);
	dest = store_nibble(dest, n);
	break;
      case 3:
	dest = store_nibble(dest, 13);
	dest = store_nibble(dest, n);
	break;
      case 4:
	dest = store_nibble(dest, 14);
	dest = store_nibble(dest, n);
	break;
      case 5:
	dest = store_nibble(dest, 15);
	dest = store_nibble(dest, n);
	break;
      case 6:
	dest = store_nibble(dest, 15);
	dest = store_nibble(dest, 15);
	dest = store_nibble(dest, n);
	break;
    }
  }
  return dest;
}

/* store 2-byte word in player file */

char *store_word(dest, source)
     char *dest;
     int source;
{
  *dest++ = (source >> 8) & 255;
  *dest++ = source & 255;
  return dest;
}

/* store 4-byte word in player file */

char *store_int(dest, source)
     char *dest;
     int source;
{
  int i;
  union
  {
    char c[4];
    int i;
  }
  u;
  u.i = htonl(source);
  for (i = 0; i < 4; i++)
    *dest++ = u.c[i];
  return dest;
}

/* retrieve 2-byte word from player file */

char *get_word(dest, source)
     char *source;
     int *dest;
{
  union
  {
    char c[4];
    int i;
  }
  u;
  u.i = 0;
  u.c[3] = *source++;
  u.c[4] = *source++;
  *dest = ntohl(u.i);
  return source;
}

/* retrieve 4-byte int from player file */

char *get_int(dest, source)
     char *source;
     int *dest;
{
  int i;
  union
  {
    char c[4];
    int i;
  }
  u;
  for (i = 0; i < 4; i++)
    u.c[i] = *source++;
  *dest = ntohl(u.i);
  return source;
}

/* get int safe - written by Oolon.
   Allows easy addition to the playerfiles and provides some
   protection against file corruptions */

char *get_int_safe(int *no, char *r, file data)
{
  if ((no) && (r))
    if (((int) r - (int) data.where) < data.length)
      return get_int(no, r);
    else
    {
      *no = 0;
      return r;
    }
  else
  {
    if (no)
      *no = 0;
    return r;
  }
}

/* get string safe - as above, but for string types :-) */
char *get_string_safe(char *str, char *r, file data)
{
  if ((str) && (r))
    if (((int) r - (int) data.where) < data.length)
      return get_string(str, r);
    else
    {
      *str = 0;
      return r;
    }
  else
  {
    if (str)
      *str = 0;
    return r;
  }
}

/* get_int_safe_explicit for the loading of items - thanks to oolon! */

char *get_int_safe_explicit(int *number, char *compressed_data,
		       const char *compressed_start, size_t compress_length)
{
  if ((number) && (compressed_data))
    if (((size_t) compressed_data - (size_t) compressed_start) < compress_length)
      return get_int(number, compressed_data);
    else
    {
      *number = 0;
      return compressed_data;
    }
  else
  {
    if (number)
      *number = 0;
    return compressed_data;
  }
}
