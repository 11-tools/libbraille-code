if ENABLE_USB
PAPENMEIERLIB = papenmeierusb.la
else
PAPENMEIERLIB =
endif

pkglib_LTLIBRARIES = $(PAPENMEIERLIB)

papenmeierusb_la_CFLAGS = $(DEBUG_FLAGS) -I$(top_builddir)/include	\
-DBRLDRV_EXPORTS
papenmeierusb_la_SOURCES = brl.c
papenmeierusb_la_LDFLAGS = -no-undefined -module -release $(LT_RELEASE)	\
-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)			\
-L$(top_builddir)/lib -lbraille -lusb

EXTRA_DIST = papenmeierusb.txt papenmeierusb.dsp
MAINTAINERCLEANFILES = Makefile.in
