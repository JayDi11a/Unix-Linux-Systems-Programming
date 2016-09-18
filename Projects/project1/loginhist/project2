#
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

dumputmp: dumputmp.o utmplib.o
	cc dumputmp.o utmplib.o -o dumputmp

see: dumputmp sample.wtmp
	./dumputmp sample.wtmp | more

dumputmp.o: dumputmp.c
	$(CC) -c dumputmp.c

utmplib.o: utmplib.c
	$(CC) -c utmplib.c

who2: who2.c
	$(CC) who2.c -o who2

who3: who3.c utmplib.o
	$(CC) who3.c utmplib.o -o who3

lastlogin: lastlogin.c utmplib.o
	$(CC) lastlogin.c utmplib.o -o lastlogin

whodate: whodate.c utmplib.o
	$(CC) whodate.c utmplib.o -o whodate

clean:
	rm -f *.o core dumputmp

