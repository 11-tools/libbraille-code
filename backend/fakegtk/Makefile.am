if FAKEBACKEND
FAKELIB = fake.la
else
FAKELIB =
endif

pkglib_LTLIBRARIES = $(FAKELIB)

fake_la_CFLAGS = $(DEBUG_FLAGS) -I$(top_builddir)/include	\
'-DPKGDATADIR="$(pkgdatadir)"' $(GTK_CFLAGS) $(GLIB_CFLAGS)	\
-DBRLDRV_EXPORTS
fake_la_SOURCES = brl.c
fake_la_LIBADD = $(GTK_LIBS) $(GLIB_LIBS)
fake_la_LDFLAGS = -no-undefined -module -release $(LT_RELEASE)	\
-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)			\
-L$(top_builddir)/lib -lbraille

pkgdata_DATA = blank.png dots_b.png dots_w.png
EXTRA_DIST = blank.png dots_b.png dots_w.png
MAINTAINERCLEANFILES = Makefile.in
