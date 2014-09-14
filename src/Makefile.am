
bin_PROGRAMS = driftnet

driftnet_SOURCES = compat.h connection.c connection.h driftnet.c driftnet.h \
				   log.c log.h options.c options.h packetcapture.c packetcapture.h \
				   tmpdir.c tmpdir.h util.c util.h layer2.c layer2.h layer3.c layer3.h \
				   pid.c pid.h

SUBDIRS = media
AM_CFLAGS = -I$(srcdir)/media
driftnet_LDADD = media/libmedia.a

if ENABLE_DISPLAY
SUBDIRS += display
AM_CFLAGS += -I$(srcdir)/display
driftnet_LDADD += display/libdisplay.a
endif

AM_CFLAGS += -Wall
AM_CFLAGS += -D__FAVOR_BSD -D_BSD_SOURCE # Get BSDish definitions of the TCP/IP structs (linux).
AM_CFLAGS += -DDRIFTNET_VERSION=\"$(VERSION)\"
AM_CFLAGS += -DDRIFTNET_PROGNAME=\"$(PACKAGE)\"
