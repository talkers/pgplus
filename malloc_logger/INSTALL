
 Jim Buchanan
 jbuchana@iquest.net           (preferred)
 c22jrb@dawg.delcoelect.com   (intermediate)
 jrbuchan@mail.delcoelect.com (disliked, but it's gonna stay around)

First, edit the Makefile. 

> # Uncomment this line if you are using SunOS 4.x, or if
> # you are running another OS that does not provide a
> # strerror() function
> #OSFLAG = -DSUNOS

As the comment says, uncomment the line if you are using SunOS 4.x. If
you are running some other operating system that does not provide an
strerror() function, try this as well. One sign might be compile
errors which complain about an argument not being a pointer.

> # This is where your library file will go
> LIBDIR = /usr/lib

This is where the actual libmem_test.a library goes.

If you are going to be installing this for general use by all users of
the machine, you'll probably want to leave this as is. You'll need to
su to root to do that. If you are going to install mem_test under your
home directory, you'll probably want to use something along the lines
of:

> LIBDIR = /home/your_account/lib

No need to be root this way either. You will have to manually create
this directory before running make if you go this way.

> # This is where your header file will go
> INCDIR = /usr/include

This is where the header file that you will include when using
mem_test goes.

Once again, for a general installation, you'll probably want to leave
this where it is. For an installation under your home directory,
something like this should do:

> INCDIR = /home/your_account/include

You will have to manually create this directory before running make if
you go this way.

> # This is where the executables go
> BINDIR = /usr/local/bin

This is where mem_analyze, which is used to analyze the log file
produced by the program linked with libmem_test, will live.

You could make an argument for having this at /usr/bin. Do what you
wish.

For a non-privileged install, try something like:

> BINDIR = /home/your_account/bin

Once again, you'll have to create this directory manually before
running make if it does not exist.

> # This is where the man pages go
> MANDIR = /usr/man

This is where the man pages for mem_test, libmem_test (hard links to
the same file), and mem_analyze go.

For a non-privileged install, something like this should do:

> MANDIR = /home/your_account/man

Don't forget to create the dir first...

> # This is your C compiler
> CC = gcc

Although I've compiled and used mem_test under SunOS, HPUX and Linux,
I've only used gcc to compile it. I suspect that most any decent
compiler should work though...

> # Switches for the compiler, use -g for debugging
> # -Wno-implicit prevents a lot of irritating errors
> # under SunOS 4.1.2
> CFLAGS = -Wall -Wno-implicit -g
> #CFLAGS = -Wall -Wno-implicit

Unlike most programs, I suggest leaving the -g option in the
production version. It is after all only going to be linked with
programs being debugged...

Once the Makefile has been edited, make the library.

> jbuchana$ make
> gcc -Wall -Wno-implicit -g -c mem_test.c
> rm libmemtest.a
> rm: libmemtest.a: No such file or directory
> make: [mem_test] Error 1 (ignored)
> ar r libmem_test.a mem_test.o
> jbuchana$

If you get an error followed by "(ignored)", do just that, it's
normal.

Now look at the first line of mem_analyze. It's a perl script, and the
first line should contain the path to your perl interpretor.

Something like this:

> #!/usr/bin/perl -w

Find the location of your perl interpretor with the which command:

> jbuchana$ which perl
> /usr/bin/perl

Edit the first line of mem_analyze to reflect this location.

Then install mem_test. If you are installing mem_test for general use
(as opposed to installing it under your home directory), you'll have
to su to root first.

> [root@zaphod mem_test]# make install
> cp libmem_test.a /usr/lib
> ranlib /usr/lib/libmem_test.a
> cp mem_test_user.h /usr/include
> cp mem_analyze /usr/local/bin
> cp mem_analyze.1 /usr/man/man1/mem_analyze.1
> cp libmem_test.3 /usr/man/man3/libmem_test.3
> ln /usr/man/man3/libmem_test.3 /usr/man/man3/mem_test.3
> [root@zaphod mem_test]#

Note that under HPUX (9.05, at least), ranlib will complain that ar
already did its job. It even gets snotty and suggests that we read the
man pages. If we leave ranlib out, SunOS will whine whenever we link
with the library. It works fine under HPUX this way, despite the
irritating message, so I've been taking the simple way out. I've just
been ignoring the message.

To compile the example program, and link with libmem_test, use the
target example_program. This shows how to link with libmem_test, and
it has some easy to locate memory leaks as examples.

Example:

> jbuchana$ make example_program
> gcc -Wall -Wno-implicit -g -c example_program.c
> gcc -Wall -Wno-implicit -g -o example_program example_program.o -lmem_test
> jbuchana$

If you decide to uninstall, the Makefile has a target for just that.

Example:

> [root@zaphod mem_test]# make uninstall
> rm /usr/lib/libmem_test.a
> rm /usr/include/mem_test_user.h
> rm /usr/local/bin/mem_analyze
> rm /usr/man/man1/mem_analyze.1
> rm /usr/man/man3/mem_test.3
> rm /usr/man/man3/libmem_test.3
> [root@zaphod mem_test]#

Note that you must not edit the Makefile (well, the parts that tell
where files are to be installed) between installation and
uninstallation, or make won't know where the files were installed.
