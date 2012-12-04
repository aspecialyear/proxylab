CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

all: proxy

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

sbuf.o: sbuf.c sbuf.h
	$(CC) $(CFLAGS) -c sbuf.c

proxylhelpers.o: proxylhelpers.c proxylhelpers.h csapp.h
	$(CC) $(CFLAGS) -c proxylhelpers.c

proxyservant.o: proxyservant.c proxyservant.h proxylhelpers.h csapp.h
	$(CC) $(CFLAGS) -c proxyservant.c

proxy.o: proxy.c csapp.h
	$(CC) $(CFLAGS) -c proxy.c

proxy: proxy.o csapp.o sbuf.o proxylhelpers.o proxyservant.o

submit:
	(make clean; cd ..; tar cvf proxylab.tar proxylab-handout)

clean:
	rm -f *~ *.o proxy core

