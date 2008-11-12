#!/bin/sh

if [ "$1" = "-noqcop" ]; then
    NO_QCOP=1
    shift
fi

/etc/rc.d/rc.gadget "$1"

if [ "$NO_QCOP" -ne 1 ]; then
    if [ "$(cat /etc/gadget)" = ether ]; then
        qcop send "QPE/NetworkState" "updateNetwork()" &
    fi
fi

