#!/bin/sh

#LD_LIBRARY_PATH=../../../libpisock/.libs gdb --args python pisocktests.py  $*
LD_LIBRARY_PATH=../../../libpisock/.libs python pisocktests.py $*

