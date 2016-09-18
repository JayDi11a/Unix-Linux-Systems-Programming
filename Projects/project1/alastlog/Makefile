# ----------------------------------------------------------
# This is a makefile.  A makefile contains rules that tell
# how to build a program, often from separate source files
#
# This sample makefile shows how to build dumputmp from
# the two source files dumputmp.c and utmplib.c
#
# You must modify this file so it contains rules to make
# your solution to the project
# (note: the indented lines MUST start with a single tab
#

alastlog: alastlog.o utmplib.o
	cc -Wall alastlog.o utmplib.o -o alastlog

alastlog.o: alastlog.c
	$(CC) -c alastlog.c

utmplib.o: utmplib.c
	$(CC) -c utmplib.c

dumputmp: dumputmp.o utmplib.o
	cc dumputmp.o utmplib.o -o dumputmp

dumputmp.o: dumputmp.c
	$(CC) -c dumputmp.c

clean:
	rm -f *.o core alastlog utmplib.o

