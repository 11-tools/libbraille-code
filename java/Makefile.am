noinst_DATA = jbraille.jar
noinst_LTLIBRARIES = libjbraille.la

libjbraille_la_CFLAGS = @DEBUG_FLAGS@ -I$(top_srcdir)/include	\
@JAVA_INC@ @JAVA_INC_NATIVE@
nodist_libjbraille_la_SOURCES = jbraille_wrap.c
libjbraille_la_LIBADD = -L$(top_srcdir)/lib -lbraille
libjbraille_la_LDFLAGS = -no-undefined --export-all-symbols	\
@JNI_LDFLAGS@ -module -version-info				\
$(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

BUILT_SOURCES = jbraille_wrap.c jbraille.java
EXTRA_DIST = jbraille.i
CLEANFILES = jbraille.jar jbraille.class jbrailleJNI.java brl_key.java	\
jbraille_wrap.c jbrailleJNI.class brl_key.class jbraille_wrap.doc	\
jbraille.java jbraille.la
MAINTAINERCLEANFILES = Makefile.in

jbraille.java jbraille_wrap.c: jbraille.i
	@SWIG@ -java -I$(top_srcdir)/include @JAVA_INC@ @JAVA_INC_NATIVE@ jbraille.i

jbraille.jar: jbraille_wrap.c
	javac jbraille.java
	jar cf jbraille.jar jbraille.class