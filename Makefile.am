
SUBDIRS = src

dist_doc_DATA = README.md TODO
EXTRA_DIST = driftnet.1.in CREDITS Changelog

MANPAGE = driftnet.1.gz
MAN1DIR = $(DESTDIR)$(mandir)/man1/

clean-local: clean-coverage clean-docs
	rm -f $(MANPAGE)

install-data-local: driftnet.1.in
	cat $(srcdir)/driftnet.1.in | sed s/@*VERSION@*/$(VERSION)/ | GZIP=$(GZIP_ENV) gzip > $(MANPAGE)
	$(MKDIR_P) $(MAN1DIR)
	$(INSTALL_DATA) $(MANPAGE) $(MAN1DIR) || exit $$?

docs:
	doxygen driftnet.doxygen

clean-docs:
	rm -Rf docs

if ENABLE_TESTS
coverage: check
	lcov --capture --directory src --output-file coverage.info
	genhtml coverage.info --output-directory coverage-docs

clean-coverage:
	find . -iname '*.gcda' -delete -or -iname '*.gcno' -delete
	rm -Rf coverage.info coverage-docs
endif
