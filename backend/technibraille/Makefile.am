pkglib_LTLIBRARIES = technibraille.la

technibraille_la_CFLAGS = $(DEBUG_FLAGS) -I$(top_builddir)/include	\
-DBRLDRV_EXPORTS
technibraille_la_SOURCES = brl.c
technibraille_la_LDFLAGS = -no-undefined -module -release		\
$(LT_RELEASE) -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)	\
-L$(top_builddir)/lib -lbraille

EXTRA_DIST = technibraille.dsp technibraille.txt
MAINTAINERCLEANFILES = Makefile.in