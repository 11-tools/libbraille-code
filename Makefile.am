if PYTHONWRAPPER
PYTHONSUBDIR = python
else
PYTHONSUBDIR =
endif

if JAVAWRAPPER
JAVASUBDIR = java
else
JAVASUBDIR =
endif

SUBDIRS = libltdl include lib backend tables doc scripts test VisualC6	\
$(PYTHONSUBDIR) $(JAVASUBDIR)

EXTRA_DIST =
CLEANFILES = 
DISTCLEANFILES = config.h config.log config.status libtool stamp-h1
MAINTAINERCLEANFILES = Makefile.in aclocal.m4 build-stamp compile	\
config.guess config.h.in config.sub configure configure-stamp		\
config.cross.cache depcomp install-sh ltmain.sh missing mkinstalldirs	\
py-compile
