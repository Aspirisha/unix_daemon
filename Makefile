LIB=-L$(shell pwd)
INC=-I$(shell pwd)
CC=g++
CFLAGS=-std=c++0x
SRCDIR=src/
BINDIR=bin/
all: my_daemon clean

my_daemon: MyLog.o MyDaemon.o MyMonitor.o main.o
	$(CC) MyLog.o MyDaemon.o MyMonitor.o main.o $(LIB) -lpthread -o $(BINDIR)my_daemon 
MyLog.o:
	$(CC) $(CFLAGS) -c $(SRCDIR)MyLog.cpp
MyDaemon.o:
	$(CC) $(CFLAGS) -c $(SRCDIR)MyDaemon.cpp
MyMonitor.o:
	$(CC) $(CFLAGS) -c $(SRCDIR)MyMonitor.cpp
main.o:
	$(CC) $(CFLAGS) -c $(SRCDIR)main.cpp
clean:
	rm -rf *o
