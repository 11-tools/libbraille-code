dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.50)

AC_INIT(libbraille, 0.19.0)
AC_CONFIG_SRCDIR(lib/braille.c)
AM_INIT_AUTOMAKE

# Library version
#
# Making releases:
#   LIBMICRO_VERSION += 1;
#   INTERFACE_AGE += 1;
#   BINARY_AGE += 1;
# if any functions have been added, set INTERFACE_AGE to 0.
# if backwards compatibility has been broken,
# set BINARY_AGE and INTERFACE_AGE to 0.
#
LIBRELEASE_VERSION=0
LIBMICRO_VERSION=18
INTERFACE_AGE=1
BINARY_AGE=4
AC_SUBST(INTERFACE_AGE)
AC_SUBST(BINARY_AGE)

dnl libtool versioning
LT_RELEASE=$LIBRELEASE_VERSION
LT_CURRENT=`expr $LIBMICRO_VERSION - $INTERFACE_AGE`
LT_REVISION=$INTERFACE_AGE
LT_AGE=`expr $BINARY_AGE - $INTERFACE_AGE`    
AC_SUBST(LT_RELEASE)
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)    

dnl generate config.h
AM_CONFIG_HEADER(config.h)

# We now have a list of macros which tell autoconf what tools we need to
# build our software, in this case "make", a C compiler, and "install".
AC_PROG_MAKE_SET
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_EXEEXT

dnl We need LIBTOOL to use shared libraries
AC_EXEEXT
AC_LIBTOOL_DLOPEN
AC_LIBLTDL_CONVENIENCE
AC_PROG_LIBTOOL
AC_CONFIG_SUBDIRS(libltdl)
AC_SUBST(LTDLINCL)
AC_SUBST(LIBLTDL)
AC_SUBST(LIBADD_DL)

dnl Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h sys/ioctl.h sys/time.h sys/select.h syslog.h unistd.h string.h termios.h windows.h)

AM_CONDITIONAL(WIN32, test x$ac_cv_header_windows_h = xyes)

AC_MSG_CHECKING([for device format])
if test "$ac_cv_header_termios_h" = yes; then
  AC_MSG_RESULT([posix])
  DEVICE_STR="/dev/ttyS0"
elif test "$ac_cv_header_windows_h" = yes; then
  AC_MSG_RESULT([windows])
  DEVICE_STR="COM1"
  JNI_LDFLAGS="-Wl,--add-stdcall-alias"
  DLL_LDFLAGS="-Wl,--add-stdcall-alias -Wl,--kill-at -mms-bitfields"
else
   AC_MSG_ERROR([unrecognized system])
fi
AC_SUBST(DEVICE_STR)
AC_SUBST(JNI_LDFLAGS)
AC_SUBST(DLL_LDFLAGS)

dnl Display debug information
AC_ARG_ENABLE(debug,
[  --enable-debug          enable debug information [[default=no]]],
              , enable_debug=no)
if test x$ac_cv_prog_gcc = xyes; then
    if test x$enable_debug = xyes; then
        DEBUG_FLAGS="$DEBUG_FLAGS -DDEBUG -Wall"
    fi
fi
AC_SUBST(DEBUG_FLAGS)

AC_ARG_ENABLE(fake,
[  --enable-fake           turn on fake terminal [[default=no]]],
[case "${enableval}" in
  yes)  fake=true
	AM_PATH_GLIB_2_0(2.0.0, have_glib=yes,
				AC_MSG_ERROR(fake option requires glib)
				have_glib=no, gthread)
	AM_PATH_GTK_2_0(2.0.0, have_gtk=yes,
			       AC_MSG_ERROR(fake option requires gtk)
			       have_gtk=no)
        ;;
  no)	fake=false ;;
  *)	AC_MSG_ERROR(bad value ${enableval} for --enable-fake) ;;
esac],[fake=false])
AM_CONDITIONAL(FAKEBACKEND, test x$fake = xtrue)

AC_ARG_ENABLE(usb,
[  --enable-usb           turn on usb support [[default=yes]]],
	,[enable_usb=yes])
if test x$enable_usb = xyes; then
	dnl Check for libusb
	AC_CHECK_PROG(LIBUSBCONFIG, libusb-config, true, false)
    	if test "$LIBUSBCONFIG" = false; then
       	   	AC_MSG_ERROR([libusb-config not found in $PATH])
    	fi;
	OLDFLAGS=$LDFLAGS
	LDFLAGS="$LDFLAGS `libusb-config --libs`"
	AC_CHECK_HEADERS([usb.h],
        	[AC_CHECK_LIB(usb, usb_interrupt_read,
			[USB_FLAGS="`libusb-config --cflags`"]
                      	[USB_LIB="`libusb-config --libs`"],
                      	[AC_MSG_ERROR([libusb >= 0.1.8 is needed])]
        		)],
        	[AC_MSG_ERROR([cannot find libusb headers])]
	)
	LDFLAGS=$OLDFLAGS
fi
AM_CONDITIONAL(ENABLE_USB, test x$enable_usb = xyes)
AC_SUBST([USB_LIB])
AC_SUBST([USB_FLAGS])

AC_ARG_ENABLE(doc,
[  --enable-doc           turn on HTML documentation generation [[default=no]]],
[case "${enableval}" in
  yes)  doc=true
    AC_PATH_PROG(LATEX2HTML, latex2html$EXEEXT, nocommand)
    if test "$LATEX2HTML" = nocommand; then
       AC_MSG_ERROR([latex2html not found in $PATH])
    fi;
    ;;
  no)	doc=false ;;
  *)	AC_MSG_ERROR(bad value ${enableval} for --enable-fake) ;;
esac],[fake=false])
AM_CONDITIONAL(DOC, test x$doc = xtrue)

dnl detect python if option --enable-python is set
AC_ARG_ENABLE(python,
[  --enable-python         enable python wrapper for the platform [[default=no]]],
              , enable_python=no)
if test x$enable_python = xyes; then
    AC_PATH_PROG(SWIG, swig$EXEEXT, nocommand)
    if test "$SWIG" = nocommand; then
       AC_MSG_ERROR([swig not found in $PATH])
    fi;
    AM_PATH_PYTHON()

    dmod="distutils.sysconfig"
    AC_MSG_CHECKING(for Python $dmod module)
    if $PYTHON -c "import $dmod" >/dev/null 2>&1; then
        AC_MSG_RESULT(found)
    else
        AC_MSG_RESULT(not found)
        AC_MSG_ERROR([** Please install Python distutils modules **])
    fi

    AC_ARG_WITH(pythoninc,
		AC_HELP_STRING([--with-pythoninc],
			       [Select path for python headers]),
	    	pythoninc=${withval},
		AC_MSG_CHECKING(for Python include dir)
		pythoninc=`$PYTHON -c "import $dmod; print $dmod.get_python_inc()"`
		AC_MSG_RESULT($pythoninc)
		)
    PY_CFLAGS="-I$pythoninc"
fi
AM_CONDITIONAL(PYTHONWRAPPER, test x$enable_python = xyes)
AC_SUBST(PY_CFLAGS)

dnl detect java if option --enable-java is set
AC_ARG_ENABLE(java,
[  --enable-java           enable java wrapper for the platform [[default=no]]],
              , enable_java=no)
if test x$enable_java = xyes; then
    AC_PATH_PROG(SWIG, swig$EXEEXT, nocommand)
    if test "$SWIG" = nocommand; then
        AC_MSG_ERROR([swig not found in $PATH])
    fi;

    AC_PATH_PROG(javapath, javac$EXEEXT, nocommand)
    if test "$javapath" = nocommand ; then
        AC_MSG_ERROR([** Java not installed or broken **])
    fi

    AC_ARG_WITH(javainc,
		AC_HELP_STRING([--with-javainc],
			       [Select path for java headers]),
	    	javainc=${withval},
		AC_MSG_ERROR([you must specify javainc]))	
    JAVA_INC="-I$javainc"

    AC_ARG_WITH(javaincnative,
		AC_HELP_STRING([--with-javaincnative],
			       [Select path for native java headers]),
	    	javaincnative=${withval},
		AC_MSG_ERROR([you must specify javaincnative]))	
    JAVA_INC_NATIVE="-I$javaincnative"
fi
AC_SUBST(JAVA_INC)
AC_SUBST(JAVA_INC_NATIVE)
AM_CONDITIONAL(JAVAWRAPPER, test x$enable_java = xyes)

# Finally create all the generated files
# The configure script takes "file.in" and substitutes variables to produce
# "file".  In this case we are just generating the Makefiles, but this could
# be used to generate any number of automatically generated files.
AC_CONFIG_FILES(
Makefile
include/Makefile
lib/Makefile
backend/Makefile
backend/alva/Makefile
backend/alvausb/Makefile
backend/baum/Makefile
backend/braillelite/Makefile
backend/braillenote/Makefile
backend/eurobraille/Makefile
backend/fakegtk/Makefile
backend/fakewin/Makefile
backend/handytech/Makefile
backend/none/Makefile
backend/once/Makefile
backend/papenmeierusb/Makefile
backend/technibraille/Makefile
backend/text/Makefile
backend/voyager/Makefile
doc/Makefile
tables/Makefile
scripts/Makefile
scripts/install.nsi
scripts/libbraille.spec
doc/libbraille.conf
python/Makefile
java/Makefile
test/Makefile
VisualC6/Makefile
VisualC6/config.h
)
AC_OUTPUT
