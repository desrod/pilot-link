AC_DEFUN(AC_PILOT_LINK,
[
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h malloc.h sys/ioctl.h sys/time.h sys/ioctl_compat.h memory.h string.h strings.h unistd.h stdlib.h netinet/in.h dirent.h sys/ndir.h sys/dir.h ndir.h sys/select.h sockio.h netdb.h sys/utsname.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_HEADER_TIME
AC_STRUCT_TM

dnl Find optional libraries (borrowed from Tcl)
tcl_checkBoth=0
AC_CHECK_FUNC(connect, tcl_checkSocket=0, tcl_checkSocket=1)
if test "$tcl_checkSocket" = 1; then
    AC_CHECK_LIB(socket, main, LIBS="$LIBS -lsocket", tcl_checkBoth=1)
fi
if test "$tcl_checkBoth" = 1; then
    tk_oldLibs=$LIBS
    LIBS="$LIBS -lsocket -lnsl"
    AC_CHECK_FUNC(accept, tcl_checkNsl=0, [LIBS=$tk_oldLibs])
fi
AC_CHECK_FUNC(gethostbyname, , AC_CHECK_LIB(nsl, main, [LIBS="$LIBS -lnsl"]))
AC_CHECK_LIB(inet, main, [LIBS="$LIBS -linet"])

dnl Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_TYPE_SIGNAL
AC_CHECK_FUNCS(atexit strchr strdup memcpy memmove strtoul cfmakeraw cfsetspeed cfsetispeed cfsetospeed sigaction dup2 inet_aton gethostname uname putenv)

AC_CACHE_CHECK([for cispeed and cospeed members of struct termios],
  ac_cv_termios_cspeed,
[AC_TRY_COMPILE([#include <termios.h>], [int main(void) {
 struct termios t;t.c_ispeed=t.c_ospeed=0;}],
  ac_cv_termios_cspeed=yes,ac_cv_termios_cspeed=no)])
if test $ac_cv_termios_cspeed = yes; then
  AC_DEFINE(TERMIOS_CSPEED)
fi

AC_CACHE_CHECK([for sa_len],
  ac_cv_sa_len,
[AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/socket.h>], [int main(void) {
 struct sockaddr t;t.sa_len = 0;}],
  ac_cv_sa_len=yes,ac_cv_sa_len=no)])
if test $ac_cv_sa_len = yes; then
  AC_DEFINE(HAVE_SA_LEN)
  SA_LEN_FIELD="unsigned char pi_len;"
fi
AC_SUBST(SA_LEN_FIELD)
])