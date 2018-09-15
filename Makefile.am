
SUBDIRS = src

dist_doc_DATA = README.md TODO
EXTRA_DIST = driftnet.1.in CREDITS Changelog

MANPAGE = driftnet.1.gz
MAN1DIR = $(DESTDIR)$(mandir)/man1/

clean-local:
	rm -f $(MANPAGE)

install-data-local: driftnet.1.in
	cat $(srcdir)/driftnet.1.in | sed s/@*VERSION@*/$(VERSION)/ | GZIP=$(GZIP_ENV) gzip > $(MANPAGE)
	$(MKDIR_P) $(MAN1DIR)
	$(INSTALL_DATA) $(MANPAGE) $(MAN1DIR) || exit $$?
