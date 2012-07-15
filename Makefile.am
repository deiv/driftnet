
bin_PROGRAMS = driftnet

driftnet_SOURCES = audio.c mpeghdr.c gif.c img.c jpeg.c png.c driftnet.c image.c \
       display.c playaudio.c connection.c media.c util.c http.c \
	   img.h driftnet.h mpeghdr.h compat.h

dist_doc_DATA = README TODO
EXTRA_DIST = driftnet.1.in CHANGES CREDITS

AM_CFLAGS  = -Wall
AM_CFLAGS += @GTK_CFLAGS@
AM_CFLAGS += -D_BSD_SOURCE # Get BSDish definitions of the TCP/IP structs (linux).
AM_CFLAGS += -DDRIFTNET_VERSION=\"$(VERSION)\"
AM_CFLAGS += -DDRIFTNET_PROGNAME=\"$(PACKAGE)\"

MANPAGE = driftnet.1.gz
MAN1DIR = $(DESTDIR)$(mandir)/man1/

clean-local:
	rm -f $(MANPAGE)

install-data-local: driftnet.1.in
	cat $(srcdir)/driftnet.1.in | sed s/@*VERSION@*/$(VERSION)/ | GZIP=$(GZIP_ENV) gzip > $(MANPAGE)
	$(MKDIR_P) $(MAN1DIR)
	$(INSTALL_DATA) $(MANPAGE) $(MAN1DIR) || exit $$?

