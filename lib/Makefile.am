if WIN32
EXTRASOURCES = serial_win32.c config_win32.c
else
EXTRASOURCES = serial_tios.c config_posix.c
endif

MAINTAINERCLEANFILES = Makefile.in

lib_LTLIBRARIES = libbraille.la

libbraille_la_CFLAGS = $(DEBUG_FLAGS) -I$(top_srcdir)/include	\
-I$(top_srcdir)/libltdl '-DSYSCONFDIR="$(sysconfdir)"'		\
'-DPKGDATADIR="$(pkgdatadir)"' '-DPKGLIBDIR="$(pkglibdir)"'	\
'-DPREFIX="$(prefix)"' $(USB_FLAGS) -DLIBBRAILLE_EXPORTS
libbraille_la_SOURCES = braille.c error.c config.c $(EXTRASOURCES)
libbraille_la_LIBADD = $(LIBLTDL) $(LIBADD_DL)
libbraille_la_LDFLAGS = -no-undefined -release $(LT_RELEASE)		\
-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) $(DLL_LDFLAGS)	\
$(USB_LIB)

EXTRA_DIST = dllentry.c libbraille.dsp serial_win32.c config_win32.c	\
serial_tios.c config_posix.c