# This is free software, licensed under the GNU Public License V2.
# See the file COPYING for details.

AC_INIT(include/pi-version.h.in)
AC_CONFIG_MACRO_DIR([m4])

dnl Initialize maintainer mode
AM_MAINTAINER_MODE

dnl Run all of the sane intltool tests
dnl IT_PROG_INTLTOOL

AC_PREFIX_DEFAULT(/usr/local)

dnl ******************************
dnl Limit to a specific ac version
dnl ******************************
AC_PREREQ(2.53)
AM_CONFIG_HEADER(config.h)
AM_CONFIG_HEADER(include/pi-md5.h)

dnl ******************************
dnl Pilot Link Version
dnl ******************************
PILOT_LINK_VERS=0
PILOT_LINK_MAJOR=12
PILOT_LINK_MINOR=5
PILOT_LINK_PATCH=

AC_SUBST(PILOT_LINK_VERS)
AC_SUBST(PILOT_LINK_MAJOR)
AC_SUBST(PILOT_LINK_MINOR)
AC_SUBST(PILOT_LINK_PATCH)
VERSION="$PILOT_LINK_VERS.$PILOT_LINK_MAJOR.$PILOT_LINK_MINOR$PILOT_LINK_PATCH"

dnl ******************************
dnl Pilot Link Library Version
dnl libpisock.so.CURRENT.AGE.REVISION
dnl Please see the following URL for details:
dnl http://sources.redhat.com/autobook/autobook/autobook_91.html
dnl ******************************
PISOCK_CURRENT=9
PISOCK_REVISION=2
PISOCK_AGE=0

AC_SUBST(PISOCK_CURRENT)
AC_SUBST(PISOCK_REVISION)
AC_SUBST(PISOCK_AGE)


dnl ******************************
dnl Pilot Link Sync Library Version
dnl libpisync.so
dnl ******************************
PISYNC_CURRENT=1
PISYNC_REVISION=3
PISYNC_AGE=0

AC_SUBST(PISYNC_CURRENT)
AC_SUBST(PISYNC_REVISION)
AC_SUBST(PISYNC_AGE)

dnl Eat -Werror so configure will run properly, if the user provided it
enable_werror=no
save_CFLAGS="$CFLAGS"
CFLAGS=`echo $save_CFLAGS | sed -e s/-Werror//g`
CXXFLAGS=`echo $save_CXXFLAGS | sed -e s/-Werror//g`
if test "x$CFLAGS" != "x$save_CFLAGS"; then
	dnl -Werror was set; treat it as implicit --enable-werror below
	enable_werror="yes"
fi


dnl Check host type
AC_CANONICAL_HOST
AC_CANONICAL_SYSTEM
AC_DEFINE_UNQUOTED(HOST_OS, "$host", [Host type])

AM_INIT_AUTOMAKE(pilot-link, [$VERSION])


dnl ******************************
dnl Checks for basic programs.
dnl ******************************
AC_PROG_CC
AC_PROG_INSTALL(install ginstall)
AC_PROG_F77(g77 g77-3.2 g77-3.3 fl32 f77 fort77 xlf f90 xlf90)
AM_PROG_LIBTOOL
AC_PROG_YACC


dnl ******************************
dnl Defaults for GCC
dnl ******************************
if test "x$GCC" = "xyes"; then
	CFLAGS=${CFLAGS:-"-g2 -Wall"}
fi


dnl ******************************
dnl Platform-specific things
dnl ******************************

dnl PIC_LIBS is flags needed to compile PIC, for shared libs
dnl where some linker offsets are not allowed. Currently set
dnl for FreeBSD-amd64 only.
PIC_LIBS=""


dnl null_device is the default NULL device on your system
dnl (usually /dev/null or NUL). If yours is _not_ /dev/null,
dnl set it in the platform-specific section below.
null_device="/dev/null"

case "$host" in
	*darwin*)
		dnl define MACOSX for code use
		CPPFLAGS="$CPPFLAGS -DMACOSX"
		dnl Use fink under MacOS X
		AC_MSG_CHECKING(for fink support)
		if test -d "/sw/lib" -a -d "/sw/include"; then
			CPPFLAGS="$CPPFLAGS -I/sw/include"
			LDFLAGS="$LDFLAGS -L/sw/lib"
			AC_MSG_RESULT(yes)
		else
			AC_MSG_RESULT(no)
		fi
    ;;
	amd64*freebsd*)
		dnl Need -fPIC for shared libs
		PIC_LIBS="-fPIC"
	;;
	*solaris*)
		dnl Check if we need -lresolv for inet_pton
		AC_CHECK_FUNC(inet_pton,,
			[AC_CHECK_LIB(resolv,inet_pton,LIBS="-lresolv $LIBS")])
		AC_CHECK_FUNC(pow,,
			[AC_CHECK_LIB(m,pow,LIBS="-lm $LIBS")])
	;;
	*)
	;;
esac
AC_DEFINE_UNQUOTED(NULL_DEVICE, "$null_device",
		   [Your system's bitbucket (usually /dev/null or NUL)])
AC_SUBST(PIC_LIBS)

dnl ******************************
dnl Check if this was CVS source
dnl ******************************
if test -d "${srcdir}/CVS" ; then
       PL_FROM_CVS=yes
fi
AM_CONDITIONAL(CVS_BUILD,[test -d "${srcdir}/CVS"])

dnl ******************************
dnl Pilot Link Checks
dnl ******************************
AC_HEADER_STDC
AC_C_CONST
AC_HEADER_TIME
AC_STRUCT_TM
AC_PROG_GCC_TRADITIONAL
AC_TYPE_SIGNAL

AC_CHECK_HEADERS(
	dirent.h errno.h fcntl.h inttypes.h memory.h netdb.h 		\
	netinet/in.h regex.h stdint.h stdlib.h string.h strings.h	\
	sys/ioctl_compat.h sys/ioctl.h 	sys/malloc.h sys/select.h	\
	sys/sockio.h sys/time.h sys/utsname.h unistd.h IOKit/IOBSD.h)
AC_CHECK_HEADERS(ifaddrs.h inttypes.h)

AC_CHECK_FUNCS(
	atexit cfmakeraw cfsetispeed cfsetospeed cfsetspeed dup2 	\
	gethostname inet_aton malloc memcpy memmove putenv sigaction	\
	snprintf strchr strdup strtok strtoul strerror uname)

dnl Find optional libraries (borrowed from Tcl)
tcl_checkBoth=0
AC_CHECK_FUNC(connect, tcl_checkSocket=0, tcl_checkSocket=1)

if test "$tcl_checkSocket" = 1; then
        AC_CHECK_LIB(socket, main, LIBS="$LIBS -lsocket", tcl_checkBoth=1)
fi

if test "$tcl_checkBoth" = 1; then
        tk_oldLibs=$LIBS
        LIBS="$LIBS -lsocket -lnsl"
        AC_CHECK_FUNC(
                accept, tcl_checkNsl=0, [LIBS=$tk_oldLibs])
fi

AC_CHECK_FUNC(gethostbyname, , AC_CHECK_LIB(nsl, main, [LIBS="$LIBS -lnsl"]))
AC_CHECK_LIB(inet, main, [LIBS="$LIBS -linet"])

AC_CACHE_CHECK([for cispeed and cospeed members of struct termios],
	ac_cv_termios_cspeed,

	[AC_TRY_COMPILE(
		[#include <termios.h>], [int main(void) {
		 struct termios t;t.c_ispeed=t.c_ospeed=0;}],
			ac_cv_termios_cspeed=yes,ac_cv_termios_cspeed=no)])

if test $ac_cv_termios_cspeed = yes; then
	AC_DEFINE(TERMIOS_CSPEED, 1, [Define if we have termios(3) support])
fi

AC_CHECK_SOCKLEN_T

dnl ******************************
dnl pilot-link conduits
dnl ******************************
AC_MSG_CHECKING(whether to enable userland conduits)
AC_ARG_ENABLE(conduits,
	[  --enable-conduits       Build the userland conduits],
		[enable_conduits="yes"], [enable_conduits="no"],
)
AM_CONDITIONAL(ENABLE_CONDUITS,[test "x$enable_conduits" != "xno"])
AC_MSG_RESULT($enable_conduits)

dnl ******************************
dnl PNG Support
dnl ******************************
msg_png=no
if test "x$enable_conduits" != "xno"; then
	have_png=no

	AC_ARG_WITH(libpng,
		[  --with-libpng           Prefix where libpng is installed])

	case $with_libpng in
		/*)
		PNG_CFLAGS="-I$withval/include"
		PNG_LIBS="-L$withval/lib"
		;;
	esac

	save_CFLAGS="$CFLAGS"
	save_CPPFLAGS="$CPPFLAGS"
	save_LDFLAGS="$LDFLAGS"
	CFLAGS="$CFLAGS $PNG_CFLAGS"
	CPPFLAGS="$CPPFLAGS $PNG_CFLAGS"
	LDFLAGS="$LDFLAGS $PNG_LIBS"

	AC_CHECK_LIB(png, main, [have_png=yes; PNG_LIBS="$PNG_LIBS -lpng"],[have_png=no],[-lm])
	if test "$have_png" = yes; then
		AC_CHECK_HEADER(png.h, , have_png=no)
		if test "$have_png" = yes; then
			AC_MSG_CHECKING(for png setjmp support)
			AC_EGREP_CPP(yes,
			[#include <png.h>
			#ifdef PNG_SETJMP_SUPPORTED
				yes
			#endif], , have_png=no)
			AC_MSG_RESULT($have_png)
			if test "$have_png" = yes; then
				AC_DEFINE(HAVE_PNG, 1, [Define if we have PNG support])
				have_png=yes
				msg_png=yes
			fi
		fi
	fi

	CFLAGS="$save_CFLAGS"
	CPPFLAGS="$save_CPPFLAGS"
	LDFLAGS="$save_LDFLAGS"
else
	have_png="none"
	PNG_CFLAGS=
	PNG_LIBS=
fi

AC_SUBST(PNG_CFLAGS)
AC_SUBST(PNG_LIBS)


dnl ******************************
dnl Threading support
dnl ******************************
msg_threads=no
PILOT_LINK_THREADS_SUPPORT
if $use_threads; then
	msg_threads=yes
fi


dnl ***************************************
dnl Test if building USB Support
dnl for Linux, FreeBSD or Darwin (Mac OS X)
dnl ***************************************
have_libusb=no
use_libusb=no
usb_type=
usb_libs=

AC_ARG_ENABLE(libusb,
	      AC_HELP_STRING([--enable-libusb         Enable usage of libusb]),
	      if test "x$enableval" != "xno" ; then
	          AC_CHECK_LIB(usb, usb_open, [have_libusb=yes;],[have_libusb=no])
	      fi
	      )
if test "$have_libusb" = yes; then
	AC_DEFINE(HAVE_USB, 1, [Define if we have USB support])
	usb_type=libusb
	msg_usb="yes, libusb"
	case "$host" in
		*darwin*)
			usb_libs="-lusb -Wl,-framework,IOKit,-framework,CoreFoundation"
		;;
		*)
			if test "$msg_threads" = "no"; then
			    usb_libs="-lusb -lpthread"
			else
			    usb_libs="-lusb"
			fi
		;;
	esac
else
	case "$host" in
		*linux*)
			AC_DEFINE(HAVE_USB, 1, [Define if we have USB support])
			usb_type=linux
			msg_usb="yes, Linux"
		;;
		*freebsd*)
			AC_DEFINE(HAVE_USB, 1, [Define if we have USB support])
			usb_type=freebsd
			msg_usb="yes, FreeBSD"
		;;
	    *darwin*)
		    AC_DEFINE(HAVE_USB, 1, [Define if we have USB support])
		    usb_type=darwin
		    msg_usb="yes, Darwin"
		    usb_libs="-Wl,-framework,IOKit,-framework,CoreFoundation"
	    ;;
	esac
fi
AM_CONDITIONAL(WITH_USB, test x$usb_type)
AM_CONDITIONAL(WITH_LIBUSB, test x$usb_type = xlibusb)
AM_CONDITIONAL(WITH_LINUXUSB, test x$usb_type = xlinux)
AM_CONDITIONAL(WITH_FREEBSDUSB, test x$usb_type = xfreebsd)
AM_CONDITIONAL(WITH_DARWINUSB, test x$usb_type = xdarwin)
AC_SUBST(usb_libs)

dnl ******************************
dnl BlueZ Support
dnl ******************************
have_bluez=no
use_bluez=auto

AC_ARG_WITH(bluez,
		AC_HELP_STRING([--with-bluez],
			[Enable use of BlueZ]),
		[use_bluez="$withval"])
if test "$use_bluez" != "no" ; then
	PKG_CHECK_MODULES([BLUEZ],[bluez],[
		have_bluez=yes
		use_bluez=yes
	],[
		AC_MSG_RESULT([no pkgconfig package])
		AC_BLUEZ([
			have_bluez=yes
			use_bluez=yes
		],[
			test "$use_bluez" != "no" -a "$use_bluez" != "auto" \
			&& AC_MSG_ERROR([BlueZ explicitly requested, but no library found]) 
			use_bluez=no
			have_bluez=no
		])
	])
fi

if test "$use_bluez" = "yes" ; then
	AC_DEFINE([HAVE_BLUEZ],[1],[Define if we build with BlueZ])
	AC_SUBST([BLUEZ_CFLAGS])
	AC_SUBST([BLUEZ_LIBS])
fi

AM_CONDITIONAL([WITH_BLUEZ],[test "$use_bluez" = "yes"])



dnl *********************************
dnl Check for popt (use internal if needed)
dnl *********************************
AC_ARG_WITH(included-popt,
[  --with-included-popt    use bundled popt library, not from system],,
		[with_included_popt="auto"]
)

if test "x$enable_conduits" != "xno"; then
	if test "x$with_included_popt" != "xyes"; then
		dnl Determine if system popt is good enough
		save_LIBS="$LIBS"
		AC_CHECK_HEADER(popt.h,
			AC_CHECK_DECL(POPT_BIT_SET,
				AC_CHECK_LIB(popt, poptStrippedArgv,,
					[with_included_popt="yes"]),
				[with_included_popt="yes"],
				[#include <popt.h>]),
			[with_included_popt="yes"]
		)
		LIBS="$save_LIBS"
	fi

	AC_MSG_CHECKING(whether to use included popt)
	if test "x$with_included_popt" = "xyes"; then
		msg_popt="yes (internal)"
	    AC_MSG_RESULT(yes)
		POPT_LIBS='$(top_builddir)/popt/libpopt.la'
	    POPT_INCLUDES='-I$(top_srcdir)/popt'
	else
	    AC_MSG_RESULT(no)
		msg_popt="yes (system)"
	    POPT_LIBS="-lpopt"
	fi
else
	with_included_popt="no"
	msg_popt="none"
	POPT_LIBS=
	POPT_INCLUDES=
fi
AM_CONDITIONAL(INCLUDED_POPT, test "x$with_included_popt" = "xyes")
AC_SUBST(POPT_LIBS)
AC_SUBST(POPT_INCLUDES)


dnl ******************************
dnl iconv checking
dnl ******************************
have_iconv=no

AC_ARG_WITH(libiconv, [  --with-libiconv         Prefix where libiconv is installed])

case $withval in
	/*)
		ICONV_CFLAGS="-I$withval/include"
		ICONV_LIBS="-L$withval/lib"
	;;
esac

save_CFLAGS="$CFLAGS"
save_LDFLAGS="$LDFLAGS"
CFLAGS="$CFLAGS $ICONV_CFLAGS"
LDFLAGS="$LDFLAGS $ICONV_LIBS"

AC_CHECK_LIB(
	iconv, libiconv, [have_iconv=yes; ICONV_LIBS="$ICONV_LIBS -liconv"], [
		AC_CHECK_FUNC(iconv, [
			AC_CHECK_FUNCS(gnu_get_libc_version)
			AC_MSG_CHECKING(if system iconv handles CP1252)
			AC_TRY_RUN([
#include <iconv.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_GNU_GET_LIBC_VERSION
#include <gnu/libc-version.h>
#endif

int main (int argc, char **argv)
{
	const char *pc = "\x66\x66\x66\x66\x66\x66\x66\xA9";
	const char *utf8 = "\x66\x66\x66\x66\x66\x66\x66\xC2\xA9";
	char transbuf[10], *trans = transbuf;
	iconv_t cd;
	size_t pc_len = strlen (pc), utf8_len = 10;
	size_t utf8_real_len = strlen (utf8);

	cd = iconv_open ("UTF-8", "CP1252");
	if (!cd)
		exit (1);
	if (iconv (cd, &pc, &pc_len, &trans, &utf8_len) == -1 || pc_len != 0)
		exit (1);
	if (memcmp (utf8, transbuf, utf8_real_len) != 0)
		exit (1);

	exit (0);
}], AC_MSG_RESULT(yes); have_iconv=yes, [AC_MSG_RESULT(no);], AC_MSG_RESULT(not cross compiling);)])])

dnl ##  Fixed AC_TRY_RUN warning, missing third argument. -DD 22/01/2002
dnl
dnl 	AC_TRY_RUN is a macro that's basically like an if-then-else structure. If
dnl 	the test program (whose text is the first argument) succeeds, the second
dnl 	argument is executed; if the test fails, the third argument is executed; and
dnl 	if autoconf knows it is cross-compiling for another machine and therefore
dnl 	can't run a test program at all, it doesn't try but just executes the fourth
dnl 	argument.  Usually that last alternative should be the most conservative
dnl 	assumption.

CFLAGS="$save_CFLAGS"
LDFLAGS="$save_LDFLAGS"

if test "$have_iconv" = yes; then
	AC_DEFINE(HAVE_ICONV, 1, [Define if we have libiconv support])
	msg_iconv=yes
else
	msg_iconv=no
fi
AC_SUBST(ICONV_CFLAGS)
AC_SUBST(ICONV_LIBS)

dnl ******************************
dnl readline checking
dnl ******************************
if test "x$enable_conduits" != "xno"; then
	VL_LIB_READLINE
else
	msg_readline="none"
fi

dnl ******************************
dnl Perl
dnl ******************************
use_perl=false
#msg_perl=no

AC_ARG_WITH(perl,    [  --with-perl=perlexec    use Perl                    [[default=no]]],
	, with_perl=no)
if test "x$with_perl" = "xno"; then
	AC_MSG_CHECKING(if perl was enabled)
	AC_MSG_RESULT(no)
	PERL=''
else
	if test "x$with_perl" != "xyes" && test "x$with_perl" != "x"; then
		PERL="$with_perl"
	else
		AC_CHECK_PROG(PERL, perl, perl)
	fi
fi

dnl Check if version of Perl is sufficient
PERL_MINVERSION="5.000_00"

if test "x$PERL" != "x"; then
	AC_MSG_CHECKING(for perl version greater than or equal to $PERL_MINVERSION)
	$PERL -e "exit ! eval 'use $PERL_MINVERSION; 1'"
	if test $? -ne 0; then
		AC_MSG_RESULT(no);
		use_perl=false
	else
		AC_MSG_RESULT(yes);
		PERL_VERSION=`$PERL -e 'print $]'`
		use_perl=true
	fi
fi

AM_CONDITIONAL(WITH_PERL, $use_perl)
#if $use_perl; then
#	msg_perl=yes
#fi
AC_SUBST(PERL)


dnl ******************************
dnl Java
dnl ******************************
use_java=false
# msg_java=no

AC_ARG_WITH(java,     [  --with-java=jdkbase     use Java                    [[default=no]]],
	, with_java=no)

if test "x$with_java" != "xno"; then
	AC_PATH_PROG(JAVAC, javac, , $with_java/bin:$PATH)
	AC_PATH_PROG(JAVA, java, , $with_java/bin:$PATH)
	AC_PATH_PROG(JAR, jar, ,$with_java/bin:$PATH)
fi

if test "x$JAVA" != "x" && test "x$JAVAC" != "x" && test "x$JAR" != "x"; then
	AC_MSG_CHECKING(for Java Development Kit)
	JAVABASE=`echo "$JAVAC" | sed -e "s/\/bin\/javac//"`
	if test -f "$JAVABASE/include/jni.h"; then
		use_java=true
		AC_MSG_RESULT(looks good)
	else
		AC_MSG_RESULT(not usable)
	fi
fi

AM_CONDITIONAL(WITH_JAVA, $use_java)


if $use_java; then
#	msg_java=yes
	JAVA_VERSION=`java -version 2>&1 | grep '^java version' | awk -F\" '{print $2}'`
	# " This is here to stop the errors from mismatched quotes. 
fi


AC_SUBST(JAVA)
AC_SUBST(JAVAC)
AC_SUBST(JAR)
AC_SUBST(JAVABASE)
AC_SUBST(JAVA_VERSION)


dnl ******************************
dnl TCL
dnl ******************************
PILOT_LINK_PATH_TCLCONFIG
if $use_tcl; then
	PILOT_LINK_LOAD_TCLCONFIG
	PILOT_LINK_PUBLIC_TCL_HEADERS
	PILOT_LINK_PROG_TCLSH
	PILOT_LINK_PROG_WISH
fi

#AC_SUBST(TCL_INCLUDES)
#AC_SUBST(TCL_LIB_FILE)
#AC_SUBST(TCL_LIB_FLAG)
#AC_SUBST(TCL_LIB_SPEC)
#AC_SUBST(TCL_LIBS)
#AC_SUBST(TCL_VERSION)
#AC_SUBST(TCLSH_PROG)
#AC_SUBST(WISH_PROG)

AM_CONDITIONAL(WITH_TCL, $use_tcl)


dnl ******************************
dnl Python
dnl ******************************
use_python=false
AC_ARG_WITH(python,   [  --with-python=pybase    use Python,                 [[default=no]]],
        , with_python=no)

if test "x$with_python" != "xno"; then
        AC_PATH_PROGS(PYTHON, [python])
        if test "x$PYTHON" != "x"; then
		AM_CHECK_PYTHON
	fi

	if test "x$PYTHON_H" = "xyes"; then
		AC_SUBST(PYTHON_VERSION)
		AC_SUBST(PYTHON_CFLAGS)
		AC_SUBST(PYTHON_LIBS)
		AC_SUBST(PYTHON_H)

		use_python=true
	else
		PYTHON_VERSION=''
	fi
fi

AM_CONDITIONAL(WITH_PYTHON, $use_python)
if $use_python; then
        msg_python=yes
fi

dnl *************************************
dnl ElectricFence (optional)
dnl *************************************
msg_efence=no
AC_ARG_WITH(efence, [  --with-efence           use ElectricFence,          [[default=no]]],
	, with_efence=no)

if test "x$with_efence" != "xno"; then
	AC_CHECK_LIB(efence, malloc, LIBS="$LIBS -lefence")
        if test "$ac_cv_lib_efence_malloc" = yes; then
                msg_efence=true
        else
                echo "Unable to locate ElectricFence malloc() debugging library!"
                echo "Please download from ftp://ftp.perens.com/pub/ElectricFence/"
                echo "You may also try omitting --with-efence when running configure."
                echo ""
                exit 1;
        fi
fi


dnl *************************************
dnl Runtime debugging
dnl *************************************
set_debug=yes
AC_ARG_ENABLE(debug,
	      [  --disable-debug         Shunt runtime debugging],
	      [set_debug=no])

if test "${enable_debug+set}" = set; then
	if test "$enable_debug" = yes; then
		CFLAGS="-g $CFLAGS"
		set_debug=yes
	fi
fi

echo -n "Runtime debugging is "
if test "$set_debug" = "yes"; then
	echo "enabled"
	AC_DEFINE(PI_DEBUG, 1, [Define if we have debugging enabled])
else
	echo "disabled"
	CFLAGS=`echo "$CFLAGS" | sed -e "s/-g //"`
fi


dnl *************************************
dnl Profiling information
dnl *************************************
AC_ARG_ENABLE(profile,
        [  --enable-profile        use gprof profiling [[default=no]]],
        CFLAGS="-pg $CFLAGS")

dnl *************************************
dnl Check for xsltproc
dnl *************************************
AM_CONDITIONAL(BUILD_XML,[test ! -d "{$srcdir}/CVS"])
# if test -d "${srcdir}/CVS" ; then

AC_ARG_ENABLE([xsltproc], [AC_HELP_STRING([--enable-xsltproc], 
	[Use xsltproc to build documentation [default=no]])],
	[enable_xsltproc="$enableval"],
[enable_xsltproc=no])
if test x"$enable_xsltproc" = x"yes"; then
  AC_PATH_PROG([XSLTPROC], [xsltproc], [no])
  if test x"$XSLTPROC" = x"no"; then
    enable_xsltproc=no
  fi
fi
AM_CONDITIONAL([ENABLE_XSLTPROC], [test x"$enable_xsltproc" = x"yes"])

# fi

# now let's test to see if we can find the docbook-xsl stuff
XSLTPROC_FLAGS="--nonet"
AC_SUBST(XML_CATALOG)
AC_SUBST(XSLTPROC_FLAGS)
AC_SUBST(DOCBOOK_ROOT)  
AC_SUBST(CAT_ENTRY_START)
AC_SUBST(CAT_ENTRY_END)  
AC_SUBST(XSLTPROC)

dnl *************************************
dnl Extra warnings
dnl *************************************
set_compile_warnings=no

AC_ARG_ENABLE(compile-warnings,
	      [  --enable-compile-warnings
                          Enable verbose compiler warnings],
	      set_compile_warnings=yes)

if test "$GCC" = "yes" -a "$set_compile_warnings" != "no"; then
	CFLAGS="$CFLAGS \
		-std=gnu99 -g -Wshadow -Wall -W -Waggregate-return -Wcast-align 	\
		-Wcast-qual -Wnested-externs -Wpointer-arith -Wundef -Winline	\
		-Wbad-function-cast -pedantic -O0"

fi

case "$host" in
	*bsd*)
		AC_DEFINE(TTYPrompt, "/dev/cua[<0..n>]", [Define verbose tty device])
	;;
	*)
		AC_DEFINE(TTYPrompt, "/dev/tty[<0..n>]", [Define verbose tty device])
	;;
esac


dnl *************************************
dnl Promote warnings to errors
dnl *************************************
dnl (already done) enable_werror=no

AC_ARG_ENABLE(compile-werror,
	      [  --enable-compile-werror
                          Causes warnings to be treated as errors in GCC],
	      enable_werror=yes)
if test "x$GCC" = "xyes" -a "x$enable_werror" = "xyes"; then
	CFLAGS="$CFLAGS -Werror"
fi

AC_OUTPUT([
	Makefile
	pilot-link.m4
	pilot-link.pc
	pilot-link-pp.pc
	libpisock/Makefile
	libpisync/Makefile
	include/Makefile
	include/pi-version.h
	popt/Makefile
	src/Makefile
	src/prc/Makefile
	src/pix/Makefile
	bindings/Makefile
	bindings/Perl/Makefile.PL
	bindings/Python/setup.py
	bindings/Python/setup-standalone.py
	bindings/Java/Makefile
	bindings/Tcl/Makefile
	doc/Makefile
	doc/doxygen.cfg
	doc/xml/Makefile
	doc/xml/catalog.xml
	doc/xml/docbook.xsl
	doc/man/Makefile
	
tests/Makefile
	darwin/Makefile
])

AC_MSG_RESULT([
  Options detected/selected
  -------------------------.
  pilot-link version...... : $VERSION
  libpisock version....... : $PISOCK_CURRENT.$PISOCK_AGE.$PISOCK_REVISION
  libpisync version....... : $PISYNC_CURRENT.$PISYNC_AGE.$PISYNC_REVISION
  Build for host.......... : $host
  Extra Warnings.......... : $set_compile_warnings
  Direct USB support...... : $msg_usb
  BlueZ support........... : $use_bluez
  Thread-safe libpisock... : $msg_threads
  ElectricFence checks.... : $msg_efence
  CPPFLAGS................ : $CPPFLAGS
  CFLAGS.................. : $CFLAGS

  Userland Tools
  -------------------------.
  Build userland tools.... : $enable_conduits
  Support for popt........ : $msg_popt
  Readline support........ : $msg_readline
  PNG support............. : $msg_png

  Internal Language Support
  -------------------------.
  Iconv support........... : $msg_iconv

  External Language Support
  -------------------------.
  TCL support............. : $use_tcl	$TCL_VERSION
  Java support............ : $use_java	$JAVA_VERSION
  Python support.......... : $use_python	$PYTHON_VERSION
  Perl support............ : $use_perl	$PERL_VERSION
])

if [[ x"$PL_FROM_CVS" = xyes ]]; then
  echo " .----- NOTICE ------------------------------------------------."
  echo " |         You are using pilot-link from CVS source.           |"
  echo " |                                                             |"
  echo " | This is likely to be unstable, or contain some incomplete   |"
  echo " | features, or just plain not work at all. Use it at your own |"
  echo " | risk. Please help us to fix any bugs you find, by reporting |"
  echo " | them back to us via email or at http://bugs.pilot-link.org/ |"
  echo " \`-------------------------------------------------------------'"
  echo ""
fi
