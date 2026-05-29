# This is free software, licensed under the GNU Public License V2.
# See the file COPYING for details.

dnl ******************************
dnl Pilot Link Version
dnl ******************************
m4_define([pilot_link_vers], [0])
m4_define([pilot_link_major], [15])
m4_define([pilot_link_minor], [0])
m4_define([pilot_link_patch], [])

AC_INIT([pilot-link],[pilot_link_vers.pilot_link_major.pilot_link_minor[]pilot_link_patch],[],[pilot-link],[https://github.com/desrod/pilot-link])
AC_CONFIG_SRCDIR([include/pi-version.h.in])
AC_CONFIG_MACRO_DIR([m4])

AC_SUBST([PILOT_LINK_VERS], pilot_link_vers)
AC_SUBST([PILOT_LINK_MAJOR], pilot_link_major)
AC_SUBST([PILOT_LINK_MINOR], pilot_link_minor)
AC_SUBST([PILOT_LINK_PATCH], pilot_link_patch)
VERSION="$PILOT_LINK_VERS.$PILOT_LINK_MAJOR.$PILOT_LINK_MINOR$PILOT_LINK_PATCH"

dnl Initialize maintainer mode
AM_MAINTAINER_MODE

dnl Run all of the sane intltool tests
dnl IT_PROG_INTLTOOL

AC_PREFIX_DEFAULT(/usr/local)

dnl ******************************
dnl Limit to a specific ac version
dnl ******************************
AC_PREREQ([2.71])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_HEADERS([include/pi-md5.h])

dnl ******************************
dnl Pilot Link Library Version
dnl libpisock.so.CURRENT.AGE.REVISION
dnl Please see the following URL for details:
dnl http://sources.redhat.com/autobook/autobook/autobook_91.html
dnl ******************************
PISOCK_CURRENT=10
PISOCK_REVISION=0
PISOCK_AGE=1

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
save_CXXFLAGS="$CXXFLAGS"
CFLAGS=$(printf '%s\n' "$save_CFLAGS" | sed -e 's/\(^\|[ 	]\)-Werror\([ 	]\|$\)/\1\2/g')
CXXFLAGS=$(printf '%s\n' "$save_CXXFLAGS" | sed -e 's/\(^\|[ 	]\)-Werror\([ 	]\|$\)/\1\2/g')
if test "x$CFLAGS" != "x$save_CFLAGS"; then
	dnl -Werror was set; treat it as implicit --enable-werror below
	enable_werror="yes"
fi


dnl Check host type
AC_CANONICAL_HOST
AC_CANONICAL_TARGET
AC_DEFINE_UNQUOTED(HOST_OS, "$host", [Host type])

AM_INIT_AUTOMAKE([foreign subdir-objects])


dnl ******************************
dnl Checks for basic programs.
dnl ******************************
AC_PROG_CC
AC_PROG_INSTALL(install ginstall)
LT_INIT([disable-fast-install])
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
dnl Check if this was from git source
dnl ******************************
if test -d "${srcdir}/.git" ; then
       PL_FROM_GIT=yes
fi
AM_CONDITIONAL(GIT_BUILD,[test -d "${srcdir}/.git"])

dnl ******************************
dnl Pilot Link Checks
dnl ******************************
AC_C_CONST
AC_CHECK_HEADERS([sys/time.h])
AC_STRUCT_TM
AC_DEFINE([RETSIGTYPE], [void], [Define as the return type of signal handlers ('int' or 'void').])

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
	[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <termios.h>]], [[int main(void) {
		 struct termios t;t.c_ispeed=t.c_ospeed=0;}]])],[ac_cv_termios_cspeed=yes],[ac_cv_termios_cspeed=no])])

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
		PNG_CFLAGS="-I$with_libpng/include"
		PNG_LIBS="-L$with_libpng/lib -lpng"
		have_png=yes
		;;
		*)
		PKG_CHECK_MODULES([PNG], [libpng],
			[have_png=yes],
			[AC_CHECK_LIB(png, main,
				[have_png=yes; PNG_LIBS="-lpng"],
				[have_png=no], [-lm])])
		;;
	esac

	if test "$have_png" = yes; then
		save_CPPFLAGS="$CPPFLAGS"
		CPPFLAGS="$CPPFLAGS $PNG_CFLAGS"
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
				msg_png=yes
			fi
		fi
		CPPFLAGS="$save_CPPFLAGS"
	fi
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
	      AS_HELP_STRING([--enable-libusb],[Enable usage of libusb]),
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
		AS_HELP_STRING([--with-bluez],[Enable use of BlueZ]),
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
			&& AC_MSG_ERROR([[BlueZ explicitly requested, but no library found]])
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
		dnl Determine if system popt is good enough (single test to avoid nested macro expansion issues)
		save_LIBS="$LIBS"
		AC_CHECK_HEADER([popt.h], [
			AC_CHECK_DECL([POPT_BIT_SET], [
				AC_CHECK_LIB([popt], [poptStrippedArgv], [], [
					with_included_popt="yes"
				])
			], [
				with_included_popt="yes"
			], [[#include <popt.h>]])
		], [
			with_included_popt="yes"
		])
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
AM_CONDITIONAL(HAVE_POPT, test "x$msg_popt" != "xnone")
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
	iconv, iconv_open, [have_iconv=yes; ICONV_LIBS="$ICONV_LIBS -liconv"], [
		AC_CHECK_FUNC(iconv, [
			AC_CHECK_FUNCS(gnu_get_libc_version)
			AC_MSG_CHECKING(if system iconv handles CP1252)
			AC_RUN_IFELSE([AC_LANG_SOURCE([[
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
}]])],[AC_MSG_RESULT(yes); have_iconv=yes],[AC_MSG_RESULT(no);],[AC_MSG_RESULT(not cross compiling);])])])

dnl ##  Fixed AC_RUN_IFELSE([AC_LANG_SOURCE([[]])],[],[],[]) warning, missing third argument. -DD 22/01/2002
dnl
dnl 	AC_RUN_IFELSE([AC_LANG_SOURCE([[]])],[],[],[]) is a macro that's basically like an if-then-else structure. If
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
		PERL_VERSION=$($PERL -e 'print $]')
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
	JAVABASE=$(echo "$JAVAC" | sed -e "s/\/bin\/javac//")
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
	JAVA_VERSION=$(java -version 2>&1 | grep '^java version' | awk -F\" '{print $2}')
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
        AC_PATH_PROGS(PYTHON, [python3])
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
	CFLAGS=$(echo "$CFLAGS" | sed -e "s/-g //")
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
AM_CONDITIONAL(BUILD_XML,[test ! -d "{$srcdir}/.git"])
# if test -d "${srcdir}/.git" ; then

AC_ARG_ENABLE([xsltproc], [AS_HELP_STRING([--enable-xsltproc],
	[Use xsltproc to build documentation [default=no]])],
	[enable_xsltproc="$enableval"],
[enable_xsltproc=no])
if test x"$enable_xsltproc" = x"yes"; then
  AC_PATH_PROG([XSLTPROC], [xsltproc], [no])
  if test x"$XSLTPROC" = x"no"; then
    enable_xsltproc=no
  fi
fi

# Locate the docbook-xsl stylesheet root. Common install locations:
#   /usr/share/xml/docbook/stylesheet/docbook-xsl                (Debian)
#   /usr/share/sgml/docbook/xsl-stylesheets                      (Red Hat)
#   /usr/share/xsl/docbook                                       (Arch)
#   /opt/homebrew/opt/docbook-xsl/docbook-xsl                    (Homebrew, ARM)
#   /usr/local/opt/docbook-xsl/docbook-xsl                       (Homebrew, Intel)
#   /usr/local/share/xsl/docbook-xsl                             (FreeBSD)
# Allow override via --with-docbook-xsl=PATH.
AC_ARG_WITH([docbook-xsl],
  [AS_HELP_STRING([--with-docbook-xsl=PATH],
    [Path to docbook-xsl stylesheets (root containing html/docbook.xsl)])],
  [DOCBOOK_ROOT="$withval"],
  [DOCBOOK_ROOT=""])

if test x"$enable_xsltproc" = x"yes" && test -z "$DOCBOOK_ROOT"; then
  AC_MSG_CHECKING([for docbook-xsl stylesheet root])
  for d in \
    /usr/share/xml/docbook/stylesheet/docbook-xsl \
    /usr/share/xml/docbook/stylesheet/nwalsh/current \
    /usr/share/sgml/docbook/xsl-stylesheets \
    /usr/share/xsl/docbook \
    /usr/share/xsl/docbook-xsl \
    /opt/homebrew/opt/docbook-xsl/docbook-xsl \
    /usr/local/opt/docbook-xsl/docbook-xsl \
    /opt/local/share/xsl/docbook-xsl \
    /usr/local/share/xsl/docbook-xsl ; do
    if test -f "$d/html/docbook.xsl" && test -f "$d/manpages/synop.xsl"; then
      DOCBOOK_ROOT="$d"
      break
    fi
  done
  if test -n "$DOCBOOK_ROOT"; then
    AC_MSG_RESULT([$DOCBOOK_ROOT])
  else
    AC_MSG_RESULT([not found])
    AC_MSG_WARN([docbook-xsl not found; disabling xsltproc. Pass --with-docbook-xsl=PATH or install docbook-xsl.])
    enable_xsltproc=no
  fi
fi

AM_CONDITIONAL([ENABLE_XSLTPROC], [test x"$enable_xsltproc" = x"yes"])

# fi

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
	dnl CFLAGS=$(printf %s "$CFLAGS" | awk -v RS= -v ORS= '!arr[$0]++')
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

AC_CONFIG_FILES([
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
AC_OUTPUT

dnl ******************************
dnl Configuration summary
dnl ******************************

dnl Normalize boolean-ish values (true/false -> yes/no) so the summary
dnl uses consistent vocabulary.
case "x$use_bluez"  in xtrue) use_bluez=yes  ;; xfalse|x) use_bluez=no  ;; esac
case "x$use_tcl"    in xtrue) use_tcl=yes    ;; xfalse|x) use_tcl=no    ;; esac
case "x$use_java"   in xtrue) use_java=yes   ;; xfalse|x) use_java=no   ;; esac
case "x$use_python" in xtrue) use_python=yes ;; xfalse|x) use_python=no ;; esac
case "x$use_perl"   in xtrue) use_perl=yes   ;; xfalse|x) use_perl=no   ;; esac
case "x$msg_efence" in xtrue) msg_efence=yes ;; xfalse|x) msg_efence=no ;; esac

dnl Enable color only on a real TTY with a capable TERM and tput available.
C_HDR= C_YES= C_NO= C_BOX= C_OFF=
if test -t 1 && test "x$TERM" != xdumb && test "x$TERM" != x \
   && ( command -v tput >/dev/null 2>&1 ); then
  C_HDR=`tput bold 2>/dev/null; tput setaf 6 2>/dev/null`
  C_YES=`tput bold 2>/dev/null; tput setaf 2 2>/dev/null`
  C_NO=`tput  bold 2>/dev/null; tput setaf 1 2>/dev/null`
  C_BOX=`tput setaf 8 2>/dev/null || tput setaf 7 2>/dev/null`
  C_OFF=`tput sgr0 2>/dev/null`
fi

dnl Colorize a yes/no-ish value. Handles "yes, Darwin" style suffixes
dnl by colorizing only the leading yes/no token.
plfmt() {
  case "$1" in
    yes|yes,*|yes\ *)
      rest=`expr "$1" : 'yes\(.*\)'`
      printf '%syes%s%s' "$C_YES" "$C_OFF" "$rest" ;;
    no|none|"")
      printf '%sno%s' "$C_NO" "$C_OFF" ;;
    *)
      printf '%s' "$1" ;;
  esac
}

dnl Render an optional "(version)" suffix only when the version is set.
plver() {
  test -n "$1" && printf ' (%s)' "$1"
}

echo ""
printf '%s+--------------------------------------------------------------+%s\n' "$C_BOX" "$C_OFF"
printf '%s|%s %spilot-link configuration summary%s                             %s|%s\n' \
       "$C_BOX" "$C_OFF" "$C_HDR" "$C_OFF" "$C_BOX" "$C_OFF"
printf '%s+--------------------------------------------------------------+%s\n' "$C_BOX" "$C_OFF"
echo ""
printf '  %sBuild%s\n' "$C_HDR" "$C_OFF"
printf '    pilot-link version ..... %s\n' "$VERSION"
printf '    libpisock version ...... %s.%s.%s\n' "$PISOCK_CURRENT" "$PISOCK_AGE" "$PISOCK_REVISION"
printf '    libpisync version ...... %s.%s.%s\n' "$PISYNC_CURRENT" "$PISYNC_AGE" "$PISYNC_REVISION"
printf '    Host ................... %s\n' "$host"
printf '    CPPFLAGS ............... %s\n' "${CPPFLAGS:-(none)}"
printf '    CFLAGS ................. %s\n' "$CFLAGS"
echo ""
printf '  %sCore features%s\n' "$C_HDR" "$C_OFF"
printf '    Direct USB ............. %s\n' "`plfmt \"$msg_usb\"`"
printf '    BlueZ .................. %s\n' "`plfmt \"$use_bluez\"`"
printf '    Thread-safe libpisock .. %s\n' "`plfmt \"$msg_threads\"`"
printf '    Extra warnings ......... %s\n' "`plfmt \"$set_compile_warnings\"`"
printf '    ElectricFence .......... %s\n' "`plfmt \"$msg_efence\"`"
printf '    Iconv .................. %s\n' "`plfmt \"$msg_iconv\"`"
echo ""
printf '  %sUserland tools%s\n' "$C_HDR" "$C_OFF"
printf '    Build tools ............ %s\n' "`plfmt \"$enable_conduits\"`"
printf '    popt ................... %s\n' "$msg_popt"
printf '    Readline ............... %s\n' "`plfmt \"$msg_readline\"`"
printf '    PNG .................... %s\n' "`plfmt \"$msg_png\"`"
echo ""
printf '  %sLanguage bindings%s\n' "$C_HDR" "$C_OFF"
printf '    TCL .................... %s%s\n' "`plfmt \"$use_tcl\"`"    "`plver \"$TCL_VERSION\"`"
printf '    Java ................... %s%s\n' "`plfmt \"$use_java\"`"   "`plver \"$JAVA_VERSION\"`"
printf '    Python ................. %s%s\n' "`plfmt \"$use_python\"`" "`plver \"$PYTHON_VERSION\"`"
printf '    Perl ................... %s%s\n' "`plfmt \"$use_perl\"`"   "`plver \"$PERL_VERSION\"`"
echo ""

if test "x$PL_FROM_GIT" = xyes; then
  printf '%s+----- NOTICE -------------------------------------------------+%s\n' "$C_BOX" "$C_OFF"
  printf '%s|%s         You are using pilot-link from source.               %s|%s\n' "$C_BOX" "$C_OFF" "$C_BOX" "$C_OFF"
  printf '%s|%s                                                             %s|%s\n' "$C_BOX" "$C_OFF" "$C_BOX" "$C_OFF"
  printf '%s|%s This is likely to be unstable, or contain some incomplete   %s|%s\n' "$C_BOX" "$C_OFF" "$C_BOX" "$C_OFF"
  printf '%s|%s features, or just plain not work at all. Use it at your own %s|%s\n' "$C_BOX" "$C_OFF" "$C_BOX" "$C_OFF"
  printf '%s|%s risk. Please help us to fix any bugs you find, by reporting %s|%s\n' "$C_BOX" "$C_OFF" "$C_BOX" "$C_OFF"
  printf '%s|%s them at: https://github.com/desrod/pilot-link/issues        %s|%s\n' "$C_BOX" "$C_OFF" "$C_BOX" "$C_OFF"
  printf '%s+--------------------------------------------------------------+%s\n' "$C_BOX" "$C_OFF"
  echo ""
fi
