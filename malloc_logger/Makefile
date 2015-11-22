
# Uncomment this line if you are using SunOS 4.x, or if
# you are running another OS that does not provide a
# strerror() function One warning sign might be compile
# errors that complain about an argument not being a pointer
#OSFLAG = -DSUNOS

# This is where your library file will go
LIBDIR = /usr/lib

# This is where your header file will go
INCDIR = /usr/include

# This is where the executables go
BINDIR = /usr/local/bin

# This is where the man pages go
MANDIR = /usr/man

# This is your C compiler
CC = gcc

# Switches for the compiler, use -g for debugging
# -Wno-implicit prevents a lot of irritating errors
# under SunOS 4.1.2
# unlike most programs, you'll probably want to leave
# the -g in, since you'll like link with this library
# when debugging
CFLAGS = -Wall -Wno-implicit -g
#CFLAGS = -Wall -Wno-implicit

ALL = mem_test

all: $(ALL)

mem_test: mem_test.o
	- rm libmem_test.a
	ar r libmem_test.a mem_test.o

mem_test.o: mem_test.c
	$(CC) $(CFLAGS) $(OSFLAG) -c mem_test.c

example_program: example_program.o
	$(CC) $(CFLAGS) -o example_program example_program.o -lmem_test

example_program.o: example_program.c
	$(CC) $(CFLAGS) -c example_program.c

install:
	cp libmem_test.a $(LIBDIR)
	ranlib $(LIBDIR)/libmem_test.a
	cp mem_test_user.h $(INCDIR)
	cp mem_analyze $(BINDIR)
	cp mem_analyze.1 $(MANDIR)/man1/mem_analyze.1
	cp libmem_test.3 $(MANDIR)/man3/libmem_test.3
	ln $(MANDIR)/man3/libmem_test.3 $(MANDIR)/man3/mem_test.3

uninstall:
	- rm $(LIBDIR)/libmem_test.a
	- rm $(INCDIR)/mem_test_user.h
	- rm $(BINDIR)/mem_analyze
	- rm $(MANDIR)/man1/mem_analyze.1
	- rm $(MANDIR)/man3/mem_test.3
	- rm $(MANDIR)/man3/libmem_test.3

clean:
	- rm $(ALL).o lib$(ALL).a example_program example_program.o
