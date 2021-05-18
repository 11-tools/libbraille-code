pkglib_LTLIBRARIES = handytech.la

handytech_la_CFLAGS = $(DEBUG_FLAGS) -I$(top_builddir)/include	\
-DBRLDRV_EXPORTS
handytech_la_SOURCES = brl.c
handytech_la_LDFLAGS = -module -release $(LT_RELEASE) -version-info	\
$(LT_CURRENT):$(LT_REVISION):$(LT_AGE) -L$(top_builddir)/lib -lbraille

EXTRA_DIST = handytech.dsp handytech.txt
MAINTAINERCLEANFILES = Makefile.in
