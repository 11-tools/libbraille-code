pkglib_LTLIBRARIES = text.la

text_la_CFLAGS = $(DEBUG_FLAGS) -I$(top_builddir)/include	\
-DBRLDRV_EXPORTS
text_la_SOURCES = brl.c
text_la_LDFLAGS = -no-undefined -module -release $(LT_RELEASE)	\
-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)		\
-L$(top_builddir)/lib -lbraille

EXTRA_DIST = text.dsp
MAINTAINERCLEANFILES = Makefile.in
