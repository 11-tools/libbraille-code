pkglib_LTLIBRARIES = braillelite.la

braillelite_la_CFLAGS = $(DEBUG_FLAGS) -I$(top_builddir)/include	\
-DBRLDRV_EXPORTS
braillelite_la_SOURCES = brl.c
braillelite_la_LDFLAGS = -no-undefined -module -release $(LT_RELEASE)	\
-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)			\
-L$(top_builddir)/lib -lbraille

EXTRA_DIST = braillelite.txt braillelite.dsp
MAINTAINERCLEANFILES = Makefile.in