pkglib_LTLIBRARIES = eurobraille.la

eurobraille_la_CFLAGS = $(DEBUG_FLAGS) -I$(top_builddir)/include	\
-DBRLDRV_EXPORTS
eurobraille_la_SOURCES = brl.c
eurobraille_la_LDFLAGS = -no-undefined -module -release $(LT_RELEASE)	\
-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)			\
-L$(top_builddir)/lib -lbraille

EXTRA_DIST = eurobraille.dsp eurobraille.txt
MAINTAINERCLEANFILES = Makefile.in