PROG	= ms
CC	= gcc
#CC	= gcc -b i486-linuxaout
# If you don't have gcc...
#CC	= cc

################################# Linux ######################################
#CFLAGS	= -O2 -m486 -fomit-frame-pointer -pipe -Wall -pedantic \


CFLAGS	= -ggdb3 -m486 -fomit-frame-pointer -pipe -Wall -pedantic \
		-Wmissing-prototypes -Wtraditional -Wstrict-prototypes \
		-Wshadow -Wpointer-arith -Wcast-align -Wnested-externs -Winline
DEPEND	= gcc
LIBS	=
##############################################################################

############################ Solaris, with gcc ###############################
#CFLAGS	= -O2 -fomit-frame-pointer -pipe -Wall -pedantic \
#		-Wmissing-prototypes -Wtraditional -Wstrict-prototypes \
#		-Wshadow -Wpointer-arith -Wcast-align -Wnested-externs \
#		-Winline -DSOLARIS
#LIBS	= -lsocket -lnsl
##############################################################################

############################ Solaris, using 'cc' #############################
#CFLAGS	= -DSOLARIS
#LIBS	= -lsocket -lnsl
##############################################################################

############################## SunOS, with gcc ###############################
#CFLAGS	= -O2 -fomit-frame-pointer -pipe -DSUNOS
#LIBS	=
##############################################################################

TOBJS	= main.o socket.o comms.o file.o

all:	$(TOBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(TOBJS) $(LIBS)

clean:
	rm -f *.o
	if [ -f $(PROG) ]; then rm -f $(PROG); fi

scrub:
	rm -f *~ '#*#' *.bak *.BAK DEADJOE

vclean:	scrub clean
	@echo "Very clean now *8-)"

%.o:
	$(CC) $(CFLAGS) -c $*.c -o $*.o

dist: vclean
	@(DIR=`basename \`pwd\`` ; cd .. ; tar --exclude $$DIR/RCS -c -v -z -f $$DIR.tar.gz $$DIR )

gcc-depend:
	$(CC) -MM *.c > .tmpdepend
	@(mv .tmpdepend .gcc-depend)

# Dependencies
###
include .gcc-depend

