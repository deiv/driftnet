#
# Makefile:
# Makefile for driftnet.
#
# Copyright (c) 2001 Chris Lightfoot. All rights reserved.
# Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
#
# $Id: Makefile,v 1.13 2002/02/15 12:35:11 chris Exp $
#

VERSION = 0.1.5

#CC = gcc

CFLAGS  += -g -Wall `gtk-config --cflags` -DDRIFTNET_VERSION='"$(VERSION)"'
LDFLAGS += -g
LDLIBS  += -lpcap -ljpeg -lungif `gtk-config --libs`

# This may or may not be necessary on your system.
CFLAGS  += -I/usr/include/pcap

SUBDIRS = 

TXTS = README TODO COPYING CHANGES CREDITS driftnet.1
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

gif.o: /usr/include/gif_lib.h img.h /usr/include/stdint.h
gif.o: /usr/include/features.h /usr/include/sys/cdefs.h
gif.o: /usr/include/gnu/stubs.h
gif.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
gif.o: /usr/include/bits/wordsize.h /usr/include/stdio.h
gif.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
gif.o: /usr/include/bits/types.h /usr/include/libio.h
gif.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
img.o: /usr/include/stdio.h /usr/include/features.h /usr/include/sys/cdefs.h
img.o: /usr/include/gnu/stubs.h
img.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
img.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
img.o: /usr/include/bits/types.h /usr/include/libio.h
img.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
img.o: /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h
img.o: /usr/include/endian.h /usr/include/bits/endian.h
img.o: /usr/include/sys/select.h /usr/include/bits/select.h
img.o: /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h
img.o: /usr/include/alloca.h /usr/include/string.h img.h
img.o: /usr/include/stdint.h /usr/include/bits/wordsize.h
jpeg.o: /usr/include/stdio.h /usr/include/features.h /usr/include/sys/cdefs.h
jpeg.o: /usr/include/gnu/stubs.h
jpeg.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
jpeg.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
jpeg.o: /usr/include/bits/types.h /usr/include/libio.h
jpeg.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
jpeg.o: /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h
jpeg.o: /usr/include/endian.h /usr/include/bits/endian.h
jpeg.o: /usr/include/sys/select.h /usr/include/bits/select.h
jpeg.o: /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h
jpeg.o: /usr/include/alloca.h /usr/include/setjmp.h
jpeg.o: /usr/include/bits/setjmp.h /usr/include/jpeglib.h
jpeg.o: /usr/include/jconfig.h /usr/include/jmorecfg.h img.h
jpeg.o: /usr/include/stdint.h /usr/include/bits/wordsize.h
driftnet.o: /usr/include/assert.h /usr/include/features.h
driftnet.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
driftnet.o: /usr/include/errno.h /usr/include/bits/errno.h
driftnet.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
driftnet.o: /usr/include/pcap/pcap.h /usr/include/sys/types.h
driftnet.o: /usr/include/bits/types.h
driftnet.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
driftnet.o: /usr/include/time.h /usr/include/endian.h
driftnet.o: /usr/include/bits/endian.h /usr/include/sys/select.h
driftnet.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
driftnet.o: /usr/include/sys/sysmacros.h /usr/include/sys/time.h
driftnet.o: /usr/include/bits/time.h /usr/include/pcap/net/bpf.h
driftnet.o: /usr/include/stdio.h
driftnet.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
driftnet.o: /usr/include/libio.h /usr/include/_G_config.h
driftnet.o: /usr/include/bits/stdio_lim.h /usr/include/linux/if_ether.h
driftnet.o: /usr/include/netinet/ip.h /usr/include/netinet/in.h
driftnet.o: /usr/include/limits.h /usr/include/bits/posix1_lim.h
driftnet.o: /usr/include/bits/local_lim.h /usr/include/linux/limits.h
driftnet.o: /usr/include/bits/posix2_lim.h /usr/include/stdint.h
driftnet.o: /usr/include/bits/wordsize.h /usr/include/bits/socket.h
driftnet.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
driftnet.o: /usr/include/asm/sockios.h /usr/include/bits/in.h
driftnet.o: /usr/include/bits/byteswap.h /usr/include/netinet/tcp.h
driftnet.o: /usr/include/stdlib.h /usr/include/alloca.h /usr/include/string.h
driftnet.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
driftnet.o: /usr/include/bits/confname.h /usr/include/getopt.h
driftnet.o: /usr/include/ctype.h /usr/include/fcntl.h
driftnet.o: /usr/include/bits/fcntl.h /usr/include/signal.h
driftnet.o: /usr/include/bits/signum.h /usr/include/bits/siginfo.h
driftnet.o: /usr/include/bits/sigaction.h /usr/include/bits/sigcontext.h
driftnet.o: /usr/include/asm/sigcontext.h /usr/include/bits/sigstack.h
driftnet.o: /usr/include/sys/stat.h /usr/include/bits/stat.h driftnet.h
driftnet.o: /usr/include/sys/socket.h /usr/include/arpa/inet.h
image.o: /usr/include/stdio.h /usr/include/features.h
image.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
image.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
image.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
image.o: /usr/include/bits/types.h /usr/include/libio.h
image.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
image.o: /usr/include/stdlib.h /usr/include/sys/types.h /usr/include/time.h
image.o: /usr/include/endian.h /usr/include/bits/endian.h
image.o: /usr/include/sys/select.h /usr/include/bits/select.h
image.o: /usr/include/bits/sigset.h /usr/include/sys/sysmacros.h
image.o: /usr/include/alloca.h /usr/include/string.h
display.o: /usr/include/gtk/gtk.h /usr/include/gdk/gdk.h
display.o: /usr/include/gdk/gdktypes.h /usr/include/glib.h
display.o: /usr/lib/glib/include/glibconfig.h /usr/include/limits.h
display.o: /usr/include/features.h /usr/include/sys/cdefs.h
display.o: /usr/include/gnu/stubs.h /usr/include/bits/posix1_lim.h
display.o: /usr/include/bits/local_lim.h /usr/include/linux/limits.h
display.o: /usr/include/bits/posix2_lim.h
display.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/float.h
display.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stdarg.h
display.o: /usr/include/gdk/gdkcursors.h /usr/include/gdk/gdkrgb.h
display.o: /usr/include/gtk/gtkaccelgroup.h /usr/include/gtk/gtkobject.h
display.o: /usr/include/gtk/gtkarg.h /usr/include/gtk/gtktypeutils.h
display.o: /usr/include/gtk/gtktypebuiltins.h /usr/include/gtk/gtkenums.h
display.o: /usr/include/gtk/gtkdebug.h /usr/include/gtk/gtkaccellabel.h
display.o: /usr/include/gtk/gtklabel.h /usr/include/gtk/gtkmisc.h
display.o: /usr/include/gtk/gtkwidget.h /usr/include/gtk/gtkadjustment.h
display.o: /usr/include/gtk/gtkdata.h /usr/include/gtk/gtkstyle.h
display.o: /usr/include/gtk/gtkalignment.h /usr/include/gtk/gtkbin.h
display.o: /usr/include/gtk/gtkcontainer.h /usr/include/gtk/gtkaspectframe.h
display.o: /usr/include/gtk/gtkframe.h /usr/include/gtk/gtkarrow.h
display.o: /usr/include/gtk/gtkbindings.h /usr/include/gtk/gtkbox.h
display.o: /usr/include/gtk/gtkbbox.h /usr/include/gtk/gtkbutton.h
display.o: /usr/include/gtk/gtkcalendar.h /usr/include/gtk/gtksignal.h
display.o: /usr/include/gtk/gtkmarshal.h /usr/include/gtk/gtkcheckbutton.h
display.o: /usr/include/gtk/gtktogglebutton.h
display.o: /usr/include/gtk/gtkcheckmenuitem.h /usr/include/gtk/gtkmenuitem.h
display.o: /usr/include/gtk/gtkitem.h /usr/include/gtk/gtkclist.h
display.o: /usr/include/gtk/gtkhscrollbar.h /usr/include/gtk/gtkscrollbar.h
display.o: /usr/include/gtk/gtkrange.h /usr/include/gtk/gtkvscrollbar.h
display.o: /usr/include/gtk/gtkcolorsel.h /usr/include/gtk/gtkwindow.h
display.o: /usr/include/gtk/gtkvbox.h /usr/include/gtk/gtkpreview.h
display.o: /usr/include/gtk/gtkentry.h /usr/include/gtk/gtkeditable.h
display.o: /usr/include/gtk/gtkhbox.h /usr/include/gtk/gtkmain.h
display.o: /usr/include/gtk/gtkscale.h /usr/include/gtk/gtkhscale.h
display.o: /usr/include/gtk/gtktable.h /usr/include/gtk/gtkeventbox.h
display.o: /usr/include/gtk/gtkcombo.h /usr/include/gtk/gtkcompat.h
display.o: /usr/include/gtk/gtkctree.h /usr/include/gtk/gtkcurve.h
display.o: /usr/include/gtk/gtkdrawingarea.h /usr/include/gtk/gtkdialog.h
display.o: /usr/include/gtk/gtkdnd.h /usr/include/gtk/gtkselection.h
display.o: /usr/include/gtk/gtkfeatures.h /usr/include/gtk/gtkfilesel.h
display.o: /usr/include/gtk/gtkfixed.h /usr/include/gtk/gtkfontsel.h
display.o: /usr/include/gtk/gtknotebook.h /usr/include/gtk/gtkgamma.h
display.o: /usr/include/gtk/gtkgc.h /usr/include/gtk/gtkhandlebox.h
display.o: /usr/include/gtk/gtkhbbox.h /usr/include/gtk/gtkhpaned.h
display.o: /usr/include/gtk/gtkpaned.h /usr/include/gtk/gtkhruler.h
display.o: /usr/include/gtk/gtkruler.h /usr/include/gtk/gtkhseparator.h
display.o: /usr/include/gtk/gtkseparator.h /usr/include/gtk/gtkimage.h
display.o: /usr/include/gtk/gtkinputdialog.h
display.o: /usr/include/gtk/gtkitemfactory.h
display.o: /usr/include/gtk/gtkmenufactory.h /usr/include/gtk/gtklayout.h
display.o: /usr/include/gtk/gtklist.h /usr/include/gtk/gtklistitem.h
display.o: /usr/include/gtk/gtkmenu.h /usr/include/gtk/gtkmenushell.h
display.o: /usr/include/gtk/gtkmenubar.h /usr/include/gtk/gtkoptionmenu.h
display.o: /usr/include/gtk/gtkpacker.h /usr/include/gtk/gtkpixmap.h
display.o: /usr/include/gtk/gtkplug.h /usr/include/gtk/gtkprogress.h
display.o: /usr/include/gtk/gtkprogressbar.h
display.o: /usr/include/gtk/gtkradiobutton.h
display.o: /usr/include/gtk/gtkradiomenuitem.h /usr/include/gtk/gtkrc.h
display.o: /usr/include/gtk/gtkscrolledwindow.h
display.o: /usr/include/gtk/gtkviewport.h /usr/include/gtk/gtksocket.h
display.o: /usr/include/gtk/gtkspinbutton.h /usr/include/gtk/gtkstatusbar.h
display.o: /usr/include/gtk/gtktearoffmenuitem.h /usr/include/gtk/gtktext.h
display.o: /usr/include/gtk/gtkthemes.h /usr/include/gtk/gtktipsquery.h
display.o: /usr/include/gtk/gtktoolbar.h /usr/include/gtk/gtktooltips.h
display.o: /usr/include/gtk/gtktree.h /usr/include/gtk/gtktreeitem.h
display.o: /usr/include/gtk/gtkvbbox.h /usr/include/gtk/gtkvpaned.h
display.o: /usr/include/gtk/gtkvruler.h /usr/include/gtk/gtkvscale.h
display.o: /usr/include/gtk/gtkvseparator.h /usr/include/stdio.h
display.o: /usr/lib/gcc-lib/i386-redhat-linux/egcs-2.91.66/include/stddef.h
display.o: /usr/include/bits/types.h /usr/include/libio.h
display.o: /usr/include/_G_config.h /usr/include/bits/stdio_lim.h
display.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
display.o: /usr/include/bits/confname.h /usr/include/getopt.h
display.o: /usr/include/fcntl.h /usr/include/bits/fcntl.h
display.o: /usr/include/sys/types.h /usr/include/time.h /usr/include/endian.h
display.o: /usr/include/bits/endian.h /usr/include/sys/select.h
display.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
display.o: /usr/include/sys/sysmacros.h /usr/include/string.h
display.o: /usr/include/errno.h /usr/include/bits/errno.h
display.o: /usr/include/linux/errno.h /usr/include/asm/errno.h driftnet.h
display.o: /usr/include/sys/socket.h /usr/include/bits/socket.h
display.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
display.o: /usr/include/asm/sockios.h /usr/include/netinet/in.h
display.o: /usr/include/stdint.h /usr/include/bits/wordsize.h
display.o: /usr/include/bits/in.h /usr/include/bits/byteswap.h
display.o: /usr/include/arpa/inet.h /usr/include/sys/time.h
display.o: /usr/include/bits/time.h img.h
