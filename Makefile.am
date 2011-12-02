cpc_sources = \
		lib/src/characteristic.h \
		lib/src/characteristic.c \
		lib/src/context.c \
		lib/src/wp.c \
		lib/src/log.c \
		lib/src/file-posix.c \
		lib/src/ptr-array.c \
		lib/src/wbxml-peer.h \
		lib/src/wbxml-libwbxml.c \
		lib/src/hmac-peer.h \
		lib/src/hmac-gnutls.c \
		src/tasks.h \
		src/callback.h \
		src/provision-cp.h \
		src/provision-cp.c \
		src/provision.h \
		src/provision.c \
		src/settings.h \
		src/settings.c \
		src/tasks.c \
		src/dbus-error.h \
		src/dbus-error.c \
		src/pm-manager.c \
		src/pm-manager.h \
		src/imsi.c \
		src/imsi.h \
		src/provision-wp.c \
		src/provision-wp.h \
		src/cpclient.c

cpc_headers = \
		lib/include/context.h \
		lib/include/wp.h \
		lib/include/error.h \
		lib/include/error-macros.h \
		lib/include/file-peer.h \
		lib/include/ptr-array.h	\
		lib/include/log.h	

cpc_testcases = \
		testcases/commands/parsecp \
		testcases/commands/parsewp \
		testcases/examples/bookmarks.xml \
		testcases/examples/data-sync.xml \
		testcases/examples/email.xml \
		testcases/examples/internetmms.xml \
		testcases/examples/omadm.xml \
		testcases/examples/proxy-nap-group.xml \
		testcases/examples/proxytest.xml \
		testcases/examples/simplemms.xml


if TEST
testdir = $(pkglibdir)/test
dist_test_SCRIPTS = $(pm_testcases)
endif

bin_PROGRAMS = cpclient
cpclient_SOURCES = $(cpc_headers) $(cpc_sources)
cpclient_CPPFLAGS = -I lib/include $(GLIB_CFLAGS)  $(GIO_CFLAGS)\
 $(LIBXML_CFLAGS) $(GNUTLS_CFLAGS) $(LIBWBXML_CFLAGS)
cpclient_LDADD = $(GLIB_LIBS) $(GIO_LIBS) $(LIBXML_LIBS) $(GNUTLS_LIBS)\
 $(LIBWBXML_LIBS)

dbussessiondir = @DBUS_SESSION_DIR@
dist_dbussession_DATA = src/com.intel.cpclient.server.service

pm_docs = \
		doc/mainpage.h \
		doc/telephony.h	\
		doc/browser.h \
		doc/synchronisation.h \
		doc/email.h \
		doc/manager.h \
		doc/omadm.h \
		doc/push-message.h

EXTRA_DIST = $(pm_docs)

SUBDIRS = doc



