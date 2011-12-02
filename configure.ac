#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])
AC_INIT([cpclient], [0.0.1], [mark.d.ryan@intel.com])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects])

AC_CONFIG_MACRO_DIR([m4])
# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

AC_ARG_ENABLE([docs], [  --enable-docs compiles doxygen documentation during build ],
		      [ docs=${enableval} ], [ docs=yes] )

AC_ARG_ENABLE([tests], [  --enable-test installs test cases],
		       [ tests=${enableval} ], [ tests=no] )

AM_CONDITIONAL([TEST], test "x${tests}" = xyes)

# Checks for libraries.
PKG_PROG_PKG_CONFIG(0.16)
PKG_CHECK_MODULES([DBUS], [dbus-1])
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.26.1])
PKG_CHECK_MODULES([GIO], [gio-2.0 >= 2.26.1])
PKG_CHECK_MODULES([LIBXML], [libxml-2.0])
PKG_CHECK_MODULES([LIBWBXML], [libwbxml2 >= 0.11], 
      [ AC_DEFINE([HAVE_NEW_WBXML], [1], [ Indicates whether we are using a version of wbxml >= 0.11 ]) ],
      [ PKG_CHECK_MODULES([LIBWBXML], [libwbxml2]) ])
PKG_CHECK_MODULES([GNUTLS], [gnutls >= 2.10.4])

AC_CHECK_PROGS([DOXYGEN], [doxygen] )
# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UINT8_T
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset strchr strrchr strstr])

if test "x${CFLAGS}" = "x"; then
   CFLAGS="-O2"
fi

CFLAGS="$CFLAGS -Wall -Wextra"
CFLAGS="$CFLAGS -Wno-unused-parameter"
CFLAGS="$CFLAGS -Wno-missing-field-initializers"
CFLAGS="$CFLAGS -Wdeclaration-after-statement"
CFLAGS="$CFLAGS -Wredundant-decls"
CFLAGS="$CFLAGS -Wcast-align"

AC_ARG_ENABLE([logging], [  --enable-logging enables logging], [logging=${enableval}], [logging=no])

if test "x${logging}" = xyes; then
   AC_DEFINE([CPC_LOGGING], 1, [logging enabled])
fi

AC_DEFINE([CPC_LOG_FILE], "/tmp/cpclient-", [Path to cpc log file])

AC_ARG_ENABLE([overwrite], [  --enable-overwrite Allows existing accounts to be overwritten], 
			   [overwrite=${enableval}], [overwrite=yes])

if test "x${overwrite}" = xyes; then
   AC_DEFINE([CPC_OVERWRITE], 1, [overwrite enabled])
fi

AC_ARG_ENABLE([werror], [  --enable-werror Warnings are treated as errors ], 
			   [werror=${enableval}], [werror=yes])

if test "x${werror}" = xyes; then
   CFLAGS="$CFLAGS -Werror"
fi

DBUS_SESSION_DIR=`$PKG_CONFIG --variable=session_bus_services_dir dbus-1`
AC_SUBST(DBUS_SESSION_DIR)

AC_DEFINE([CPC_SERVICE], "com.intel.cpclient", [Prefixed used for dbus names])
AC_DEFINE([CPC_OBJECT], "/com/intel/cpclient", [Name of object exposed by cpclient])

CPC_SERVER_NAME=com.intel.cpclient.server
AC_SUBST(CPC_SERVER_NAME)
AC_DEFINE([CPC_SERVER_NAME], "com.intel.cpclient.server",
				 [d-Bus Name of the cpclient server])
CPC_INTERFACE_MANAGER=com.intel.cpclient.Manager
AC_SUBST(CPC_INTERFACE_MANAGER)
AC_DEFINE([CPC_INTERFACE_MANAGER], "com.intel.cpclient.Manager",
			       [d-Bus Name of the cpclient server interface])

CPC_INTERFACE_PUSH_MESSAGE=com.intel.cpclient.PushMessage
AC_SUBST(CPC_INTERFACE_PUSH_MESSAGE)
AC_DEFINE([CPC_INTERFACE_PUSH_MESSAGE], "com.intel.cpclient.PushMessage",
			       [d-Bus Name of the cpclient push message interface])

AC_DEFINE([PROVMAN_SERVICE], "com.intel.provman", [Prefixed used for dbus names])
AC_DEFINE([PROVMAN_OBJECT], "/com/intel/provman", [Name of object exposed by cpclient])

AC_CONFIG_FILES([Makefile
		 src/com.intel.cpclient.server.service
		 doc/Makefile])

if test "x${docs}" = xno; then
DOXYGEN=
fi

AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])
AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([doc/doxygen/doxyfile
					doc/doxygen/Makefile])])

AC_OUTPUT

echo \
"------------------------------------------------- 

${PACKAGE_NAME} Version ${PACKAGE_VERSION} 

Prefix: '${prefix}'. 
Compiler: '${CC} ${CFLAGS} ${CPPFLAGS}'
Doxygen: ${DOXYGEN:-NONE} 

Package features: 
	enable-docs: ${docs}
	enable-tests: ${tests}
	enable-logging: ${logging} 
	enable-overwrite: ${overwrite} 
	enable-werror: ${werror} 

 --------------------------------------------------"
