#!/bin/sh
# Run this to generate all the initial makefiles, etc.

PKG_NAME=pilot-link
srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

DIE=0

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`autoconf' installed to compile $PKG_NAME."
  echo "Download the appropriate package for your distribution or get the source."
  echo "Get ftp://ftp.gnu.org/pub/gnu/autoconf/autoconf-2.50.tar.gz"
  echo "(or a newer version if it is available)"
  DIE=1
}

(grep "^AM_PROG_LIBTOOL" $srcdir/configure.ac >/dev/null) && {
  (glibtool --version) < /dev/null > /dev/null 2>&1 || 
  (libtool --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "**Error**: You must have \`libtool' installed to compile $PKG_NAME."
    echo "Get ftp://ftp.gnu.org/pub/gnu/libtool/libtool-1.5.tar.gz"
    echo "(or a newer version if it is available)"
    DIE=1
  }
}

#grep "^AM_GNU_GETTEXT" $srcdir/configure.ac >/dev/null && {
#  grep "sed.*POTFILES" $srcdir/configure.ac >/dev/null || \
#  (gettext --version) < /dev/null > /dev/null 2>&1 || {
#    echo
#    echo "**Error**: You must have \`gettext' installed to compile $PKG_NAME."
#    echo "Get ftp://ftp.gnu.org/pub/gnu/gettext/gettext-0.11.5.tar.gz"
#    echo "(or a newer version if it is available)"
#    DIE=1
#  }
#}

(automake --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`automake' installed to compile $PKG_NAME."
  echo "Get ftp://ftp.gnu.org/pub/gnu/automake/automake-1.7.tar.gz"
  echo "(or a newer version if it is available)"
  DIE=1
  NO_AUTOMAKE=yes
}


# if no automake, don't bother testing for aclocal
test -n "$NO_AUTOMAKE" || (aclocal --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: Missing \`aclocal'.  The version of \`automake'"
  echo "installed doesn't appear recent enough."
  echo "Get ftp://ftp.gnu.org/pub/gnu/automake/automake-1.7.tar.gz"
  echo "(or a newer version if it is available)"
  DIE=1
}

if test "$DIE" -eq 1; then
  exit 1
fi

if test -z "$*"; then
  echo "**Warning**: I am going to run \`configure' with no arguments."
  echo "If you wish to pass any to it, please specify them on the"
  echo \`$0\'" command line."
  echo
fi

case $CC in
xlc )
  am_opt=--include-deps;;
esac

for coin in `find $srcdir -name configure.ac -print`
do 
  dr=`dirname $coin`
  if test -f $dr/NO-AUTO-GEN; then
    echo skipping $dr -- flagged as no auto-gen
  else
    echo processing $dr
    ( cd $dr
      aclocalinclude="$ACLOCAL_FLAGS -I m4"

      if grep "^AM_GNU_GETTEXT" configure.ac >/dev/null; then
	if grep "sed.*POTFILES" configure.ac >/dev/null; then
	  : do nothing -- we still have an old unmodified configure.ac
	else
	  echo "Creating $dr/aclocal.m4 ..."
	  test -r $dr/aclocal.m4 || touch $dr/aclocal.m4
	  echo "Running gettextize...  Ignore non-fatal messages."
	  echo "no" | gettextize --force --copy
	  echo "Making $dr/aclocal.m4 writable ..."
	  test -r $dr/aclocal.m4 && chmod u+w $dr/aclocal.m4
        fi
      fi
      if grep "^AM_PROG_LIBTOOL" configure.ac >/dev/null; then
	if test -z "$NO_LIBTOOLIZE" ; then 
	    case "$OSTYPE" in
		*darwin*)
		    echo "Running glibtoolize... ($MACHTYPE)"
		    glibtoolize --force --copy
		;;
		    *)
		    echo "Running libtoolize..."
		    libtoolize --force --copy
		;;
	    esac
	fi
      fi

      echo "Running aclocal $aclocalinclude ..."
      aclocal $aclocalinclude || {
	echo
	echo "**Error**: aclocal failed. This may mean that you have not"
	echo "installed all of the packages you need, or you may need to"
	echo "set ACLOCAL_FLAGS to include \"-I \$prefix/share/aclocal\""
	echo "for the prefix where you installed the packages whose"
	echo "macros were not found"
	exit 1
      }

      if grep "^AM_CONFIG_HEADER" configure.ac >/dev/null; then
	echo "Running autoheader..."
	autoheader || { echo "**Error**: autoheader failed."; exit 1; }
      fi
      echo "Running automake --gnu $am_opt ..."
      automake --add-missing --gnu $am_opt ||
	{ echo "**Error**: automake failed."; exit 1; }
      echo "Running autoconf ..."
      autoconf || { echo "**Error**: autoconf failed."; exit 1; }
    ) || exit 1
  fi
done

conf_flags="--enable-maintainer-mode " #--enable-compile-warnings --enable-iso-c

if test x$NOCONFIGURE = x; then
  echo Running $srcdir/configure $conf_flags "$@" ...
  $srcdir/configure $conf_flags "$@" \
  && echo Now type \`make\' to compile $PKG_NAME || exit 1
else
  echo Skipping configure process.
fi
