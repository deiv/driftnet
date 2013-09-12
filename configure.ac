#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#

AC_PREREQ([2.69])
AC_INIT([driftnet], [1.1.2], [david.sephirot@gmail.com])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_CONFIG_SRCDIR([src/driftnet.c])
AC_CONFIG_HEADERS(src/[config.h])

AC_ARG_ENABLE([display],
    [AC_HELP_STRING([--disable-display], [disable display (default is no)])],
    ,
    [enable_display=yes])

if test "x$enable_display" = xno; then
    AC_DEFINE([NO_DISPLAY_WINDOW], [1], [Compile without support for a display window])
fi
AM_CONDITIONAL(ENABLE_DISPLAY, test "x$enable_display" = xyes)

AC_ARG_ENABLE([debug],
	[AC_HELP_STRING([--disable-debug], [disable debugging (default is no)])],
    ,
    [enable_debug=yes])

if test "x$enable_debug" = xyes; then
    CFLAGS="-O2 -g -DDEBUG"
else
	CFLAGS="-O2 -DNDEBUG"
fi
#AM_CONDITIONAL(ENABLE_DEBUG, test "$enable_debug" = "yes")

#
# Checks for programs.
#

AC_PROG_CC
AC_PROG_INSTALL
#AM_PROG_CC_C_O
PKG_PROG_PKG_CONFIG

AM_PROG_AR
AC_PROG_RANLIB

#
# Checks for libraries.
#

AC_CHECK_LIB([jpeg],
    [jpeg_read_header],
    [],
    [AC_MSG_ERROR([cannot find lib jpeg])] )

AC_CHECK_LIB([pcap],
    [pcap_open_live],
    [],
    [AC_MSG_ERROR([cannot find lib pcap])] )

AC_CHECK_LIB([png],
    [png_read_info],
    [],
    [AC_MSG_ERROR([cannot find lib png])] )

AC_CHECK_LIB([pthread],
    [pthread_create],
    [],
    [AC_MSG_ERROR([cannot find lib pthread])] )

AC_CHECK_LIB([gif],
    [DGifOpenFileHandle],
    [],
    [AC_MSG_ERROR([cannot find lib gif])] )

if test "x$enable_display" = xyes; then

	AC_CHECK_LIB([glib-2.0],
        [g_free],
        [],
		[AC_MSG_ERROR([cannot find lib glib-2.x])])

	AC_CHECK_LIB([gobject-2.0],
        [g_signal_connect_data],
        [],
		[AC_MSG_ERROR([cannot find lib libgobject-2.0])])

	AC_CHECK_LIB([gtk-x11-2.0],
        [gtk_main],
        [],
		[AC_MSG_ERROR([cannot find lib gtk-2.x])])

#	AC_CHECK_LIB([gthread-2.0],
#        [g_thread_init],
#        [],
#		[AC_MSG_ERROR([cannot find lib libgthread-2.0])])

	AC_CHECK_LIB([gdk-x11-2.0],
        [gdk_window_get_state],
        [],
		[AC_MSG_ERROR([cannot find lib gdk-x11-2.0])])

#	AC_CHECK_LIB([Xrender],
#        [XRenderComposite],
#        [],
#		[AC_MSG_ERROR([cannot find lib XRender])])

	GTK_CFLAGS="$(${PKG_CONFIG} --cflags gtk+-x11-2.0 glib-2.0 gobject-2.0 gdk-x11-2.0)"
    #gthread-2.0)"

    AC_SUBST(GTK_CFLAGS)
fi

#
# Checks for header files.
#

AC_CHECK_HEADERS(
    [arpa/inet.h fcntl.h limits.h netinet/in.h stdlib.h string.h sys/socket.h \
        sys/time.h unistd.h sys/types.h stdint.h inttypes.h],
    [],
    [])

#
# Checks for typedefs, structures, and compiler characteristics.
#

AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Check for byte order.
AC_C_BIGENDIAN(
    [AC_DEFINE(DRIFTNET_BIG_ENDIAN, [1], [Define if byte order is big endian.])],
    [AC_DEFINE(DRIFTNET_LITTLE_ENDIAN, [1], [Define if byte order is litle endian.])],
    [AC_MSG_ERROR([unknown endianess ?])],
    [AC_MSG_ERROR([unknown endianess ?])]
)

AC_C_INLINE

#
# Checks for library functions.
#

AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([dup2 inet_ntoa memchr memset mkdir rmdir strcasecmp strchr strdup strerror strncasecmp strrchr])

# nanosleep seems to exist in librt or libposix4 on Solaris
sleep_func=no
AC_SEARCH_LIBS([nanosleep],
    [libc rt posix4],
    [AC_CHECK_FUNCS([nanosleep], sleep_func=nanosleep, [])],
    [])

if test x$sleep_func = xno; then
    AC_CHECK_FUNCS([usleep],
        [sleep_func=usleep],
        [AC_MSG_ERROR([cannot find any usable sleep function])])
fi

AC_CONFIG_FILES([Makefile src/Makefile src/media/Makefile src/display/Makefile])
AC_OUTPUT

echo \
"-------------------------------------------------

 ${PACKAGE_NAME} Version ${PACKAGE_VERSION}

 Prefix: '${prefix}'
 Compiler: '${CC} ${CFLAGS}'

 Package features:
   - enable display: ${enable_display}
   - enable debug: ${enable_debug}

 Now type:
   - 'make' to build the package
   - 'make install' to install everything

--------------------------------------------------"
