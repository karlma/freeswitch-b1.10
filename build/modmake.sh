#!/bin/sh
pwd=$1
shift
mod=$1
shift

if [ -f $pwd/build/freeswitch.env ] ; then
    . $pwd/build/freeswitch.env
fi

make=`which gmake`

if [ -z $make ] ; then
    make=`which make`
fi

end=`echo $mod | sed "s/^.*\///g"`
if [ -z $end ] ; then
    end=$mod
fi

if [ -f $mod/Makefile ] ; then
    CFLAGS="$MOD_CFLAGS $CFLAGS " MODNAME=$end $make -C $mod $@
else 
    CFLAGS="$MOD_CFLAGS $CFLAGS" MODNAME=$end $make -f $pwd/generic_mod.mk -C $mod $@
fi

