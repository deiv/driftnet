
noinst_LIBRARIES = libmedia.a
libmedia_a_SOURCES = media.c media.h image.c image.h audio.c audio.h \
					 mpeghdr.c mpeghdr.h playaudio.c playaudio.h http.c http.h \
					 pngformat.h

AM_CFLAGS  = -Wall
AM_CFLAGS += -I$(top_srcdir)/src
if ENABLE_DISPLAY
AM_CFLAGS += -I$(top_srcdir)/src/display
endif
