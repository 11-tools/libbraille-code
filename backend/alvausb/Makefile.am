if ENABLE_USB
ALVALIB = alvausb.la
else
ALVALIB =
endif

pkglib_LTLIBRARIES = $(ALVALIB)

alvausb_la_CFLAGS = $(DEBUG_FLAGS) -I$(top_builddir)/include	\
-DBRLDRV_EXPORTS
alvausb_la_SOURCES = brl.c
alvausb_la_LDFLAGS = -no-undefined -module -release $(LT_RELEASE)	\
-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)			\
-L$(top_builddir)/lib -lbraille -lusb

EXTRA_DIST = alvausb.txt alvausb.dsp
MAINTAINERCLEANFILES = Makefile.in
