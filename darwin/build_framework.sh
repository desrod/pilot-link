#!/bin/sh
#
# Build a libpisock.framework embeddable in an application
#
what=libpisock
libs=../libpisock/.libs/libpisock.a
linkflags="-framework Carbon -framework System -framework IOKit -liconv -lgcc"
incs=../include

rm -Rf $what.framework
mkdir -p $what.framework/Versions/A/Headers

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
