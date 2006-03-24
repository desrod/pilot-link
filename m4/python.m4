PYTHON=
PYTHON_VERSION=
PYTHON_CFLAGS=
PYTHON_LIBS=

AC_DEFUN([AM_CHECK_PYTHON],
[
        AC_SUBST(PYTHON_LIBS)
        AC_SUBST(PYTHON_CFLAGS)

        AC_ARG_WITH(python,
                AC_HELP_STRING([--with-python],[Compile with Python bindings]),
                if test "x$withval" != "xno" -a "x$withval" != "xyes"; then
                        ith_arg="$withval/include:-L$withval/lib $withval/include/python:-L$withval/lib"
                fi
        )

        if test "x$with_python" != "xno"; then

                AC_PATH_PROG(PYTHON, python)

                if test "$PYTHON" != ""; then
                        PYTHON_VERSION=`$PYTHON -c "import sys; print sys.version[[0:3]]"`
                        PYTHON_PREFIX=`$PYTHON -c "import sys; print sys.prefix"`
                fi

                AC_MSG_CHECKING(for Python.h)

                PYTHON_EXEC_PREFIX=`$PYTHON -c "import sys; print sys.exec_prefix"`

                if test "$PYTHON_VERSION" != ""; then 
                        if test -f $PYTHON_PREFIX/include/python$PYTHON_VERSION/Python.h; then 
                        AC_MSG_RESULT($PYTHON_PREFIX/include/python$PYTHON_VERSION/Python.h)
                                PYTHON_LIB_LOC="-L$PYTHON_EXEC_PREFIX/lib/python$PYTHON_VERSION/config"
                                PYTHON_CFLAGS="-I$PYTHON_PREFIX/include/python$PYTHON_VERSION"
                                PYTHON_MAKEFILE="$PYTHON_EXEC_PREFIX/lib/python$PYTHON_VERSION/config/Makefile"

                                PYTHON_LOCALMODLIBS=`sed -n -e 's/^LOCALMODLIBS=\(.*\)/\1/p' $PYTHON_MAKEFILE`
                                PYTHON_BASEMODLIBS=`sed -n -e 's/^BASEMODLIBS=\(.*\)/\1/p' $PYTHON_MAKEFILE`
                                PYTHON_OTHER_LIBS=`sed -n -e 's/^LIBS=\(.*\)/\1/p' $PYTHON_MAKEFILE`
                                PYTHON_OTHER_LIBM=`sed -n -e 's/^LIBC=\(.*\)/\1/p' $PYTHON_MAKEFILE`
                                PYTHON_OTHER_LIBC=`sed -n -e 's/^LIBM=\(.*\)/\1/p' $PYTHON_MAKEFILE`
                                PYTHON_LIBS="$PYTHON_LOCALMODLIBS $PYTHON_BASEMODLIBS $PYTHON_OTHER_LIBS $PYTHON_OTHER_LIBC $PYTHON_OTHER_LIBM"

                                PYTHON_LIBS="-L$PYTHON_EXEC_PREFIX/lib $PYTHON_LIB_LOC -lpython$PYTHON_VERSION $PYTHON_LIBS"
                                PYTHON_CFLAGS="$PYTHON_CFLAGS"
                                PYTHON_H=yes
			else
				AC_MSG_RESULT(not found or unusable)
				PYTHON_H=no
                        fi
                fi
        fi
])
