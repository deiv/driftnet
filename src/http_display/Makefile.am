
noinst_LIBRARIES = libhttpdisplay.a
libhttpdisplay_a_SOURCES = httpd.c httpd.h web_data.c web_data.h

AM_CFLAGS  = -Wall

STATIC_WEB_FILES = $(wildcard static_web/*)

web_data.c: $(STATIC_WEB_FILES)
	rm -f '$@'
	for f in $^; do xxd -i $$f >> $@; done

static-web: web_data.c

all-am: static-web

clean-local:
	-rm -f web_data.c
