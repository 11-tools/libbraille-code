if DOC
HTMLDOC = tutorial.html index.html tutorial.css
else
HTMLDOC =
endif

CLEANFILES = tutorial.html tutorial.css labels.pl index.html
DISTCLEANFILES = libbraille.conf
MAINTAINERCLEANFILES = Makefile.in

sysconf_DATA = libbraille.conf
noinst_DATA = $(HTMLDOC)

EXTRA_DIST = FAQ libbraille.conf.in tutorial.tex

tutorial.html: tutorial.tex
	latex2html -no_subdir -no_navigation -split 0 -split +0 -link 0 -toc_depth 4 -info 0 -up_url ../doc.php -ascii_mode -up_title "Libbraille documentation" -address "Copyright 2004 S&eacute;bastien Sabl&eacute; <sable@users.sourceforge.net> - `date +%Y-%m-%d`<br><a href=\"http://sourceforge.net\"><img id="srcforge" src=\"http://sourceforge.net/sflogo.php?group_id=17127&amp;type=1\" style=\"width: 88px; height: 31px;\" alt=\"SourceForge Logo\"></a>" tutorial.tex
