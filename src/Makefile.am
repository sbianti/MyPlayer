bin_PROGRAMS = myplayer

myplayer_SOURCES = context.c ${top_srcdir}/include/context.h ${top_srcdir}/include/myp_plugin.h main.c playlist.c print.c

myplayer_LDADD = $(builtin_libadd) @GLIB_LIBS@ ${top_srcdir}/plugins/plugin_gstreamer/libplugin.la ${top_srcdir}/ui/gtk/libui.la
# $(MULTIMEDIA_LIBS) $(MULTIMEDIA_LD_FLAGS)

AM_CPPFLAGS = -Wall -I${top_srcdir}/include

AM_CFLAGS = $(GLIB_CFLAGS)	\
	    -include config.h

MAINTAINERCLEANFILES = Makefile.in \
	aclocal.m4 configure config.h.in config.sub config.guess \
	ltmain.sh depcomp compile missing install-sh mkinstalldirs
