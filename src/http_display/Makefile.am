
STATIC_WEB_FILES = $(shell find static_web/ -not -type d -name '*')

noinst_LIBRARIES = libhttpdisplay.a
libhttpdisplay_a_SOURCES = httpd.c httpd.h web_data.c web_data.h

htmldir = $(pkgdatadir)/static-html
dist_html_DATA = $(STATIC_WEB_FILES)

AM_CFLAGS  = -Wall
AM_CFLAGS += @WS_CFLAGS@
AM_CFLAGS += -I$(srcdir)/../
AM_CFLAGS += -DSTATIC_WEB_DIR=\"$(htmldir)\"
AM_CFLAGS += -I$(srcdir)/../compat

web_data.c: $(STATIC_WEB_FILES)
	rm -f '$@'

	echo '#include "web_data.h"' >> $@

	echo 'char* static_files[] = {' >> $@
	for f in $^ ; do \
		basename $$f | sed 's@\(.*\)@"\1",@g' >> $@; \
	done

	echo '0};' >> $@

static-web: web_data.c

all-am: static-web

clean-local:
	-rm -f web_data.c
