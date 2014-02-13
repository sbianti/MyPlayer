AC_PREREQ([2.60])
AC_INIT([MyPlayer], [0.1], [sebastienbianti@free.fr],)
AM_INIT_AUTOMAKE([foreign subdir-objects color-tests])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])

dnl LT_PREREQ([2.2])
dnl LT_INIT([dlopen])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AM_MAINTAINER_MODE

# Checks for programs.
#AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX

# Checks for libraries.

PKG_PROG_PKG_CONFIG
#COMPILER_FLAGS
AC_LANG_C

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h termios.h unistd.h sys/ioctl.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_FUNCS([memset])

AC_PROG_INSTALL

AC_CONFIG_FILES([Makefile
		 src/Makefile
		 plugins/plugin_gstreamer/Makefile
		 ui/gtk/Makefile])

AC_CONFIG_MACRO_DIR([m4])

AC_DISABLE_STATIC
AC_PROG_LIBTOOL

AC_ARG_WITH(multimedia-plugin-name,
	[  --with-multimedia-plugin-name  gstreamer or ffmpeg],
	[], [multimedia_plugin_name=gstreamer])
AC_ARG_WITH(plugin-name,
	[  --with-ui-plugin-name  gtk or Qt],
	[], [ui_plugin_name=gtk])

PKG_CHECK_MODULES(GLIB, [glib-2.0 >= 2.32])
AS_IF([test "x$multimedia_plugin_name" = "xgstreamer"],
	[PKG_CHECK_MODULES(GSTREAMER, [gstreamer-1.0 >= 1.0])],
	[MULTIMEDIA_LIBS="-L plugins/plugin_gstreamer/.libs"]
	)
PKG_CHECK_MODULES(GTK, [gtk+-2.0 >= 2.18])

AC_SUBST(GSTREAMER_CFLAGS)
AC_SUBST(GSTREAMER_LIBS)
AC_SUBST(GSTREAMER_DEPS)

AC_OUTPUT

echo \
"-------------------------------------------------

 ${PACKAGE_NAME} Version ${PACKAGE_VERSION}

 Prefix: '${prefix}'.
 Compiler: '${CC} ${CFLAGS} ${CPPFLAGS}'

 Now type 'make @<:@<target>@:>@'
   where the optional <target> is:
     all                - build all binaries
     install            - install everything

--------------------------------------------------"
