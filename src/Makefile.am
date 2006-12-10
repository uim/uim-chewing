uim_plugin_libdir = @UIM_LIBDIR@/uim/plugin

uim_plugin_lib_LTLIBRARIES = libuim-chewing.la
libuim_chewing_la_SOURCES = chewing.c keytab.h
libuim_chewing_la_LDFLAGS = -rpath $(uim_plugin_libdir) -avoid-version -module @UIM_LIBS@ @CHEWING_LIBS@
libuim_chewing_la_CFLAGS = -I$(top_srcdir) @UIM_CFLAGS@ @CHEWING_CFLAGS@
