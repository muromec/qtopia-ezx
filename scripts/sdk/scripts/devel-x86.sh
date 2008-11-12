#!/bin/sh

. /opt/Qtopia/SDK/scripts/functions

QPEVER=`version`

export QPEDIR=/opt/Qtopia/SDK/$QPEVER/x86
export PATH=/opt/Qtopia/SDK/scripts:$QPEDIR/bin:$QPEDIR/scripts:$PATH
