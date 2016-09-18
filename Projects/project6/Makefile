#
#
# makefile for webserver
#

CC = gcc -Wall

ws: ws.o socklib.o
	$(CC) -o ws ws.o socklib.o

wsng: wsng.o socklib.o web-time.o
	$(CC) -o wsng wsng.o socklib.o web-time.o

clean:
	rm -f *.o core
