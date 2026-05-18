PYTHON=
PYTHON_VERSION=
PYTHON_CFLAGS=
PYTHON_LIBS=

AC_DEFUN([AM_CHECK_PYTHON],
[
        AC_SUBST(PYTHON_LIBS)
        AC_SUBST(PYTHON_CFLAGS)

        AC_ARG_WITH(python,
                AS_HELP_STRING([--with-python],[Compile with Python bindings]),
                if test "x$withval" != "xno" -a "x$withval" != "xyes"; then
                        ith_arg="$withval/include:-L$withval/lib $withval/include/python:-L$withval/lib"
                fi
        )

        if test "x$with_python" != "xno"; then

                if test "x$PYTHON" = "x"; then
                        AC_PATH_PROGS(PYTHON, [python3 python], [no])
                fi

                if test "$PYTHON" != "no"; then
                        PYTHON_VERSION=$($PYTHON -c "import sys; print('%d.%d' % (sys.version_info.major, sys.version_info.minor))")
                        PYTHON_INCLUDE=$($PYTHON -c "import sysconfig; print(sysconfig.get_path('include'))")
                        PYTHON_LIBS=$($PYTHON -c "import sysconfig; print(' '.join([x for x in [sysconfig.get_config_var('LIBS'), sysconfig.get_config_var('SYSLIBS'), sysconfig.get_config_var('LINKFORSHARED')] if x]))")
                fi

                AC_MSG_CHECKING(for Python.h)

                if test "$PYTHON" != "no" -a "$PYTHON_VERSION" != ""; then
                        if test -f "$PYTHON_INCLUDE/Python.h"; then
                                AC_MSG_RESULT($PYTHON_INCLUDE/Python.h)
                                PYTHON_CFLAGS="-I$PYTHON_INCLUDE"
                                PYTHON_H=yes
                        else
                                AC_MSG_RESULT(not found or unusable)
                                PYTHON_H=no
                        fi
                else
                        AC_MSG_RESULT(not found or unusable)
                        PYTHON_H=no
                fi
        fi
])
