#!/bin/sh
#
echo "Generating build information using aclocal, automake and autoconf"
echo "This may take a while ..."

# Regenerate configuration files
rm -rf libltdl
libtoolize --force --copy --ltdl

cd libltdl
aclocal
autoheader
automake --copy
autoconf

cd ..
aclocal
autoheader
automake --add-missing --include-deps --copy
autoconf

# Run configure for this platform
#conf_flags="--enable-maintainer-mode --enable-debug "
#$conf_flags
#./configure $*
echo "Now you are ready to run ./configure"
