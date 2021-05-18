pkglib_LTLIBRARIES = none.la

none_la_CFLAGS = $(DEBUG_FLAGS) -I$(top_builddir)/include	\
-DBRLDRV_EXPORTS
none_la_SOURCES = brl.c
none_la_LDFLAGS = -no-undefined -module -release $(LT_RELEASE)	\
-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)			\
-L$(top_builddir)/lib -lbraille

EXTRA_DIST = none.dsp none.txt
MAINTAINERCLEANFILES = Makefile.in