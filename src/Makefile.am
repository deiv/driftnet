
bin_PROGRAMS = driftnet

driftnet_SOURCES = compat/compat.h driftnet.c driftnet.h \
				   options.c options.h \
				   pid.c pid.h uid.c uid.h \
				   media_dispatcher.c media_dispatcher.h

SUBDIRS = common
AM_CFLAGS = -I$(srcdir)/common
driftnet_LDADD = common/libcommon.a

SUBDIRS += network
AM_CFLAGS += -I$(srcdir)/network
driftnet_LDADD += network/libnetwork.a

SUBDIRS += media
AM_CFLAGS += -I$(srcdir)/media
driftnet_LDADD += media/libmedia.a

if ENABLE_DISPLAY
SUBDIRS += display
AM_CFLAGS += -I$(srcdir)/display
driftnet_LDADD += display/libdisplay.a
driftnet_LDADD += @GTK_LIBS@
endif

if ENABLE_HTTP_DISPLAY
SUBDIRS += http_display
AM_CFLAGS += -I$(srcdir)/http_display
driftnet_LDADD += http_display/libhttpdisplay.a
endif

AM_CFLAGS += -I$(srcdir)/compat

AM_CFLAGS += -Wall
AM_CFLAGS += -D__FAVOR_BSD -D_BSD_SOURCE -D_DEFAULT_SOURCE # Get BSDish definitions of the TCP/IP structs (linux).
AM_CFLAGS += -DDRIFTNET_VERSION=\"$(VERSION)\"
AM_CFLAGS += -DDRIFTNET_PROGNAME=\"$(PACKAGE)\"
