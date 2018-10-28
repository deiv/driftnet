
noinst_LIBRARIES = libmedia.a
libmedia_a_SOURCES = media.c media.h image.c image.h audio.c audio.h \
					 mpeghdr.c mpeghdr.h playaudio.c playaudio.h http.c http.h \
					 pngformat.h

AM_CFLAGS  = -Wall
AM_CFLAGS += -I$(top_srcdir)/src

if ENABLE_TESTS
check_PROGRAMS = test_unit
TESTS = test_unit

test_unit_SOURCES = media.c \
                         audio.h \
                         playaudio.h \
                         mpeghdr.c \
                         playaudio.c \
                         http.c \
                         mpeghdr.h \
                         image.c \
                         audio.c \
                         image.h \
                         pngformat.h \
                         media.h \
                         http.h \
                         tests/test_unit.c

test_unit_CFLAGS =  -I$(top_srcdir)/src
test_unit_CFLAGS += -Wall -g -O0 -coverage
test_unit_CFLAGS += -D__FAVOR_BSD -D_BSD_SOURCE -D_DEFAULT_SOURCE # Get BSDish definitions of the TCP/IP structs (linux).
test_unit_CFLAGS += -DDRIFTNET_VERSION=\"$(VERSION)\"
test_unit_CFLAGS += -DDRIFTNET_PROGNAME=\"$(PACKAGE)\"
#test_unit_CFLAGS += -DNO_HTTP_DISPLAY -DNO_DISPLAY_WINDOW
test_unit_LDFLAGS = -lcmocka
test_unit_LDADD = libmedia.a ../common/libcommon.a
endif
