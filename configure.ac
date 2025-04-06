#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#

AC_PREREQ([2.71])
AC_INIT([driftnet],[1.6.0],[david.sephirot@gmail.com])

AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([foreign -Wall -Werror -Wno-portability subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_CONFIG_SRCDIR([src/driftnet.c])
AC_CONFIG_HEADERS(src/[compat/config.h])

#
# Detect then host and target system
#

build_windows=no
build_linux=no
build_mac=no

case "${host_os}" in
    linux*|freebsd*)
        build_linux=yes
        ;;
    cygwin*|mingw*)
		case "${target_cpu}" in
			i686*)
				build_windows=yes
				;;
			*)
				AC_MSG_ERROR(["Building for $target_cpu is not supported (only 32 bits builds)"])
				;;
		esac
        ;;
    darwin*)
        build_mac=yes
        ;;
    *)
        AC_MSG_ERROR(["OS $host_os is not supported"])
        ;;
esac

AC_ARG_ENABLE(
    [display],
    [AS_HELP_STRING([--disable-display],[disable display (default is no)])],
    ,
    [enable_display=yes])

if test "x$enable_display" = xno; then
    AC_DEFINE([NO_DISPLAY_WINDOW], [1], [Compile without support for a display window])
fi
AM_CONDITIONAL(ENABLE_DISPLAY, test "x$enable_display" = xyes)

AC_ARG_ENABLE([http-display],
    [AS_HELP_STRING([--disable-http-display],[disable http display (default is yes)])],
    ,
    [enable_http_display=yes])

if test "x$enable_http_display" = xno; then
    AC_DEFINE([NO_HTTP_DISPLAY], [1], [Compile without support for http display])
fi
AM_CONDITIONAL(ENABLE_HTTP_DISPLAY, test "x$enable_http_display" = xyes)

AC_ARG_ENABLE([http-display-in-src-statics],
    [AS_HELP_STRING([--enable-http-display-in-src-statics],[use html statics from src instead of pkgdatadir (default is no)])],
    [use_http_in_src_statics=yes],
    )

if test "x$use_http_in_src_statics" = xyes; then
    AC_DEFINE([USE_HTTP_DISPLAY_IN_SRC_STATICS], [1], [Use html statics from src instead of pkgdatadir])
fi
AM_CONDITIONAL(ENABLE_HTTP_DISPLAY_IN_SRC_STATICS, test "x$use_http_in_src_statics" = xyes)

AC_ARG_ENABLE([debug],
	[AS_HELP_STRING([--disable-debug],[disable debugging (default is no)])],
    ,
    [enable_debug=yes])

if test "x$enable_debug" = xyes; then
    CFLAGS="-g -DDEBUG"
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

#
# On windows we use the libwinpcap library
#
if test "x$build_windows" = xyes; then
	AC_CHECK_LIB([wpcap],
		[pcap_open_live],
		[],
		[AC_MSG_ERROR([cannot find lib wpcap])] )
    AC_DEFINE([PCAP_LIB_ON_WINDOWS], [1], [Compile using linwinpcap library])
else
	AC_CHECK_LIB([pcap],
		[pcap_open_live],
		[],
		[AC_MSG_ERROR([cannot find lib pcap])] )
fi

AC_CHECK_LIB([pthread],
    [pthread_create],
    [],
    [AC_MSG_ERROR([cannot find lib pthread])] )

if test "x$enable_display" = xyes; then

    PKG_CHECK_MODULES(
        GTK,
        gtk+-3.0 >= 3.0.0,
        [gtk_version_to_link=3.x.x],
        [AC_MSG_ERROR([cannot find gtk+-3.0 >= 3.0.0])])

    AC_CHECK_LIB([gif],
    [DGifOpenFileHandle],
    [],
    [AC_MSG_ERROR([cannot find lib gif])] )

    AC_CHECK_LIB([png],
    [png_read_info],
    [],
    [AC_MSG_ERROR([cannot find lib png])] )

    AC_CHECK_LIB([jpeg],
    [jpeg_read_header],
    [],
    [AC_MSG_ERROR([cannot find lib jpeg])] )
else
    gtk_version_to_link=none
fi


AC_CHECK_LIB([webp],
    [WebPDecodeRGBInto],
    [],
    [AC_MSG_ERROR([cannot find lib webp])] )

if test "x$enable_http_display" = xyes; then
    AC_CHECK_LIB([websockets],
        [lws_create_context],
        [],
        [AC_MSG_ERROR([cannot find lib websockets])] )

    AC_CHECK_HEADER([uv/version.h],
        [],
        [],
        [])

     WS_CFLAGS="$(${PKG_CONFIG} --cflags libwebsockets)"
     AC_SUBST(WS_CFLAGS)
fi


AC_CHECK_LIB([cmocka],
    _cmocka_run_group_tests,
    [enable_tests=yes],
    [AC_MSG_WARN([cannot find lib cmocka, the testsuite will not run])])
AM_CONDITIONAL(ENABLE_TESTS, test "x$enable_tests" = xyes)

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

#
# AC_FUNC_MALLOC and AC_FUNC_REALLOC seems to not work on cygwin, we check them using AC_CHECK_FUNCS
#
if test "x$build_windows" = xyes; then
	AC_CHECK_FUNCS([malloc realloc])
else
	AC_FUNC_MALLOC
	AC_FUNC_REALLOC
fi

AC_FUNC_FORK

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

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/common/Makefile
                 src/network/Makefile
                 src/media/Makefile
                 src/display/Makefile
                 src/http_display/Makefile])

AC_OUTPUT

echo \
"-------------------------------------------------

 ${PACKAGE_NAME} Version ${PACKAGE_VERSION}

 Prefix: '${prefix}'
 Compiler: '${CC} ${CFLAGS}'

 Package features:
   - enable display: ${enable_display}
   - GTK version found: ${gtk_version_to_link}
   - enable http display: ${enable_http_display}
   - enable debug: ${enable_debug}

 Now type:
   - 'make' to build the package
   - 'make install' to install everything

--------------------------------------------------"
