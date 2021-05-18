if ENABLE_USB
VOYAGERLIB = voyager.la
else
VOYAGERLIB =
endif

pkglib_LTLIBRARIES = $(VOYAGERLIB)

voyager_la_CFLAGS = $(DEBUG_FLAGS) -I$(top_builddir)/include	\
-DBRLDRV_EXPORTS
voyager_la_SOURCES = brl.c
voyager_la_LDFLAGS = -no-undefined -module -release $(LT_RELEASE)	\
-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)			\
-L$(top_builddir)/lib -lbraille -lusb

EXTRA_DIST = voyager.txt voyager.dsp
MAINTAINERCLEANFILES = Makefile.in
