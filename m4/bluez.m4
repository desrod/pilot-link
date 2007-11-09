dnl AC_BLUEZ([action-if-found],[action-if-not-found])
dnl Input: use_bluez may contain prefix
AC_DEFUN([AC_BLUEZ], [
 bluez_prefix="$prefix"
 test "$prefix" = "NONE" && bluez_prefix="$ac_default_prefix"
 test "${use_bluez#/}" != "$use_bluez" && bluez_prefix="$use_bluez"
 BLUEZ_CFLAGS=""
 BLUEZ_LIBS=""
 ac_save_CPPFLAGS="$CPPFLAGS"
 for bluid in {"${bluez_prefix}",/usr,/usr/local}/include ; do
  CPPFLAGS="$ac_save_CPPFLAGS -I${bluid}"
  AC_CHECK_HEADER([bluetooth/bluetooth.h],[
   BLUEZ_CFLAGS="-I$bluid"
  ])
  test -z "$BLUEZ_CFLAGS" || break
 done
 CPPFLAGS="$ac_save_CPPFLAGS"
 if test -n "$BLUEZ_CFLAGS" ; then
  ac_save_LDFLAGS="$LDFLAGS"
  for bluld in {"${bluez_prefix}",usr,/usr/local}/{lib,lib64} ; do
   LDFLAGS="$ac_saveLDFLAGS -L${bluld}"
   AC_CHECK_LIB([bluetooth],[hci_open_dev],[
    AC_CHECK_LIB([bluetooth],[sdp_connect],[
     BLUEZ_LIBS="-L${bluld} -lbluetooth"
    ])
   ])
   test -z "${BLUEZ_LIBS}" || break
  done
  LDFLAGS="$ac_save_LDFLAGS"
 fi
 if test -z "${BLUEZ_CFLAGS}" -o -z "${BLUEZ_LIBS}" ; then
  ifelse([$2], , :, [$2])
 else
  ifelse([$1], , :, [$1])
 fi
])
