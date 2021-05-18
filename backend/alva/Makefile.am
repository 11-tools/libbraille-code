pkglib_LTLIBRARIES = alva.la

alva_la_CFLAGS = $(DEBUG_FLAGS) -I$(top_builddir)/include	\
-DBRLDRV_EXPORTS
alva_la_SOURCES = brl.c
alva_la_LDFLAGS = -no-undefined -module -release $(LT_RELEASE)	\
-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)		\
-L$(top_builddir)/lib -lbraille

EXTRA_DIST = alva.dsp alva.txt
MAINTAINERCLEANFILES = Makefile.in
