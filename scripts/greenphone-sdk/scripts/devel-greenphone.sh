#!/bin/sh

. /opt/Qtopia/SDK/scripts/functions

QPEVER=`version`

export QPEDIR=/opt/Qtopia/SDK/$QPEVER/greenphone
export PATH=/opt/Qtopia/SDK/scripts:$QPEDIR/bin:$QPEDIR/scripts:/opt/toolchains/greenphone/gcc-4.1.1-glibc-2.3.6/arm-linux/bin:$PATH
