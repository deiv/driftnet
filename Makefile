#
# Makefile:
# Makefile for driftnet.
#
# Copyright (c) 2001 Chris Lightfoot. All rights reserved.
# Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
#
# $Id: Makefile,v 1.8 2001/08/11 13:45:42 chris Exp $
#

VERSION = 0.1.3pre3

#CC = gcc

CFLAGS  += -g -Wall `gtk-config --cflags` -DDRIFTNET_VERSION='"$(VERSION)"'
LDFLAGS += -g
LDLIBS  += -lpcap -ljpeg -lungif `gtk-config --libs`

# This may or may not be necessary on your system.
CFLAGS  += -I/usr/include/pcap

SUBDIRS = 

TXTS = README TODO COPYING CHANGES
SRCS = gif.c img.c jpeg.c png.c driftnet.c image.c display.c
HDRS = img.h driftnet.h
BINS = driftnet

OBJS = $(SRCS:.c=.o)

driftnet:   depend $(OBJS)
	$(CC) -o driftnet $(OBJS) $(LDFLAGS) $(LDLIBS)

%.o:    %.c Makefile
	$(CC) $(CFLAGS) -c -o $@ $<

clean:  nodepend
	rm -f *~ *.bak *.o core $(BINS) TAGS

tags:
	etags *.c *.h

tarball: nodepend $(SRCS) $(HDRS) $(TXTS)
	mkdir driftnet-$(VERSION)
	set -e ; for i in Makefile $(SRCS) $(HDRS) $(TXTS) ; do cp $$i driftnet-$(VERSION)/$$i ; done
	tar cvzf driftnet-$(VERSION).tar.gz driftnet-$(VERSION)
	rm -rf driftnet-$(VERSION)
	mv driftnet-$(VERSION).tar.gz ..
	

depend:
	makedepend -- $(CFLAGS) -- $(SRCS)
	touch depend
	rm -f Makefile.bak

nodepend:
	makedepend -- --
	rm -f depend Makefile.bak

# DO NOT DELETE


