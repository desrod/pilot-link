#!/bin/sh
#
# Build a libpisock.framework embeddable in an application
#
# Usage examples:
#
# sh build_framework.sh
#	builds the framework without prebinding
#
# sh build_framework.sh 0x12345678
#	builds the framework prebound at address 0x12345678, suited for
#	building frameworks that are going to be linked to prebound
#	executables
#
# Copyright (c) 2004, Florent Pillet
#
what=libpisock
libs=../libpisock/.libs/libpisock.a
linkflags="-framework Carbon -framework System -framework IOKit -liconv -lgcc"
incs=../include

rm -Rf $what.framework
mkdir -p $what.framework/Versions/A/Headers

if [ $1 ];
then
	export LD_PREBIND=1
	linkflags="$linkflags -seg1addr $1";
fi

libtool -dynamic \
	-o $what.framework/Versions/A/$what \
	-install_name @executable_path/../Frameworks/$what.framework/Versions/A/$what \
	$libs $linkflags

cp $incs/*.h $what.framework/Versions/A/Headers/

cd $what.framework/Versions
ln -sf A Current
cd ..
ln -sf Versions/Current/Headers Headers
ln -sf Versions/Current/$what $what
cd ..
