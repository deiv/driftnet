#
# Makefile:
# Makefile for driftnet.
#
# Copyright (c) 2001 Chris Lightfoot. All rights reserved.
# Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
#
# $Id: Makefile,v 1.3 2001/07/20 12:02:30 chris Exp $
#

VERSION = 0.1.1

#CC = gcc

# You may need to edit the -I/usr/include/pcap and add a -L... option to
# LDFLAGS below take account of the location of libpcap and its header files
# on your system.
CFLAGS  += -g -Wall `gtk-config --cflags` -I/usr/include/pcap/
LDFLAGS += -g
LDLIBS  += -lpcap -ljpeg -lungif -lpnm -lppm -lpgm -lpbm `gtk-config --libs`

SUBDIRS = 

TXTS = README TODO COPYING CHANGES
SRCS = gif.c img.c jpeg.c png.c pnm.c raw.c driftnet.c image.c display.c
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
