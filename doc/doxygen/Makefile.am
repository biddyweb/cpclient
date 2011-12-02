docpkg = $(PACKAGE_TARNAME)-doxy-$(PACKAGE_VERSION).tar.gz

doc_DATA = $(docpkg)

DEPENDENCIES = \
	$(top_srcdir)/doc/mainpage.h \
	$(top_srcdir)/doc/telephony.h \
	$(top_srcdir)/doc/browser.h \
	$(top_srcdir)/doc/synchronisation.h \
	$(top_srcdir)/doc/omadm.h \
	$(top_srcdir)/doc/email.h \
	$(top_srcdir)/doc/manager.h \
	$(top_srcdir)/doc/push-message.h


$(docpkg): $(DEPENDENCIES)
	$(DOXYGEN) $(DOXYFLAGS)
	tar chof - html | gzip -9 -c >$@

install-data-hook:
	cd $(DESTDIR)$(docdir) && tar xf $(docpkg)

uninstall-hook:
	cd $(DESTDIR)$(docdir) && rm -rf html

CLEANFILES = doxywarn.txt $(docpkg)

clean-local:
	rm -rf html

