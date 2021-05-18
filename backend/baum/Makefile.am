if ENABLE_USB
BAUMLIB = baum.la
else
BAUMLIB =
endif

pkglib_LTLIBRARIES = $(BAUMLIB)

baum_la_CFLAGS = $(DEBUG_FLAGS) -I$(top_builddir)/include	\
-DBRLDRV_EXPORTS
baum_la_SOURCES = brl.c
baum_la_LDFLAGS = -no-undefined -module -release $(LT_RELEASE)	\
-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)			\
-L$(top_builddir)/lib -lbraille -lusb

EXTRA_DIST = baum.txt baum.dsp
MAINTAINERCLEANFILES = Makefile.in
