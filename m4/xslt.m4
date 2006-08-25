AC_CHECK_PROG(XSLTPROC,xsltproc,xsltproc)

AC_MSG_CHECKING([if xsltproc is recent enough])
if test "$($XSLTPROC --version | awk '/^libxslt/{print $2}')" -lt 10024 ; then 
  AC_MSG_RESULT([version 1.0.24 or newer required])
  exit 1
fi
AC_MSG_RESULT(yes)

XSLTPROC_FLAGS=""--nonet""
__USE_SYSTEM_CATALOG=yes

AC_ARG_WITH(xslroot, [  --with-xslroot=DIR      Specify DocBook XSL location])

if test "$with_xslroot" != "" ; then
  XSL_ROOT="$with_xslroot"      # -- user has specified the parameter
fi
AC_MSG_CHECKING([if XSL_ROOT (envar) was specified manually...])
if test ! -z "$XSL_ROOT"; then  # -- ugly hack to see if the envar was set
  AC_MSG_RESULT(yes)
  AC_MSG_CHECKING(for $XSL_ROOT/xhtml/docbook.xsl)
  if test -f "$XSL_ROOT/xhtml/docbook.xsl"; then
    AC_MSG_RESULT(yes)
    __USE_SYSTEM_CATALOG=no
  else
    AC_MSG_RESULT(not found)
  fi
else
  AC_MSG_RESULT(no)
fi

if test "$__USE_SYSTEM_CATALOG" = "yes"; then
  AC_MSG_CHECKING([if /etc/xml/catalog exists])
  if test ! -r /etc/xml/catalog; then
    AC_MSG_RESULT(no)
    PDB_PATHS="/usr/share/sgml/docbook/xsl"
    PDB_PATHS="$PDB_PATHS /usr/share/sgml/docbook/stylesheet/xsl/nwalsh"
    PDB_PATHS="$PDB_PATHS /usr/share/sgml/docbook/xsl-stylesheets"
    #
    # -- yeah, one of my ugly build systems uses /docbook
    #
    PDB_PATHS="$PDB_PATHS /docbook"
    #
    # -- add other possible docbook paths here before the for loop
    #
    # PDB_PATHS="$PDB_PATHS /some/path"
    #
    for i in $PDB_PATHS ; do
      AC_MSG_CHECKING([Looking in $i])
      if test -r "${i}/xhtml/docbook.xsl"; then
        AC_MSG_RESULT(yes)
        XSL_ROOT=$i
      else
        AC_MSG_RESULT(no)
      fi
    done
  
    # -- since we haven't been able to locate the docbook.xsl file
    #    in any of the potentially expected places, we'll need to
    #    try to allow xsltproc to connect to the canonical source.
    #    Yes.  This is ugly, but it might work.
    # 
    if test -z "$XSL_ROOT"; then
      XSLTPROC_FLAGS=
    fi
  else
    AC_MSG_RESULT(yes)
    XML_CATALOG=/etc/xml/catalog
  fi
fi

XSLTPROC_WORKS=no
if test -n "$XSLTPROC"; then
  AC_MSG_CHECKING([whether xsltproc works])
  if test -z "$XML_CATALOG" -a -z "$XSLROOT"; then
    # -- only if we are using the 'net do we need to set this
    #
    DB_FILE="http://docbook.sourceforge.net/release/xsl/current/xhtml/docbook.xsl"
  else
    DB_FILE="$XSL_ROOT/xhtml/docbook.xsl"
  fi

$XSLTPROC $XSLTPROC_FLAGS $DB_FILE >/dev/null 2>&1 << END
<?xml version="1.0" encoding='ISO-8859-1'?>
<!DOCTYPE book PUBLIC "-//OASIS//DTD DocBook XML V4.2//EN"
"http://www.oasis-open.org/docbook/xml/4.2/docbookx.dtd">
<book id="test">
</book>
END
  if test "$?" = 0; then
    XSLTPROC_WORKS=yes
  else
    AC_MSG_ERROR(xsltproc does not appear to work)
  fi
  AC_MSG_RESULT($XSLTPROC_WORKS)
fi

AC_SUBST(XML_CATALOG)
AC_SUBST(XSLTPROC)
AC_SUBST(XSLTPROC_FLAGS)
AC_SUBST(XSL_ROOT)

AC_OUTPUT
