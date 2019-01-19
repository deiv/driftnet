
noinst_LIBRARIES = libhttpdisplay.a
libhttpdisplay_a_SOURCES = httpd.c httpd.h web_data.c web_data.h

AM_CFLAGS  = -Wall
AM_CFLAGS += @WS_CFLAGS@
AM_CFLAGS += -I$(srcdir)/../

STATIC_WEB_FILES = $(wildcard static_web/*)

web_data.c: $(STATIC_WEB_FILES)
	rm -f '$@'

	echo '#include "web_data.h"' >> $@
	echo '#include <stddef.h>' >> $@

	for f in $^ ; do \
		printf 'unsigned char %s' `basename $$f | sed 's@[-. ]@_@g'` >> $@; \
		echo '[] = {' >> $@; \
		cat $$f | xxd -i >> $@; \
		echo '};' >> $@; \
	done

	echo 'web_static_file_t static_content[] = {' >> $@
	for f in $^ ; do \
		echo '{' >> $@; \
		basename $$f | sed 's@\(.*\)@"\1",@g' >> $@; \
		basename $$f | sed 's@[-. ]@_@g' >> $@; \
		echo ',' >> $@; \
		cat $$f | wc -c >> $@; \
		echo '},' >> $@ ;\
	done

	echo '{NULL, NULL, 0}};' >> $@

static-web: web_data.c

all-am: static-web

clean-local:
	-rm -f web_data.c
