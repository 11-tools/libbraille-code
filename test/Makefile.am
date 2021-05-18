if PYTHONWRAPPER
PYTHONTEST = test_libbraille.py
else
PYTHONTEST =
endif

bin_PROGRAMS = test_libbraille
check_PROGRAMS = test_libbraille
check_SCRIPT = $(PYTHONTEST)
## check_JAVA = test_libbraille.java

test_libbraille_CFLAGS = $(DEBUG_FLAGS) -I$(top_srcdir)/include
test_libbraille_SOURCES = test_libbraille.c
test_libbraille_LDFLAGS = -L$(top_srcdir)/lib -lbraille $(LIBLTDL)	\
$(LIBADD_DL)

EXTRA_DIST = test_libbraille.java test_libbraille.py SimpleTest.dsp

TESTS = test_libbraille $(PYTHONTEST)

MAINTAINERCLEANFILES = Makefile.in
