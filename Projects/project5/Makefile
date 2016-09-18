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

CC = gcc -g -Wall

smsh: smsh.o splitline.o execute2.o process2.o controlflow.o \
		builtin.o flexstr.o varlib.o
	$(CC) -o smsh smsh.o splitline.o execute2.o process2.o \
		controlflow.o builtin.o flexstr.o varlib.o
clean:
	rm -f *.o smsh core


