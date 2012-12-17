
noinst_LIBRARIES = libdisplay.a
libdisplay_a_SOURCES = display.c display.h img.c img.h gif.c jpeg.c png.c

AM_CFLAGS  = -Wall
AM_CFLAGS += @GTK_CFLAGS@
AM_CFLAGS += -I$(top_srcdir)/src
AM_CFLAGS += -I$(top_srcdir)/src/media # for pngformat.h
