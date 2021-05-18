pkglib_LTLIBRARIES = braillenote.la

braillenote_la_CFLAGS = $(DEBUG_FLAGS) -I$(top_builddir)/include	\
-DBRLDRV_EXPORTS
braillenote_la_SOURCES = brl.c
braillenote_la_LDFLAGS = -no-undefined -module -release $(LT_RELEASE)	\
-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)			\
-L$(top_builddir)/lib -lbraille

EXTRA_DIST = braillenote.txt braillenote.dsp
MAINTAINERCLEANFILES = Makefile.in
