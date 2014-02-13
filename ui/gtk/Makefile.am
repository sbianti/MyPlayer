AM_CFLAGS = $(GTK_CFLAGS) -I${top_srcdir}/include/	\
	    -include config.h

lib_LTLIBRARIES = libui.la

libui_la_SOURCES = gtk.c ${top_srcdir}/include/common.h ${top_srcdir}/include/myp_ui.h ${top_srcdir}/include/print.h

libui_la_LIBADD = @GTK_LIBS@
