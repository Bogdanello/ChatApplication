CC = gcc
DEBUG = -g
CFLAGS = -Wall -lpthread -c $(DEBUG)
LFLAGS = -Wall -lpthread $(DEBUG)

all: client server

client: client.o common.o
	$(CC) $(LFLAGS) client.o common.o -o client

server: server.o common.o
	$(CC) $(LFLAGS) server.o common.o -o server


client.o: client.c common.h
	$(CC) $(CFLAGS) client.c

server.o: server.c common.h
	$(CC) $(CFLAGS) server.c

common.o: common.h common.c
	$(CC) $(CFLAGS) common.c

clean:
	rm -rf *.o *~ client server
