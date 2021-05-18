nodist_python_PYTHON = braille.py
pyexec_LTLIBRARIES = _braille.la

_braille_la_CFLAGS = @DEBUG_FLAGS@ -I$(top_srcdir)/include @PY_CFLAGS@	\
'-DSYSCONFDIR="$(sysconfdir)"' '-DPKGDATADIR="$(pkgdatadir)"'		\
'-DPKGLIBDIR="$(pkglibdir)"'
nodist__braille_la_SOURCES = braille_wrap.c
_braille_la_LIBADD = -L$(top_srcdir)/lib -lbraille
_braille_la_LDFLAGS = -module

BUILT_SOURCES = braille_wrap.c braille.py
EXTRA_DIST = braille.i Python.dsp
CLEANFILES = braille.py braille_wrap.c braille_wrap.doc
MAINTAINERCLEANFILES = Makefile.in

braille_wrap.c braille.py: braille.i
	@SWIG@ -I$(top_srcdir)/include -python braille.i
