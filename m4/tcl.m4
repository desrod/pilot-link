# tcl.m4 --
#
#	This file provides a set of autoconf macros to help 
#	pilot-link find Tcl support (borrowed from PILOT_LINK)
#
# Copyright (c) 1999-2000 Ajuba Solutions.
# Copyright (c) 2002-2003 ActiveState Corporation.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.

#------------------------------------------------------------------------
# PILOT_LINK_PATH_TCLCONFIG --
#
#	Locate the tclConfig.sh file and perform a sanity check on
#	the Tcl compile flags
#
# Arguments:
#	none
#
# Results:
#
#	Adds the following arguments to configure:
#		--with-tcl=...
#
#	Defines the following vars:
#		TCL_BIN_DIR	Full path to the directory containing
#				the tclConfig.sh file
#------------------------------------------------------------------------

AC_DEFUN([PILOT_LINK_PATH_TCLCONFIG], [
    # Ok, lets find the tcl configuration
    # First, look for one uninstalled.
    # the alternative search directory is invoked by --with-tcl
    #

    use_tcl=false

    AC_ARG_WITH(tcl, [  --with-tcl=tclconfig    use TCL], with_tclconfig=${withval}, with_tclconfig=no)

    if test x"${with_tclconfig}" != xno ; then
	AC_MSG_CHECKING([for Tcl configuration])
	AC_CACHE_VAL(ac_cv_c_tclconfig,[

	    # First check to see if --with-tcl was specified.
	    if test x"${with_tclconfig}" != x ; then
		if test -f "${with_tclconfig}/tclConfig.sh" ; then
		    ac_cv_c_tclconfig=`(cd ${with_tclconfig}; pwd)`
		else
		    AC_MSG_ERROR([${with_tclconfig}, but directory doesn't contain tclConfig.sh])
		fi
	    fi

	    # then check for a private Tcl installation
	    if test x"${ac_cv_c_tclconfig}" = x ; then
		for i in \
			../tcl \
			`ls -dr ../tcl[[8-9]].[[0-9]]* 2>/dev/null` \
			../../tcl \
			`ls -dr ../../tcl[[8-9]].[[0-9]]* 2>/dev/null` \
			../../../tcl \
			`ls -dr ../../../tcl[[8-9]].[[0-9]]* 2>/dev/null` ; do
		    if test -f "$i/unix/tclConfig.sh" ; then
			ac_cv_c_tclconfig=`(cd $i/unix; pwd)`
			break
		    fi
		done
	    fi

	    # check in a few common install locations
	    if test x"${ac_cv_c_tclconfig}" = x ; then
		for i in `ls -d ${exec_prefix}/lib 2>/dev/null` \
			`ls -d ${prefix}/lib 2>/dev/null` \
			`ls -d /usr/local/lib 2>/dev/null` \
			`ls -d /usr/contrib/lib 2>/dev/null` \
			`ls -d /usr/lib 2>/dev/null` \
			; do
		    if test -f "$i/tclConfig.sh" ; then
			ac_cv_c_tclconfig=`(cd $i; pwd)`
			break
		    fi
		done
	    fi

	    # check in a few other private locations
	    if test x"${ac_cv_c_tclconfig}" = x ; then
		for i in \
			${srcdir}/../tcl \
			`ls -dr ${srcdir}/../tcl[[8-9]].[[0-9]]* 2>/dev/null` ; do
		    if test -f "$i/unix/tclConfig.sh" ; then
		    ac_cv_c_tclconfig=`(cd $i/unix; pwd)`
		    break
		fi
		done
	    fi
	])

	if test x"${ac_cv_c_tclconfig}" = x ; then
	    TCL_BIN_DIR="# no Tcl configs found"
	    AC_MSG_WARN("Cannot find Tcl configuration definitions")
	    exit 0
	else
	    use_tcl=true
	    TCL_BIN_DIR=${ac_cv_c_tclconfig}
	    AC_MSG_RESULT([found $TCL_BIN_DIR/tclConfig.sh])
	fi
    fi
])


#------------------------------------------------------------------------
# PILOT_LINK_LOAD_TCLCONFIG --
#
#	Load the tclConfig.sh file
#
# Arguments:
#	
#	Requires the following vars to be set:
#		TCL_BIN_DIR
#
# Results:
#
#	Subst the following vars:
#		TCL_BIN_DIR
#		TCL_SRC_DIR
#		TCL_LIB_FILE
#
#------------------------------------------------------------------------

AC_DEFUN([PILOT_LINK_LOAD_TCLCONFIG], [
    AC_MSG_CHECKING([for existence of $TCL_BIN_DIR/tclConfig.sh])

    if test -f "$TCL_BIN_DIR/tclConfig.sh" ; then
        AC_MSG_RESULT([loading])
	. $TCL_BIN_DIR/tclConfig.sh
    else
        AC_MSG_RESULT([file not found])
    fi

    #
    # If the TCL_BIN_DIR is the build directory (not the install directory),
    # then set the common variable name to the value of the build variables.
    # For example, the variable TCL_LIB_SPEC will be set to the value
    # of TCL_BUILD_LIB_SPEC. An extension should make use of TCL_LIB_SPEC
    # instead of TCL_BUILD_LIB_SPEC since it will work with both an
    # installed and uninstalled version of Tcl.
    #

    if test -f $TCL_BIN_DIR/Makefile ; then
        TCL_LIB_SPEC=${TCL_BUILD_LIB_SPEC}
        TCL_STUB_LIB_SPEC=${TCL_BUILD_STUB_LIB_SPEC}
        TCL_STUB_LIB_PATH=${TCL_BUILD_STUB_LIB_PATH}
    fi

    #
    # eval is required to do the TCL_DBGX substitution
    #

    eval "TCL_LIB_FILE=\"${TCL_LIB_FILE}\""
    eval "TCL_LIB_FLAG=\"${TCL_LIB_FLAG}\""
    eval "TCL_LIB_SPEC=\"${TCL_LIB_SPEC}\""

    eval "TCL_STUB_LIB_FILE=\"${TCL_STUB_LIB_FILE}\""
    eval "TCL_STUB_LIB_FLAG=\"${TCL_STUB_LIB_FLAG}\""
    eval "TCL_STUB_LIB_SPEC=\"${TCL_STUB_LIB_SPEC}\""

    AC_SUBST(TCL_VERSION)
    AC_SUBST(TCL_BIN_DIR)
    AC_SUBST(TCL_SRC_DIR)

    AC_SUBST(TCL_LIB_FILE)
    AC_SUBST(TCL_LIB_FLAG)
    AC_SUBST(TCL_LIB_SPEC)

    AC_SUBST(TCL_STUB_LIB_FILE)
    AC_SUBST(TCL_STUB_LIB_FLAG)
    AC_SUBST(TCL_STUB_LIB_SPEC)

    #AC_SUBST(TCL_DBGX)
    AC_SUBST(TCL_LIBS)
    AC_SUBST(TCL_DEFS)
    AC_SUBST(TCL_EXTRA_CFLAGS)
    AC_SUBST(TCL_LD_FLAGS)
    AC_SUBST(TCL_SHLIB_LD_LIBS)
    #AC_SUBST(TCL_BUILD_LIB_SPEC)
    #AC_SUBST(TCL_BUILD_STUB_LIB_SPEC)
])


#--------------------------------------------------------------------
# PILOT_LINK_TCL_LINK_LIBS
#
#	Search for the libraries needed to link the Tcl shell.
#	Things like the math library (-lm) and socket stuff (-lsocket vs.
#	-lnsl) are dealt with here.
#
# Arguments:
#	Requires the following vars to be set in the Makefile:
#		DL_LIBS
#		LIBS
#		MATH_LIBS
#	
# Results:
#
#	Subst's the following var:
#		TCL_LIBS
#		MATH_LIBS
#
#	Might append to the following vars:
#		LIBS
#
#	Might define the following vars:
#		HAVE_NET_ERRNO_H
#
#--------------------------------------------------------------------

AC_DEFUN([PILOT_LINK_TCL_LINK_LIBS], [
    #--------------------------------------------------------------------
    # On a few very rare systems, all of the libm.a stuff is
    # already in libc.a.  Set compiler flags accordingly.
    # Also, Linux requires the "ieee" library for math to work
    # right (and it must appear before "-lm").
    #--------------------------------------------------------------------

    AC_CHECK_FUNC(sin, MATH_LIBS="", MATH_LIBS="-lm")
    AC_CHECK_LIB(ieee, main, [MATH_LIBS="-lieee $MATH_LIBS"])

    #--------------------------------------------------------------------
    # Interactive UNIX requires -linet instead of -lsocket, plus it
    # needs net/errno.h to define the socket-related error codes.
    #--------------------------------------------------------------------

    AC_CHECK_LIB(inet, main, [LIBS="$LIBS -linet"])
    AC_CHECK_HEADER(net/errno.h, AC_DEFINE(HAVE_NET_ERRNO_H))

    #--------------------------------------------------------------------
    #	Check for the existence of the -lsocket and -lnsl libraries.
    #	The order here is important, so that they end up in the right
    #	order in the command line generated by make.  Here are some
    #	special considerations:
    #	1. Use "connect" and "accept" to check for -lsocket, and
    #	   "gethostbyname" to check for -lnsl.
    #	2. Use each function name only once:  can't redo a check because
    #	   autoconf caches the results of the last check and won't redo it.
    #	3. Use -lnsl and -lsocket only if they supply procedures that
    #	   aren't already present in the normal libraries.  This is because
    #	   IRIX 5.2 has libraries, but they aren't needed and they're
    #	   bogus:  they goof up name resolution if used.
    #	4. On some SVR4 systems, can't use -lsocket without -lnsl too.
    #	   To get around this problem, check for both libraries together
    #	   if -lsocket doesn't work by itself.
    #--------------------------------------------------------------------

    tcl_checkBoth=0
    AC_CHECK_FUNC(connect, tcl_checkSocket=0, tcl_checkSocket=1)
    if test "$tcl_checkSocket" = 1; then
	AC_CHECK_FUNC(setsockopt, , [AC_CHECK_LIB(socket, setsockopt,
	    LIBS="$LIBS -lsocket", tcl_checkBoth=1)])
    fi
    if test "$tcl_checkBoth" = 1; then
	tk_oldLibs=$LIBS
	LIBS="$LIBS -lsocket -lnsl"
	AC_CHECK_FUNC(accept, tcl_checkNsl=0, [LIBS=$tk_oldLibs])
    fi
    AC_CHECK_FUNC(gethostbyname, , [AC_CHECK_LIB(nsl, gethostbyname,
	    [LIBS="$LIBS -lnsl"])])
    
    # Don't perform the eval of the libraries here because DL_LIBS
    # won't be set until we call PILOT_LINK_CONFIG_CFLAGS

    TCL_LIBS='${DL_LIBS} ${LIBS} ${MATH_LIBS}'
    AC_SUBST(TCL_LIBS)
    AC_SUBST(MATH_LIBS)
])

#--------------------------------------------------------------------
# PILOT_LINK_TCL_EARLY_FLAGS
#
#	Check for what flags are needed to be passed so the correct OS
#	features are available.
#
# Arguments:
#	None
#	
# Results:
#
#	Might define the following vars:
#		_ISOC99_SOURCE
#		_LARGEFILE64_SOURCE
#
#--------------------------------------------------------------------

AC_DEFUN([PILOT_LINK_TCL_EARLY_FLAG],[
    AC_CACHE_VAL([tcl_cv_flag_]translit($1,[A-Z],[a-z]),
	AC_TRY_COMPILE([$2], $3, [tcl_cv_flag_]translit($1,[A-Z],[a-z])=no,
	    AC_TRY_COMPILE([[#define ]$1[ 1
]$2], $3,
		[tcl_cv_flag_]translit($1,[A-Z],[a-z])=yes,
		[tcl_cv_flag_]translit($1,[A-Z],[a-z])=no)))
    if test ["x${tcl_cv_flag_]translit($1,[A-Z],[a-z])[}" = "xyes"] ; then
	AC_DEFINE($1)
	tcl_flags="$tcl_flags $1"
    fi
])

AC_DEFUN([PILOT_LINK_TCL_EARLY_FLAGS],[
    AC_MSG_CHECKING([for required early compiler flags])
    tcl_flags=""
    PILOT_LINK_TCL_EARLY_FLAG(_ISOC99_SOURCE,[#include <stdlib.h>],
	[char *p = (char *)strtoll; char *q = (char *)strtoull;])
    PILOT_LINK_TCL_EARLY_FLAG(_LARGEFILE64_SOURCE,[#include <sys/stat.h>],
	[struct stat64 buf; int i = stat64("/", &buf);])
    if test "x${tcl_flags}" = "x" ; then
	AC_MSG_RESULT([none])
    else
	AC_MSG_RESULT([${tcl_flags}])
    fi
])

#--------------------------------------------------------------------
# PILOT_LINK_TCL_64BIT_FLAGS
#
#	Check for what is defined in the way of 64-bit features.
#
# Arguments:
#	None
#	
# Results:
#
#	Might define the following vars:
#		TCL_WIDE_INT_IS_LONG
#		TCL_WIDE_INT_TYPE
#		HAVE_STRUCT_DIRENT64
#		HAVE_STRUCT_STAT64
#		HAVE_TYPE_OFF64_T
#
#--------------------------------------------------------------------

AC_DEFUN([PILOT_LINK_TCL_64BIT_FLAGS], [
    AC_MSG_CHECKING([for 64-bit integer type])
    AC_CACHE_VAL(tcl_cv_type_64bit,[
	AC_TRY_COMPILE(,[__int64 value = (__int64) 0;],
           tcl_cv_type_64bit=__int64,tcl_cv_type_64bit=none
           AC_TRY_RUN([#include <unistd.h>
		int main() {exit(!(sizeof(long long) > sizeof(long)));}
		], tcl_cv_type_64bit="long long"))])
    if test "${tcl_cv_type_64bit}" = none ; then
	AC_MSG_RESULT([using long])
    else
	AC_DEFINE_UNQUOTED(TCL_WIDE_INT_TYPE,${tcl_cv_type_64bit})
	AC_MSG_RESULT([${tcl_cv_type_64bit}])

	# Now check for auxiliary declarations
	AC_MSG_CHECKING([for struct dirent64])
	AC_CACHE_VAL(tcl_cv_struct_dirent64,[
	    AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/dirent.h>],[struct dirent64 p;],
		tcl_cv_struct_dirent64=yes,tcl_cv_struct_dirent64=no)])
	if test "x${tcl_cv_struct_dirent64}" = "xyes" ; then
	    AC_DEFINE(HAVE_STRUCT_DIRENT64)
	fi
	AC_MSG_RESULT([${tcl_cv_struct_dirent64}])

	AC_MSG_CHECKING([for struct stat64])
	AC_CACHE_VAL(tcl_cv_struct_stat64,[
	    AC_TRY_COMPILE([#include <sys/stat.h>],[struct stat64 p;
],
		tcl_cv_struct_stat64=yes,tcl_cv_struct_stat64=no)])
	if test "x${tcl_cv_struct_stat64}" = "xyes" ; then
	    AC_DEFINE(HAVE_STRUCT_STAT64)
	fi
	AC_MSG_RESULT([${tcl_cv_struct_stat64}])

	AC_MSG_CHECKING([for off64_t])
	AC_CACHE_VAL(tcl_cv_type_off64_t,[
	    AC_TRY_COMPILE([#include <sys/types.h>],[off64_t offset;
],
		tcl_cv_type_off64_t=yes,tcl_cv_type_off64_t=no)])
	if test "x${tcl_cv_type_off64_t}" = "xyes" ; then
	    AC_DEFINE(HAVE_TYPE_OFF64_T)
	fi
	AC_MSG_RESULT([${tcl_cv_type_off64_t}])
    fi
])

##
## Here ends the standard Tcl configuration bits and starts the
## PILOT_LINK specific functions
##

#------------------------------------------------------------------------
# PILOT_LINK_PUBLIC_TCL_HEADERS --
#
#	Locate the installed public Tcl header files
#
# Arguments:
#	None.
#
# Results:
#
#	Adds a --with-tclinclude switch to configure.
#	Result is cached.
#
#	Substs the following vars:
#		TCL_INCLUDES
#------------------------------------------------------------------------

AC_DEFUN([PILOT_LINK_PUBLIC_TCL_HEADERS], [
    AC_MSG_CHECKING([for Tcl public headers])

    AC_ARG_WITH(tclinclude, [  --with-tclinclude       public Tcl header dir], with_tclinclude=${withval})

    AC_CACHE_VAL(ac_cv_c_tclh, [
	# Use the value from --with-tclinclude, if it was given

	if test x"${with_tclinclude}" != x ; then
	    if test -f "${with_tclinclude}/tcl.h" ; then
		ac_cv_c_tclh=${with_tclinclude}
	    else
		AC_MSG_ERROR([${with_tclinclude} directory does not contain tcl.h])
	    fi
	else
	    # Check order: pkg --prefix location, Tcl's --prefix location,
	    # directory of tclConfig.sh, and Tcl source directory.
	    # Looking in the source dir is not ideal, but OK.

	    eval "temp_includedir=${includedir}"
	    list="`ls -d ${temp_includedir}      2>/dev/null` \
		`ls -d ${TCL_PREFIX}             2>/dev/null` \
		`ls -d ${TCL_PREFIX}/include     2>/dev/null` \
		`ls -d ${TCL_BIN_DIR}/../include 2>/dev/null`"
	    if test "${PILOT_LINK_PLATFORM}" != "windows" -o "$GCC" = "yes"; then
		list="/usr/local/include /usr/include $list"
	    fi
	    for i in $list ; do
		if test -f "$i/tcl.h" ; then
		    ac_cv_c_tclh=$i
		    break
		fi
	    done
	fi
    ])

    # Print a message based on how we determined the include path

    if test x"${ac_cv_c_tclh}" = x ; then
	AC_MSG_ERROR([tcl.h not found.

           Please specify its location with --with-tclinclude=<path>       
           Where <path> is the directory containing tcl.h for your 
           tcl version, such as --with-tclinclude=/usr/include/tcl/   
])
    else
	AC_MSG_RESULT([${ac_cv_c_tclh}])
    fi

    TCL_INCLUDES=-I${ac_cv_c_tclh}

    AC_SUBST(TCL_INCLUDES)
])


#------------------------------------------------------------------------
# PILOT_LINK_PROG_TCLSH
#	Locate a tclsh shell in the following directories:
#		${TCL_BIN_DIR}		${TCL_BIN_DIR}/../bin
#		${exec_prefix}/bin	${prefix}/bin
#		${PATH}
#
# Arguments
#	none
#
# Results
#	Subst's the following values:
#		TCLSH_PROG
#------------------------------------------------------------------------

AC_DEFUN([PILOT_LINK_PROG_TCLSH], [
    AC_MSG_CHECKING([for tclsh])

    AC_CACHE_VAL(ac_cv_path_tclsh, [
	if test "x${CELIB_DIR}" != "x" ; then
	    # If CELIB_DIR is defined, assume Windows/CE target is requested
	    # which means target tclsh cannot be run (cross-compile)
	    search_path=`echo ${PATH} | sed -e 's/:/ /g'`
	else
	    search_path=`echo ${TCL_BIN_DIR}:${TCL_BIN_DIR}/../bin:${exec_prefix}/bin:${prefix}/bin:${PATH} | sed -e 's/:/ /g'`
	fi
	for dir in $search_path ; do
	    for j in `ls -r $dir/tclsh[[8-9]]*${EXEEXT} 2> /dev/null` \
		    `ls -r $dir/tclsh*${EXEEXT} 2> /dev/null` ; do
		if test x"$ac_cv_path_tclsh" = x ; then
		    if test -f "$j" ; then
			ac_cv_path_tclsh=$j
			break
		    fi
		fi
	    done
	done
    ])

    if test -f "$ac_cv_path_tclsh" ; then
	TCLSH_PROG=$ac_cv_path_tclsh
	AC_MSG_RESULT([$TCLSH_PROG])
    else
	AC_MSG_ERROR([No tclsh found in PATH:  $search_path])
    fi
    AC_SUBST(TCLSH_PROG)
])

#------------------------------------------------------------------------
# PILOT_LINK_PROG_WISH
#	Locate a wish shell in the following directories:
#		${TK_BIN_DIR}		${TK_BIN_DIR}/../bin
#		${TCL_BIN_DIR}		${TCL_BIN_DIR}/../bin
#		${exec_prefix}/bin	${prefix}/bin
#		${PATH}
#
# Arguments
#	none
#
# Results
#	Subst's the following values:
#		WISH_PROG
#------------------------------------------------------------------------

AC_DEFUN([PILOT_LINK_PROG_WISH], [
    AC_MSG_CHECKING([for wish])

    AC_CACHE_VAL(ac_cv_path_wish, [
	if test "x${CELIB_DIR}" != "x" ; then
	    # If CELIB_DIR is defined, assume Windows/CE target is requested
	    # which means target wish cannot be run (cross-compile)
	    search_path=`echo ${PATH} | sed -e 's/:/ /g'`
	else
	    search_path=`echo ${TK_BIN_DIR}:${TK_BIN_DIR}/../bin:${TCL_BIN_DIR}:${TCL_BIN_DIR}/../bin:${exec_prefix}/bin:${prefix}/bin:${PATH} | sed -e 's/:/ /g'`
	fi
	for dir in $search_path ; do
	    for j in `ls -r $dir/wish[[8-9]]*${EXEEXT} 2> /dev/null` \
		    `ls -r $dir/wish*${EXEEXT} 2> /dev/null` ; do
		if test x"$ac_cv_path_wish" = x ; then
		    if test -f "$j" ; then
			ac_cv_path_wish=$j
			break
		    fi
		fi
	    done
	done
    ])

    if test -f "$ac_cv_path_wish" ; then
	WISH_PROG=$ac_cv_path_wish
	AC_MSG_RESULT([$WISH_PROG])
    else
	AC_MSG_ERROR([No wish found in PATH:  $search_path])
    fi
    AC_SUBST(WISH_PROG)
])

#------------------------------------------------------------------------
# PILOT_LINK_PATH_CONFIG --
#
#	Locate the ${1}Config.sh file and perform a sanity check on
#	the ${1} compile flags.  These are used by packages like
#	[incr Tk] that load *Config.sh files from more than Tcl and Tk.
#
# Arguments:
#	none
#
# Results:
#
#	Adds the following arguments to configure:
#		--with-$1=...
#
#	Defines the following vars:
#		$1_BIN_DIR	Full path to the directory containing
#				the $1Config.sh file
#------------------------------------------------------------------------

AC_DEFUN([PILOT_LINK_PATH_CONFIG], [
    #
    # Ok, lets find the $1 configuration
    # First, look for one uninstalled.
    # the alternative search directory is invoked by --with-$1
    #

    if test x"${no_$1}" = x ; then
	# we reset no_$1 in case something fails here
	no_$1=true
	AC_ARG_WITH($1, [  --with-$1              directory containing $1 configuration ($1Config.sh)], with_$1config=${withval})
	AC_MSG_CHECKING([for $1 configuration])
	AC_CACHE_VAL(ac_cv_c_$1config,[

	    # First check to see if --with-$1 was specified.
	    if test x"${with_$1config}" != x ; then
		if test -f "${with_$1config}/$1Config.sh" ; then
		    ac_cv_c_$1config=`(cd ${with_$1config}; pwd)`
		else
		    AC_MSG_ERROR([${with_$1config} directory doesn't contain $1Config.sh])
		fi
	    fi

	    # then check for a private $1 installation
	    if test x"${ac_cv_c_$1config}" = x ; then
		for i in \
			../$1 \
			`ls -dr ../$1[[8-9]].[[0-9]]* 2>/dev/null` \
			../../$1 \
			`ls -dr ../../$1[[8-9]].[[0-9]]* 2>/dev/null` \
			../../../$1 \
			`ls -dr ../../../$1[[8-9]].[[0-9]]* 2>/dev/null` \
			${srcdir}/../$1 \
			`ls -dr ${srcdir}/../$1[[8-9]].[[0-9]]* 2>/dev/null` \
			; do
		    if test -f "$i/$1Config.sh" ; then
			ac_cv_c_$1config=`(cd $i; pwd)`
			break
		    fi
		    if test -f "$i/unix/$1Config.sh" ; then
			ac_cv_c_$1config=`(cd $i/unix; pwd)`
			break
		    fi
		done
	    fi

	    # check in a few common install locations
	    if test x"${ac_cv_c_$1config}" = x ; then
		for i in `ls -d ${exec_prefix}/lib 2>/dev/null` \
			`ls -d ${prefix}/lib 2>/dev/null` \
			`ls -d /usr/local/lib 2>/dev/null` \
			`ls -d /usr/contrib/lib 2>/dev/null` \
			`ls -d /usr/lib 2>/dev/null` \
			; do
		    if test -f "$i/$1Config.sh" ; then
			ac_cv_c_$1config=`(cd $i; pwd)`
			break
		    fi
		done
	    fi
	])

	if test x"${ac_cv_c_$1config}" = x ; then
	    $1_BIN_DIR="# no $1 configs found"
	    AC_MSG_WARN("Cannot find $1 configuration definitions")
	    exit 0
	else
	    no_$1=
	    $1_BIN_DIR=${ac_cv_c_$1config}
	    AC_MSG_RESULT([found $$1_BIN_DIR/$1Config.sh])
	fi
    fi
])

#------------------------------------------------------------------------
# PILOT_LINK_LOAD_CONFIG --
#
#	Load the $1Config.sh file
#
# Arguments:
#	
#	Requires the following vars to be set:
#		$1_BIN_DIR
#
# Results:
#
#	Subst the following vars:
#		$1_SRC_DIR
#		$1_LIB_FILE
#		$1_LIB_SPEC
#
#------------------------------------------------------------------------

AC_DEFUN([PILOT_LINK_LOAD_CONFIG], [
    AC_MSG_CHECKING([for existence of ${$1_BIN_DIR}/$1Config.sh])

    if test -f "${$1_BIN_DIR}/$1Config.sh" ; then
        AC_MSG_RESULT([loading])
	. ${$1_BIN_DIR}/$1Config.sh
    else
        AC_MSG_RESULT([file not found])
    fi

    #
    # If the $1_BIN_DIR is the build directory (not the install directory),
    # then set the common variable name to the value of the build variables.
    # For example, the variable $1_LIB_SPEC will be set to the value
    # of $1_BUILD_LIB_SPEC. An extension should make use of $1_LIB_SPEC
    # instead of $1_BUILD_LIB_SPEC since it will work with both an
    # installed and uninstalled version of Tcl.
    #

    if test -f ${$1_BIN_DIR}/Makefile ; then
	AC_MSG_WARN([Found Makefile - using build library specs for $1])
        $1_LIB_SPEC=${$1_BUILD_LIB_SPEC}
        $1_STUB_LIB_SPEC=${$1_BUILD_STUB_LIB_SPEC}
        $1_STUB_LIB_PATH=${$1_BUILD_STUB_LIB_PATH}
    fi

    AC_SUBST($1_VERSION)
    AC_SUBST($1_SRC_DIR)

    AC_SUBST($1_LIB_FILE)
    AC_SUBST($1_LIB_SPEC)

    AC_SUBST($1_STUB_LIB_FILE)
    AC_SUBST($1_STUB_LIB_SPEC)
    AC_SUBST($1_STUB_LIB_PATH)
])

