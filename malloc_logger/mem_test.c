/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * 
 * You can contact me via email at:
 * 
 * jbuchana@iquest.net          (preferred)
 * c22jrb@dawg.delcoelect.com   (intermediate)
 * jrbuchan@mail.delcoelect.com (disliked, but it's gonna stay around)
 * 
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 *
 * Copyright (C) 1996, 1997, 1998 Jim Buchanan
 *
 * This is free software. You may freely copy it and freely distribute it.
 *
 * You must retain this copyright notice when doing so.
 *
 * Should you decide to distribute it in executable binary form, you must
 * provide the source code, or make it freely available. A pointer to an
 * anonymous ftp site or a web site which offers the source is adequate, but
 * not preferred.
 *
 * When making this software available to others, you may not attempt to limit
 * their redistribution of this software. They must receive it under the terms
 * of this license, as you do.
 *
 * You may include any of the programs, in their entirety, that make up this
 * software package, or the entire package, in any free software package you
 * develop. You must retain this copyright notice in those parts you include.
 *
 * You may include any of the programs, in their entirety, that make up this
 * software package, or the entire package, in a commercial software package or
 * distribution, so long as you include this copyright notice. You may not
 * attempt to limit redistribution of the portions of the finished product
 * which came from this software package.  They must receive it under the terms
 * of this license, as you do.
 *
 * You may, of course, control the distribution of the original portions of
 * your commercial package.
 *
 * Naturally this also means that you may include it on a CDROM (or other
 * media) archive which you distribute commercially. Where would we be without
 * those?
 *
 * You may use portions of this source code in any free software you develop.
 * In fact, it is encouraged. You must, however, retain this copyright notice.
 * You can use any reasonable free licensing scheme you want on your portions
 * of the code. The Free Software Foundation's General Public License would be
 * fine. If in doubt, ask me.
 *
 * If you are developing a commercial software package you may not use portions
 * of this source code smaller than that required to build an executable
 * program.  You may commercially distribute portions of this software package
 * as described above, but they must compromise an entire executable program,
 * or the source code required to build such an entity. If you are interested
 * in any commercial use of portions of this source code, contact me, I'm sure
 * we can arrange something.
 *
 * You may modify this software in any way and distribute it freely under the
 * same terms that you may use portions of it. You must retain this copyright
 * notice, and add your own notice describing the changes made. I'd surely like
 * to see the finished product, but you really don't even have to tell me about
 * it.
 *
 * You may not modify this software in any way other than the above mentioned
 * case of including part of it, down to the level of an individual program, in
 * another package and then distribute it commercially.  If you feel this need,
 * contact me, once again, I'm sure we can work things out.
 *
 * I reserve the right to use any portion of this software package, or future
 * upgrades to the package as a whole, in a commercial product of my own,
 * license it out to to others for commercial purposes, or to base future
 * commercial products on it.
 *
 * But I probably won't. Let's get real here. I wrote this for fun.
 *
 * Any future use of this software for commercial gains does not affect your
 * right to copy and freely distribute this version. This license ensures that
 * this version of the software package may be freely distributed and will
 * remain so.
 *
 * It does not guarantee that future versions will remain so. But they probably
 * will.
 *
 * There is no warranty provided with this free software. No fitness for any
 * purpose is claimed or implied. It might not work correctly. It might not
 * work at all. It might do something really unpleasant. The risk is yours.
 * You accept it by installing and running this software. Under no
 * circumstances are the author, any modifiers, or any distributors of this
 * software liable for any damages arising out of the use or inability to use
 * this software.
 *
 * Have a nice day.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <errno.h>
#include <string.h>
#include "mem_test.h"

#ifdef SUNOS
char *listrerror (int errnum);
#endif

#ifdef SUNOS
extern char *sys_errlist[];        /* used by listrerror() */
extern int sys_nerr;               /* used by listrerror() */
#endif

FILE *MEM_FILE = NULL;

#ifdef SUNOS
#define strerror listrerror
#endif

void *limalloc (unsigned int size, char *file_name, int line_number)
	{
	void *ptr;

        if (! MEM_FILE)
            MEM_FILE = fopen (getmemfilename (), "w");

	ptr = malloc (size);

	fprintf (MEM_FILE, "limalloc:%u:%d:%s:%d\n", (unsigned) ptr, size, file_name, line_number);
        fflush (MEM_FILE);

	if (! ptr)
		{
		fprintf (MEM_FILE, "# ***ERROR*** malloc() was not sucessful.\n");
                fflush (MEM_FILE);
                fprintf (MEM_FILE, "# Line %d of '%s'\n", line_number, file_name);
                fflush (MEM_FILE);
		fprintf (MEM_FILE, "# '%s'\n", strerror (errno));
                fflush (MEM_FILE);
		}

	return (ptr);
	
	}

void *licalloc (unsigned int nelem, unsigned int size, char *file_name, int line_number)
	{
	void *ptr;

        if (! MEM_FILE)
            MEM_FILE = fopen (getmemfilename (), "w");

	ptr = calloc (nelem, size);

	fprintf (MEM_FILE, "licalloc:%u:%d:%s:%d\n", (unsigned) ptr, size * nelem, file_name, line_number);
        fflush (MEM_FILE);

	if (! ptr)
		{
		fprintf (MEM_FILE, "# ***ERROR*** calloc() was not sucessful.\n");
                fflush (MEM_FILE);
                fprintf (MEM_FILE, "# Line %d of '%s'\n", line_number, file_name);
                fflush (MEM_FILE);
		fprintf (MEM_FILE, "# '%s'\n", strerror (errno));
                fflush (MEM_FILE);
		}

	return (ptr);
	
	}

void *lirealloc (void *in_ptr, unsigned int size, char *file_name, int line_number)
	{
	void *out_ptr;

        if (! MEM_FILE)
            MEM_FILE = fopen (getmemfilename (), "w");

	out_ptr = realloc (in_ptr, size);

	fprintf (MEM_FILE, "lirealloc:%u:%d:%s:%d\n", (unsigned) in_ptr, 0, file_name, line_number);
	fprintf (MEM_FILE, "lirealloc:%u:%d:%s:%d\n", (unsigned) out_ptr, size, file_name, line_number);
        fflush (MEM_FILE);

	if (! out_ptr)
		{
		fprintf (MEM_FILE, "# ***ERROR*** realloc() was not sucessful.\n");
                fflush (MEM_FILE);
                fprintf (MEM_FILE, "# Line %d of '%s'\n", line_number, file_name);
                fflush (MEM_FILE);
		fprintf (MEM_FILE, "# '%s'\n", strerror(errno));
                fflush (MEM_FILE);
		}


	return (out_ptr);
	
	}

void lifree (void *ptr, char *file_name, int line_number)
	{

        if (! MEM_FILE)
            MEM_FILE = fopen (getmemfilename (), "w");

	free (ptr);

	fprintf (MEM_FILE, "lifree:%u:0:%s:%d\n", (unsigned) ptr, file_name, line_number);
        fflush (MEM_FILE);


	}

void licfree (void *ptr, char *file_name, int line_number)
	{

        if (! MEM_FILE)
            MEM_FILE = fopen (getmemfilename (), "w");

	cfree (ptr);

	fprintf (MEM_FILE, "licfree:%u:0:%s:%d\n", (unsigned) ptr, file_name, line_number);
        fflush (MEM_FILE);


	}

void *listrdup (char *s1, char *file_name, int line_number)
	{
	void *ptr;

        if (! MEM_FILE)
            MEM_FILE = fopen (getmemfilename (), "w");

	ptr = (char *) strdup (s1);

	fprintf (MEM_FILE, "listrdup:%u:%u:%s:%d\n", (unsigned) ptr, (unsigned) strlen (s1) + 1, file_name, line_number);
        fflush (MEM_FILE);

	if (! ptr)
		{
		fprintf (MEM_FILE, "# ***ERROR*** strdup() was not sucessful.\n");
                fflush (MEM_FILE);
                fprintf (MEM_FILE, "# Line %d of '%s'\n", line_number, file_name);
                fflush (MEM_FILE);
		fprintf (MEM_FILE, "# '%s'\n", strerror(errno));
                fflush (MEM_FILE);
		}


	return (ptr);

	}

char *getmemfilename (void)
    {
    char *file_name;

    file_name = getenv ("MEM_FILE");

    if (! file_name)
        {
        file_name = "MEM_TEST_FILE";
        }

    return (file_name);

    }

/* SunOS 4.1.2 does not seem to have strerror(),
* We'll use our own. */
#ifdef SUNOS
char *listrerror (int errnum)
  {

  if (errnum < sys_nerr)
      {
      return (sys_errlist[errnum]);
      }
  else
      {
      return ("Unknown error");
      }

  }
#endif
