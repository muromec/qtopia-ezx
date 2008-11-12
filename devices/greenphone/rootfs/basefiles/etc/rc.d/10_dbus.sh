#!/bin/sh

case $1 in
start)
    dbus-daemon --system
    ;;
stop)
    kill $( pidof dbus-daemon ) 2>/dev/null
    rm -f /var/run/dbus/pid
    ;;
*)
    echo "usage: $0 { start | stop }"
    exit 2
    ;;
esac

exit 0


