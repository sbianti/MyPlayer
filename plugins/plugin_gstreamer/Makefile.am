AM_CFLAGS = $(GSTREAMER_CFLAGS) -I${top_srcdir}/include/	\
	    -include config.h

lib_LTLIBRARIES = libplugin.la

libplugin_la_SOURCES = gstreamer.c ${top_srcdir}/include/myp_plugin.h ${top_srcdir}/include/print.h

libplugin_la_LIBADD = @GSTREAMER_LIBS@
libplugin_la_LDFLAGS = `pkg-config --libs gstreamer-video-1.0 gstreamer-pbutils-1.0`
