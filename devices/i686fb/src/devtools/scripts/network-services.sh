#!/bin/sh

case $1 in
    start)
        telnetd -l /bin/sh
        ftpd -S
        ;;
    stop)
        killall telnetd
        killall ftpd
        ;;
    *)
        echo "$0 {start|stop}"
        exit 1;
        ;;
esac
