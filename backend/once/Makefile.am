pkglib_LTLIBRARIES = once.la

once_la_CFLAGS = $(DEBUG_FLAGS) -I$(top_builddir)/include	\
-DBRLDRV_EXPORTS
once_la_SOURCES = brl.c
once_la_LDFLAGS = -no-undefined -module -release $(LT_RELEASE)	\
-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)		\
-L$(top_builddir)/lib -lbraille

EXTRA_DIST = once.dsp once.txt
MAINTAINERCLEANFILES = Makefile.in