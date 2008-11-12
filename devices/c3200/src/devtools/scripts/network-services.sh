#!/bin/sh

case $1 in
    start)
        telnetd -l /bin/sh
        ftpd -S
        /etc/init.d/samba.sh start
        ;;
    stop)
        killall telnetd
        killall ftpd
        /etc/init.d/samba.sh stop
        ;;
    *)
        echo "$0 {start|stop}"
        exit 1;
        ;;
esac
